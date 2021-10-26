#pragma once

#include "ctu/ast/ast.h"
#include "ctu/ast/ops/ops.h"
#include "ctu/lir/lir.h"

#include <gmp.h>

typedef enum {
    CTU_DIGIT,
    CTU_PATH,
    CTU_BOOL,
    CTU_STRING,

    CTU_UNARY,
    CTU_BINARY,
    CTU_CALL,
    CTU_ACCESS,
    CTU_CAST,
    CTU_LAMBDA,
    CTU_NULL,

    CTU_STMTS,
    CTU_RETURN,
    CTU_WHILE,
    CTU_ASSIGN,
    CTU_BRANCH,
    CTU_BREAK,

    CTU_TYPEPATH,
    CTU_POINTER,
    CTU_CLOSURE,
    CTU_MUTABLE,
    CTU_VARARGS,

    CTU_VALUE,
    CTU_PARAM,
    CTU_DEFINE,
    CTU_ATTRIB,
    CTU_NEWTYPE,

    CTU_IMPORT,
    CTU_MODULE
} ctu_type_t;

typedef struct ctu_t {
    ctu_type_t type;
    node_t *node;
    lir_t *lir;

    union {
        mpz_t digit;

        bool boolean;

        const char *ident;

        const char *str;

        struct ctu_t *ptr;

        struct {
            unary_t unary;
            struct ctu_t *operand;
        };

        struct {
            struct ctu_t *object;
            const char *field;
            bool indirect;
        };

        struct {
            binary_t binary;
            struct ctu_t *lhs;
            struct ctu_t *rhs;
        };

        struct {
            struct ctu_t *dst;
            struct ctu_t *src;
        };

        struct {
            struct ctu_t *func;
            vector_t *args;
        };

        struct {
            struct ctu_t *cond;
            struct ctu_t *then;
            struct ctu_t *other;
        };

        struct {
            const char *name;
            vector_t *attribs;
            bool exported; /* is this exported on local */

            union {
                struct {
                    bool mut;
                    struct ctu_t *kind;
                    struct ctu_t *value;
                };

                struct {
                    vector_t *params;
                    struct ctu_t *result;
                    struct ctu_t *body;
                };

                vector_t *fields;
            };
        };

        struct {
            vector_t *imports;
            vector_t *decls;
        };

        struct {
            vector_t *path;
            const char *alias;
        };

        vector_t *stmts;
    };
} ctu_t;

/* literals */
ctu_t *ctu_digit(scan_t *scan, where_t where, mpz_t digit);
ctu_t *ctu_path(scan_t *scan, where_t where, vector_t *path);
ctu_t *ctu_bool(scan_t *scan, where_t where, bool value);
ctu_t *ctu_string(scan_t *scan, where_t where, const char *str);
ctu_t *ctu_null(scan_t *scan, where_t where);

/* expressions */
ctu_t *ctu_unary(scan_t *scan, where_t where, unary_t unary, ctu_t *operand);
ctu_t *ctu_binary(scan_t *scan, where_t where, binary_t binary, ctu_t *lhs, ctu_t *rhs);
ctu_t *ctu_call(scan_t *scan, where_t where, ctu_t *func, vector_t *args);
ctu_t *ctu_access(scan_t *scan, where_t where, ctu_t *object, const char *field, bool indirect);
ctu_t *ctu_cast(scan_t *scan, where_t where, ctu_t *expr, ctu_t *type);
ctu_t *ctu_lambda(scan_t *scan, where_t where, vector_t *params, ctu_t *result, ctu_t *body);

/* statements */
ctu_t *ctu_stmts(scan_t *scan, where_t where, vector_t *stmts);
ctu_t *ctu_return(scan_t *scan, where_t where, ctu_t *operand);
ctu_t *ctu_while(scan_t *scan, where_t where, ctu_t *cond, ctu_t *body);
ctu_t *ctu_assign(scan_t *scan, where_t where, ctu_t *dst, ctu_t *src);
ctu_t *ctu_branch(scan_t *scan, where_t where, ctu_t *cond, ctu_t *then, ctu_t *other);
ctu_t *ctu_break(scan_t *scan, where_t where);

/* types */
ctu_t *ctu_pointer(scan_t *scan, where_t where, ctu_t *ptr);
ctu_t *ctu_typename(scan_t *scan, where_t where, const char *name);
ctu_t *ctu_typepath(scan_t *scan, where_t where, vector_t *path);
ctu_t *ctu_closure(scan_t *scan, where_t where, vector_t *args, ctu_t *result);
ctu_t *ctu_mutable(scan_t *scan, where_t where, ctu_t *type);
ctu_t *ctu_varargs(scan_t *scan, where_t where);

/* declarations */
ctu_t *ctu_value(scan_t *scan, where_t where, 
                 bool mut, const char *name, 
                 ctu_t *type, ctu_t *value);

ctu_t *ctu_param(scan_t *scan, where_t where,
                 const char *name, ctu_t *type);

ctu_t *ctu_define(scan_t *scan, where_t where, 
                  const char *name, vector_t *params, 
                  ctu_t *result, ctu_t *body);

ctu_t *ctu_newtype(scan_t *scan, where_t where, 
                   const char *name, ctu_t *type);

ctu_t *ctu_attrib(scan_t *scan, where_t where, const char *name, vector_t *params);

/* modules */
ctu_t *ctu_module(scan_t *scan, where_t where, vector_t *imports, vector_t *decls);

ctu_t *ctu_import(scan_t *scan, where_t where, vector_t *path, const char *alias);

ctu_t *set_details(ctu_t *decl, vector_t *attribs, bool exported);
