#include "common.h"

#include "cthulhu/tree/query.h"

#include "std/map.h"
#include "std/vector.h"
#include "std/typed/vector.h"

#include "base/memory.h"
#include "base/panic.h"

#include <string.h>

ssa_type_t *ssa_type_new(ssa_kind_t kind, const char *name, quals_t quals)
{
    ssa_type_t *type = ctu_malloc(sizeof(ssa_type_t));
    type->kind = kind;
    type->quals = quals;
    type->name = name;
    return type;
}

ssa_type_t *ssa_type_empty(const char *name, quals_t quals)
{
    return ssa_type_new(eTypeEmpty, name, quals);
}

ssa_type_t *ssa_type_unit(const char *name, quals_t quals)
{
    return ssa_type_new(eTypeUnit, name, quals);
}

ssa_type_t *ssa_type_bool(const char *name, quals_t quals)
{
    return ssa_type_new(eTypeBool, name, quals);
}

ssa_type_t *ssa_type_digit(const char *name, quals_t quals, sign_t sign, digit_t digit)
{
    ssa_type_digit_t it = { .sign = sign, .digit = digit };
    ssa_type_t *type = ssa_type_new(eTypeDigit, name, quals);
    type->digit = it;
    return type;
}

ssa_type_t *ssa_type_closure(const char *name, quals_t quals, ssa_type_t *result, typevec_t *params, bool variadic)
{
    ssa_type_closure_t it = {
        .result = result,
        .params = params,
        .variadic = variadic
    };

    ssa_type_t *closure = ssa_type_new(eTypeClosure, name, quals);
    closure->closure = it;
    return closure;
}

ssa_type_t *ssa_type_pointer(const char *name, quals_t quals, ssa_type_t *pointer, size_t length)
{
    ssa_type_pointer_t it = {
        .pointer = pointer,
        .length = length
    };

    ssa_type_t *ptr = ssa_type_new(eTypePointer, name, quals);
    ptr->pointer = it;
    return ptr;
}

ssa_type_t *ssa_type_opaque_pointer(const char *name, quals_t quals)
{
    return ssa_type_new(eTypeOpaque, name, quals);
}

ssa_type_t *ssa_type_struct(const char *name, quals_t quals, typevec_t *fields)
{
    ssa_type_record_t it = { .fields = fields };
    ssa_type_t *type = ssa_type_new(eTypeRecord, name, quals);
    type->record = it;
    return type;
}

static typevec_t *collect_params(map_t *cache, const tree_t *type)
{
    vector_t *vec = tree_fn_get_params(type);

    size_t len = vector_len(vec);
    typevec_t *result = typevec_of(sizeof(ssa_param_t), len);

    for (size_t i = 0; i < len; i++)
    {
        const tree_t *param = vector_get(vec, i);
        CTASSERTF(tree_is(param, eTreeDeclParam), "expected param, got %s", tree_to_string(param));

        const char *name = tree_get_name(param);
        const tree_t *type = tree_get_type(param);

        ssa_param_t entry = {
            .name = name,
            .type = ssa_type_create_cached(cache, type)
        };

        typevec_set(result, i, &entry);
    }

    return result;
}

static typevec_t *collect_fields(map_t *cache, const tree_t *type)
{
    size_t len = vector_len(type->fields);
    typevec_t *result = typevec_of(sizeof(ssa_field_t), len);
    for (size_t i = 0; i < len; i++)
    {
        const tree_t *field = vector_get(type->fields, i);
        CTASSERTF(tree_is(field, eTreeDeclField), "expected field, got %s", tree_to_string(field));

        const char *name = tree_get_name(field);
        const tree_t *type = tree_get_type(field);

        ssa_field_t entry = {
            .name = name,
            .type = ssa_type_create_cached(cache, type)
        };

        typevec_set(result, i, &entry);
    }

    return result;
}

static ssa_type_t *ssa_type_create(map_t *cache, const tree_t *type)
{
    tree_kind_t kind = tree_get_kind(type);
    const char *name = tree_get_name(type);
    quals_t quals = tree_ty_get_quals(type);

    switch (kind)
    {
    case eTreeTypeEmpty: return ssa_type_empty(name, quals);
    case eTreeTypeUnit: return ssa_type_unit(name, quals);
    case eTreeTypeBool: return ssa_type_bool(name, quals);
    case eTreeTypeDigit: return ssa_type_digit(name, quals, type->sign, type->digit);
    case eTreeTypeClosure:
        return ssa_type_closure(
            /* name = */ name,
            /* quals = */ quals,
            /* result = */ ssa_type_create_cached(cache, tree_fn_get_return(type)),
            /* params = */ collect_params(cache, type),
            /* variadic = */ tree_fn_get_arity(type) == eArityVariable
        );

    case eTreeTypeReference:
        return ssa_type_pointer(name, quals, ssa_type_create_cached(cache, type->ptr), 1);

    case eTreeTypeArray:
    case eTreeTypePointer:
        return ssa_type_pointer(name, quals, ssa_type_create_cached(cache, type->ptr), type->length);

    case eTreeTypeOpaque:
        return ssa_type_opaque_pointer(name, quals);

    case eTreeTypeEnum: return ssa_type_create(cache, type->underlying);
    case eTreeTypeStruct: return ssa_type_struct(name, quals, collect_fields(cache, type));

    default: NEVER("unexpected type kind: %s", tree_to_string(type));
    }
}

ssa_type_t *ssa_type_create_cached(map_t *cache, const tree_t *type)
{
    ssa_type_t *old = map_get_ptr(cache, type);
    if (old != NULL) { return old; }

    ssa_type_t *temp = ssa_type_empty(tree_get_name(type), eQualUnknown);
    map_set_ptr(cache, type, temp);

    ssa_type_t *result = ssa_type_create(cache, type);
    memcpy(temp, result, sizeof(ssa_type_t));
    return temp;
}
