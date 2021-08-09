#include "ast.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

#include "ctu/util/str.h"
#include "ctu/util/report.h"
#include "ctu/util/util.h"

#include "ctu/debug/ast.h"

static node_t *new_node(scanner_t *scanner, where_t where, ast_t kind) {
    node_t *node = ctu_malloc(sizeof(node_t));

    node->kind = kind;
    node->scanner = scanner;
    node->where = where;
    node->typeof = NULL;
    node->attribs = 0;
    node->local = NOT_LOCAL;
    node->cast = NULL;

    return node;
}

static node_t *new_decl(scanner_t *scanner, where_t where, ast_t kind, char *name) {
    node_t *decl = new_node(scanner, where, kind);
    decl->name = name;
    decl->ctx = NULL;
    decl->decorate = new_list(NULL);
    decl->section = NULL;
    return decl;
}

const char *get_decl_name(node_t *node) {
    switch (node->kind) {
    case AST_DECL_FUNC: case AST_DECL_VAR: case AST_DECL_PARAM:
    case AST_DECL_STRUCT: case AST_DECL_FIELD: case AST_DECL_ENUM:
    case AST_DECL_UNION:
        return node->name;

    default:
        reportf(LEVEL_INTERNAL, node, "node is not a declaration");
        return "not-a-decl";
    }
}

const char *get_resolved_name(node_t *node) {
    switch (node->kind) {
    case AST_BUILTIN_TYPE:
        return node->nameof;

    default:
        return get_decl_name(node);
    }
}

const char *get_field_name(node_t *node) {
    switch (node->kind) {
    case AST_DECL_FIELD:
        return node->name;
    
    default:
        reportf(LEVEL_INTERNAL, node, "node is not a field");
        return "not-a-field";
    }
}

const char *get_item_name(node_t *node) {
    switch (node->kind) {
    case AST_ENUM_ITEM:
        return node->name;

    default:
        reportf(LEVEL_INTERNAL, node, "node is not an enum item");
        return "not-an-enum-item";
    }
}

bool is_discard_name(const char *name) {
    return name[0] == '$';
}

type_t *raw_type(node_t *node) {
    return node->typeof;
}

type_t *get_type(node_t *node) {
    type_t *type = raw_type(node);
    
    if (type == NULL) {
        type = new_unresolved(node);
    }

    return type;
}

type_t *get_resolved_type(node_t *node) {
    type_t *type = raw_type(node);
    
    if (type == NULL) {
        reportf(LEVEL_INTERNAL, node, "type of node is not known");
        type = new_unresolved(node);
    }

    return type;
}

list_t *get_stmts(node_t *node) {
    ASSERT(node->kind == AST_STMTS)("node->kind != AST_STMTS when calling get_stmts");

    return node->stmts;
}

bool is_math_op(binary_t op) {
    switch (op) {
    case BINARY_ADD: case BINARY_SUB:
    case BINARY_DIV: case BINARY_MUL:
    case BINARY_REM: 
        return true;
    default:
        return false;
    }
}

bool is_comparison_op(binary_t op) {
    switch (op) {
    case BINARY_GT: case BINARY_GTE:
    case BINARY_LT: case BINARY_LTE:
        return true;
    default:
        return false;
    }
}

bool is_equality_op(binary_t op) {
    switch (op) {
    case BINARY_EQ: case BINARY_NEQ:
        return true;
    default:
        return false;
    }
}

bool is_deref(node_t *expr) { 
    return expr->kind == AST_UNARY && expr->unary == UNARY_DEREF;
}

bool is_access(node_t *expr) {
    return expr->kind == AST_ACCESS;
}

bool is_symbol(node_t *it) {
    return it->kind == AST_SYMBOL;
}

bool is_index(node_t *expr) {
    return expr->kind == AST_INDEX;
}

node_t *make_implicit(node_t *node) {
    node->attribs |= ATTR_IMPLICIT;
    return node;
}

node_t *make_exported(node_t *node) {
    node->attribs |= ATTR_EXPORT;
    return node;
}

static void add_integer_type(node_t *node, char *str) {
    while (isxdigit(*str)) {
        str++;
    }

    bool sign = true;
    if (*str == 'u') {
        *str = 0;
        sign = false;
        str += 1;
    }

    integer_t kind = INTEGER_INT;

    switch (*str) {
    case 't':
        /* char suffix */
        kind = INTEGER_CHAR;
        break;
    case 's':
        /* short suffix */
        kind = INTEGER_SHORT;
        break;
    case 'i':
        /* int suffix */
        kind = INTEGER_INT;
        break;
    case 'l':
        /* long suffix */
        kind = INTEGER_LONG;
        break;
    case 'z':
        /* size suffix */
        kind = INTEGER_SIZE;
        break;
    case 'p':
        /* ptr suffix */
        kind = INTEGER_INTPTR;
        break;
    case 'm':
        /* max suffix */
        kind = INTEGER_INTMAX;
        break;
    
    case '\0': break;

    default:
        assert("invalid suffix `%c`", *str);
        break;
    }

    *str = 0;

    node->sign = sign;
    node->integer = kind;
}

node_t *ast_digit(scanner_t *scanner, where_t where, char *digit, int base) {
    node_t *node = new_node(scanner, where, AST_DIGIT);

    add_integer_type(node, digit);

    if (mpz_init_set_str(node->num, digit, base)) {
        report(LEVEL_INTERNAL, scanner, where, "failed to initialize mpz_t digit");
    }

    ctu_free(digit);

    sanitize_range(
        get_int_type(node->sign, node->integer),
        node->num,
        scanner,
        where
    );

    return node;
}

node_t *ast_bool(scanner_t *scanner, where_t where, bool boolean) {
    node_t *node = new_node(scanner, where, AST_BOOL);

    node->boolean = boolean;

    return node;
}

static char *escape_string(scanner_t *scanner, where_t where, const char *str) {
    size_t len = strlen(str);
    char *out = ctu_malloc(len + 1);

    size_t idx = 0;
    size_t dst = 0;

    while (str[idx]) {
        char c = str[idx++];
        if (c == '\\') {
            char n = str[idx++];
            switch (n) {
            case '\\': out[dst++] = '\\'; break;
            case 'n': out[dst++] = '\n'; break;
            case 't': out[dst++] = '\t'; break;
            case 'r': out[dst++] = '\r'; break;
            case '"': out[dst++] = '"'; break;
            case '\'': out[dst++] = '\''; break;
            case '0': out[dst++] = '\0'; break;
            case 'a': out[dst++] = '\a'; break;
            case 'b': out[dst++] = '\b'; break;
            case 'e': out[dst++] = '\x1b'; break;
            case 'v': out[dst++] = '\v'; break;
            case 'f': out[dst++] = '\f'; break;
            default:
                report(LEVEL_ERROR, scanner, where, "invalid escape sequence `\\%c`", n);
                break;
            }
        } else {
            out[dst++] = c;
        }
    }

    out[dst] = 0;

    return out;
}

node_t *ast_string(scanner_t *scanner, where_t where, char *string) {
    node_t *node = new_node(scanner, where, AST_STRING);

    char *in = string + 1;
    string[strlen(string) - 1] = '\0';

    node->string = escape_string(scanner, where, in);

    ctu_free(string);

    return node;
}

node_t *ast_symbol(scanner_t *scanner, where_t where, list_t *text) {
    node_t *node = new_node(scanner, where, AST_SYMBOL);

    for (size_t i = 0; i < list_len(text); i++) {
        char *str = list_at(text, i);
        if (is_discard_name(str)) {
            reportf(LEVEL_ERROR, node, "symbol may not contain discard name `$`");
        }
    }

    node->ident = text;

    return node;
}

node_t *ast_pointer(scanner_t *scanner, where_t where, node_t *ptr) {
    node_t *node = new_node(scanner, where, AST_PTR);
    node->ptr = ptr;
    return node;
}

node_t *ast_unary(scanner_t *scanner, where_t where, unary_t unary, node_t *expr) {
    node_t *node = new_node(scanner, where, AST_UNARY);

    node->unary = unary;
    node->expr = expr;

    return node;
}

node_t *ast_binary(scanner_t *scanner, where_t where, binary_t binary, node_t *lhs, node_t *rhs) {
    node_t *node = new_node(scanner, where, AST_BINARY);

    node->binary = binary;
    node->lhs = lhs;
    node->rhs = rhs;

    return node;
}

node_t *ast_call(scanner_t *scanner, where_t where, node_t *body, list_t *args) {
    node_t *node = new_node(scanner, where, AST_CALL);

    node->expr = body;
    node->args = args;

    return node;
}

node_t *ast_cast(scanner_t *scanner, where_t where, node_t *expr, node_t *cast) {
    node_t *node = new_node(scanner, where, AST_CAST);

    node->expr = expr;
    node->convert = cast;

    return node;
}

node_t *ast_stmts(scanner_t *scanner, where_t where, list_t *stmts) {
    node_t *node = new_node(scanner, where, AST_STMTS);

    node->stmts = stmts;

    return node;
}

node_t *ast_return(scanner_t *scanner, where_t where, node_t *expr) {
    node_t *node = new_node(scanner, where, AST_RETURN);

    node->expr = expr;

    return node;
}

node_t *ast_branch(scanner_t *scanner, where_t where, node_t *cond, node_t *branch) {
    node_t *node = new_node(scanner, where, AST_BRANCH);

    node->cond = cond;
    node->branch = branch;
    node->next = NULL;

    return node;
}

node_t *add_branch(node_t *branch, node_t *next) {
    branch->next = next;
    return branch;
}

node_t *ast_assign(scanner_t *scanner, where_t where, node_t *dst, node_t *src) {
    node_t *node = new_node(scanner, where, AST_ASSIGN);

    node->dst = dst;
    node->src = src;

    return node;
}

node_t *ast_while(scanner_t *scanner, where_t where, node_t *cond, node_t *body) {
    node_t *node = new_node(scanner, where, AST_WHILE);

    node->cond = cond;
    node->next = body;

    return node;
}

node_t *ast_decl_func(
    scanner_t *scanner, where_t where, 
    char *name, list_t *params,
    node_t *result, node_t *body) {

    node_t *node = new_decl(scanner, where, AST_DECL_FUNC, name);

    node->params = params;
    node->result = result;
    node->body = body;

    return node;
}

node_t *ast_decl_param(scanner_t *scanner, where_t where, char *name, node_t *type) {
    node_t *node = new_decl(scanner, where, AST_DECL_PARAM, name);

    node->type = type;

    return node;
}

node_t *ast_decl_var(scanner_t *scanner, where_t where, bool mut, char *name, node_t *type, node_t *init) {
    node_t *node = new_decl(scanner, where, AST_DECL_VAR, name);

    if (mut) {
        node->attribs |= ATTR_MUTABLE;
    }

    node->type = type;
    node->init = init;

    return node;
}

node_t *ast_mut(scanner_t *scanner, where_t where, node_t *it) {
    node_t *node = new_node(scanner, where, AST_MUT);
    node->next = it;
    return node;
}

node_t *ast_type(const char *name) {
    node_t *node = new_node(NULL, NOWHERE, AST_BUILTIN_TYPE);
    node->nameof = name;
    return node;
}

node_t *ast_access(scanner_t *scanner, where_t where, node_t *expr, char *name, bool indirect) {
    node_t *node = new_node(scanner, where, AST_ACCESS);
    
    node->target = expr;
    node->field = name;
    node->indirect = indirect;

    return node;
}

node_t *ast_decl_struct(scanner_t *scanner, where_t where, char *name, list_t *fields) {
    node_t *node = new_decl(scanner, where, AST_DECL_STRUCT, name);

    node->fields = fields;

    return node;
}

node_t *ast_field(scanner_t *scanner, where_t where, char *name, node_t *type) {
    node_t *node = new_node(scanner, where, AST_DECL_FIELD);

    node->name = name;
    node->type = type;

    return node;
}

node_t *ast_import(scanner_t *scanner, where_t where, list_t *path) {
    node_t *node = new_node(scanner, where, AST_DECL_IMPORT);

    node->path = path;

    return node;
}

node_t *ast_root(scanner_t *scanner, list_t *imports, list_t *decls) {
    node_t *node = new_node(scanner, NOWHERE, AST_ROOT);

    node->imports = imports;
    node->decls = decls;

    return node;
}

bool is_exported(node_t *node) {
    return node->attribs & ATTR_EXPORT;
}

bool is_mut(node_t *node) {
    return node->attribs & ATTR_MUTABLE;
}

void mark_used(node_t *node) {
    node->attribs |= ATTR_USED;
}

bool is_used(node_t *node) {
    return node->attribs & ATTR_USED;
}

node_t *ast_attribs(node_t *decl, attrib_t attribs, list_t *decorate) {
    decl->attribs |= attribs;
    decl->decorate = decorate;
    return decl;
}

node_t *ast_attrib(scanner_t *scanner, where_t where, list_t *name, list_t *args) {
    node_t *decl = new_node(scanner, where, AST_ATTRIB);

    decl->attr = name;
    decl->args = args;

    return decl;
}

void mark_interop(node_t *node) {
    node->attribs |= ATTR_INTEROP;
}

bool is_interop(node_t *node) {
    return node->attribs & ATTR_INTEROP;
}

node_t *ast_noop(scanner_t *scanner, where_t where) {
    return new_node(scanner, where, AST_NOOP);
}

node_t *ast_array(scanner_t *scanner, where_t where, node_t *of, node_t *size) {
    node_t *node = new_node(scanner, where, AST_ARRAY);

    node->of = of;
    node->size = size;

    return node;
}

node_t *ast_index(scanner_t *scanner, where_t where, node_t *expr, node_t *index) {
    node_t *node = new_node(scanner, where, AST_INDEX);

    node->expr = expr;
    node->index = index;

    return node;
}

node_t *ast_decl_union(scanner_t *scanner, where_t where, char *name, list_t *fields) {
    node_t *node = new_decl(scanner, where, AST_DECL_UNION, name);

    node->fields = fields;

    return node;
}

node_t *ast_decl_enum(scanner_t *scanner, where_t where, char *name, list_t *fields) {
    node_t *node = new_decl(scanner, where, AST_DECL_ENUM, name);

    node->fields = fields;

    return node;
}

node_t *ast_enum_item(scanner_t *scanner, where_t where, char *name, node_t *init) {
    node_t *node = new_decl(scanner, where, AST_ENUM_ITEM, name);

    node->init = init;

    return node;
}

node_t *ast_break(scanner_t *scanner, where_t where) {
    return new_node(scanner, where, AST_BREAK);
}

node_t *ast_continue(scanner_t *scanner, where_t where) {
    return new_node(scanner, where, AST_CONTINUE);
}

node_t *ast_sizeof(node_t *it) {
    node_t *node = new_node(NULL, NOWHERE, AST_BUILTIN_SIZEOF);

    node->of = it;

    return node;
}

node_t *ast_null(scanner_t *scanner, where_t where) {
    return new_node(scanner, where, AST_NULL);
}

node_t *ast_funcptr(scanner_t *scanner, where_t where, list_t *params, node_t *result) {
    node_t *node = new_node(scanner, where, AST_FUNCPTR);

    node->params = params;
    node->result = result;

    return node;
}

node_t *ast_arg(scanner_t *scanner, where_t where, char *name, node_t *expr) {
    node_t *node = new_node(scanner, where, AST_ARG);

    node->name = name;
    node->arg = expr;

    return node;
}

void mark_section(node_t *node, const char *section) {
    node->section = section;
}

node_t *ast_list(scanner_t *scanner, where_t where, list_t *init, node_t *kind) {
    node_t *node = new_node(scanner, where, AST_LIST);

    node->exprs = init;
    node->elem = kind;

    return node;
}
