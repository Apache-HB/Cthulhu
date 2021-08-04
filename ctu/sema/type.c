static type_t *query_type(sema_t *sema, node_t *it);

static type_t *query_type_local(sema_t *sema, node_t *node, const char *name) {    
    node_t *type = map_get(sema->types, name);
    if (type) {
        return get_type(type);
    }

    type_t *builtin = map_get(builtins, name);
    if (builtin) {
        return builtin;
    }

    if (sema->parent) {
        return query_type_local(sema->parent, node, name);
    }

    reportf(LEVEL_ERROR, node, "unknown type `%s`", name);
    return new_poison(node, format("unresolved type `%s`", name));
}

static type_t *query_type_symbol(sema_t *sema, node_t *expr, list_t *symbol) {
    const char *first = list_first(symbol);
    if (symbol->len == 1) {
        return query_type_local(sema, expr, first);
    }

    sema_t *other = map_get(sema->imports, first);
    if (other) {
        list_t slice = list_slice(symbol, 1);
        return query_type_symbol(other, expr, &slice);
    }

    reportf(LEVEL_ERROR, expr, "unknown type `%s`", first);
    return new_poison(expr, "unresolved type");
}

static type_t *query_array(sema_t *sema, node_t *expr) {
    type_t *of = query_type(sema, expr->of);
    type_t *len = NULL;
    size_t size = 0;

    if (expr->size) {
        len = query_expr(sema, expr->size);
        
        if (!type_can_become_explicit(&expr->size, size_int(), len)) {
            reportf(LEVEL_ERROR, expr, "array size must be convertible to usize, `%s` is incompatible", typefmt(len));
            return new_poison(expr, "unresolved array size");
        }

        if (!is_consteval(expr->size)) {
            reportf(LEVEL_ERROR, expr, "array size must be constant");
        } else {
            mpz_t num;
            if (!eval_ast(num, expr->size)) {
                reportf(LEVEL_INTERNAL, expr->size, "failed to evaluate size");
            }
            size = mpz_get_ui(num);
        }
    }

    return new_array(expr, of, size, expr->size == NULL);
}

/**
 * find a type given a typename
 */
static type_t *query_type(sema_t *sema, node_t *it) {
    type_t *type = raw_type(it); 
    if (type) {
        return type;
    }

    switch (it->kind) {
    case AST_SYMBOL:
        type = query_type_symbol(sema, it, it->ident);
        break;

    case AST_DECL_PARAM:
        type = query_type(sema, it->type);
        type = set_lvalue(type, true);
        break;

    case AST_PTR:
        type = query_type(sema, it->ptr);
        type = new_pointer(it, type);
        break;

    case AST_MUT:
        type = query_type(sema, it->next);
        type = set_mut(type, true);
        break;

    case AST_ARRAY:
        type = query_array(sema, it);
        break;

    default:
        assert("query-type invalid %d", it->kind);
        break;
    }

    connect_type(it, type);

    return type;
}

static void record_contains(type_t *type, type_t *other) {
    if (!is_record(type)) {
        return;
    }

    for (size_t i = 0; i < type->fields.size; i++) {
        field_t field = type->fields.fields[i];
        type_t *ty = field.type;
        if (types_equal(ty, other)) {
            reportf(LEVEL_ERROR, nodeof(ty), "recursive field `%s`", field.name);
            ty->valid = false;
        }

        if (is_record(ty) && ty->valid) {
            record_contains(ty, other);
        }
    }
}

static void recursive_record(type_t *it) {
    record_contains(it, it);
}

static void add_field(sema_t *sema, size_t at, type_t *record, node_t *field) {
    const char *name = get_field_name(field);

    if (!is_discard_name(name)) {
        for (size_t i = 0; i < at; i++) {
            field_t it = record->fields.fields[i];
            const char *other = it.name;

            if (strcmp(name, other) == 0) {
                reportf(LEVEL_ERROR, field, "duplicate field `%s`", name);
            }
        }
    }

    type_t *ty = query_type(sema, field->type);

    if (ty->kind == TYPE_UNRESOLVED) {
        reportf(LEVEL_ERROR, field, "unresolved field type `%s`", name);
    }

    if (is_array(ty) && ty->unbounded) {
        reportf(LEVEL_ERROR, field, "structs may not contain unbounded arrays");
    }

    record->fields.fields[at] = new_type_field(name, ty);
}

static void build_record(sema_t *sema, node_t *node) {
    list_t *fields = node->fields;
    size_t len = list_len(fields);

    type_t *result = raw_type(node);
    resize_type(result, len);

    for (size_t i = 0; i < len; i++) {
        node_t *field = list_at(fields, i);
        add_field(sema, i, result, field);
    }

    connect_type(node, result);

    recursive_record(result);
}

static void add_item(size_t idx, type_t *result, node_t *field) {
    const char *name = get_item_name(field);

    if (is_discard_name(name)) {
        reportf(LEVEL_ERROR, field, "enum cannot contain discard values");
        return;
    }

    for (size_t i = 0; i < idx; i++) {
        field_t it = result->fields.fields[i];
        if (strcmp(it.name, name) == 0) {
            reportf(LEVEL_ERROR, field, "duplicate enum value `%s`", name);
            return;
        }
    }

    node_t *val = field->init;
    size_t out;

    if (val) {
        if (!is_consteval(val)) {
            reportf(LEVEL_ERROR, field, "enum value for `%s` must be constant", name);
            return;
        }

        mpz_t num;
        if (!eval_ast(num, val)) {
            reportf(LEVEL_INTERNAL, val, "failed to evaluate enum value for `%s`", name);
            return;
        }
        out = mpz_get_ui(num);
    } else {
        out = idx;
    }

    result->fields.fields[idx] = new_init_field(name, result, out);

    connect_type(field, result);
}

static void build_enum(node_t *node) {
    list_t *fields = node->fields;
    size_t len = list_len(fields);

    type_t *result = raw_type(node);
    resize_type(result, len);

    for (size_t i = 0; i < len; i++) {
        node_t *field = list_at(fields, i);
        add_item(i, result, field);
    }

    connect_type(node, result);
}
