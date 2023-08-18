#include "ctu/sema/decl.h"
#include "ctu/sema/type.h"
#include "ctu/sema/expr.h"

#include "cthulhu/tree/query.h"

#include "std/vector.h"
#include "std/str.h"

#include "base/panic.h"

#include <string.h>

///
/// attributes
///

static const attribs_t kAttribPrivate = {
    .link = eLinkModule,
    .visibility = eVisiblePrivate
};

static const attribs_t kAttribExport = {
    .link = eLinkExport,
    .visibility = eVisiblePublic
};

///
/// decl resolution
///

static void ctu_resolve_global(cookie_t *cookie, tree_t *sema, tree_t *self, void *user)
{
    ctu_t *decl = user;
    CTASSERTF(decl->kind == eCtuDeclGlobal, "decl %s is not a global", decl->name);

    tree_t *type = decl->type == NULL ? NULL : ctu_sema_type(sema, decl->type);
    tree_t *expr = decl->value == NULL ? NULL : ctu_sema_rvalue(sema, decl->value, type);

    CTASSERT(expr != NULL || type != NULL);

    self->type = type == NULL ? tree_get_type(expr) : type;
    tree_close_global(self, expr);
}

static void ctu_resolve_function(cookie_t *cookie, tree_t *sema, tree_t *self, void *user)
{
    ctu_t *decl = user;
    CTASSERTF(decl->kind == eCtuDeclFunction, "decl %s is not a function", decl->name);

    tree_t *body = ctu_sema_stmt(sema, self, decl->body);
    tree_close_function(self, body);
}

static void ctu_resolve_type(cookie_t *cookie, tree_t *sema, tree_t *self, void *user)
{
    ctu_t *decl = user;
    CTASSERTF(decl->kind == eCtuDeclTypeAlias, "decl %s is not a type alias", decl->name);
    CTASSERTF(decl->type != NULL, "decl %s has no type", decl->name);

    tree_t *temp = tree_resolve(tree_get_cookie(sema), ctu_sema_type(sema, decl->typeAlias)); // TODO: doesnt support newtypes, also feels icky
    tree_close_decl(self, temp);
}

static void ctu_resolve_struct(cookie_t *cookie, tree_t *sema, tree_t *self, void *user)
{
    ctu_t *decl = user;
    CTASSERTF(decl->kind == eCtuDeclStruct, "decl %s is not a struct", decl->name);

    size_t len = vector_len(decl->fields);
    vector_t *items = vector_of(len);
    for (size_t i = 0; i < len; i++)
    {
        ctu_t *field = vector_get(decl->fields, i);
        tree_t *type = ctu_sema_type(sema, field->fieldType);
        char *name = field->name == NULL ? format("field%zu", i) : field->name;
        tree_t *item = tree_decl_field(field->node, name, type);

        vector_set(items, i, item);
    }

    tree_close_struct(self, items);
}

/* TODO: set visibility inside forwarding */

///
/// forward declarations
///

static tree_t *ctu_forward_global(tree_t *sema, ctu_t *decl)
{
    CTASSERTF(decl->kind == eCtuDeclGlobal, "decl %s is not a global", decl->name);
    CTASSERTF(decl->type != NULL || decl->value != NULL, "decl %s has no type and no init expr", decl->name);

    tree_resolve_info_t resolve = {
        .sema = sema,
        .user = decl,
        .fnResolve = ctu_resolve_global
    };

    tree_t *type = decl->type == NULL ? NULL : ctu_sema_type(sema, decl->type);
    tree_t *global = tree_open_global(decl->node, decl->name, type, resolve);

    return global;
}

static tree_t *ctu_forward_function(tree_t *sema, ctu_t *decl)
{
    CTASSERTF(decl->kind == eCtuDeclFunction, "decl %s is not a function", decl->name);

    tree_resolve_info_t resolve = {
        .sema = sema,
        .user = decl,
        .fnResolve = ctu_resolve_function
    };

    tree_t *returnType = ctu_sema_type(sema, decl->returnType);
    tree_t *signature = tree_type_closure(decl->node, decl->name, returnType, vector_of(0), eArityFixed);

    return tree_open_function(decl->node, decl->name, signature, resolve);
}

static tree_t *ctu_forward_type(tree_t *sema, ctu_t *decl)
{
    CTASSERTF(decl->kind == eCtuDeclTypeAlias, "decl %s is not a type alias", decl->name);

    tree_resolve_info_t resolve = {
        .sema = sema,
        .user = decl,
        .fnResolve = ctu_resolve_type
    };

    return tree_open_decl(decl->node, decl->name, resolve);
}

static tree_t *ctu_forward_struct(tree_t *sema, ctu_t *decl)
{
    CTASSERTF(decl->kind == eCtuDeclStruct, "decl %s is not a struct", decl->name);

    tree_resolve_info_t resolve = {
        .sema = sema,
        .user = decl,
        .fnResolve = ctu_resolve_struct
    };

    return tree_open_struct(decl->node, decl->name, resolve);
}

static ctu_forward_t forward_decl_inner(tree_t *sema, ctu_t *decl)
{
    switch (decl->kind)
    {
    case eCtuDeclGlobal: {
        ctu_forward_t fwd = {
            .tag = eCtuTagValues,
            .decl = ctu_forward_global(sema, decl),
        };
        return fwd;
    }
    case eCtuDeclFunction: {
        ctu_forward_t fwd = {
            .tag = eCtuTagFunctions,
            .decl = ctu_forward_function(sema, decl),
        };
        return fwd;
    }
    case eCtuDeclTypeAlias: {
        ctu_forward_t fwd = {
            .tag = eCtuTagTypes,
            .decl = ctu_forward_type(sema, decl)
        };
        return fwd;
    }
    case eCtuDeclStruct: {
        ctu_forward_t fwd = {
            .tag = eCtuTagTypes,
            .decl = ctu_forward_struct(sema, decl)
        };
        return fwd;
    }
    default:
        NEVER("invalid decl kind %d", decl->kind);
    }
}

ctu_forward_t ctu_forward_decl(tree_t *sema, ctu_t *decl)
{
    ctu_forward_t fwd = forward_decl_inner(sema, decl);

    tree_set_attrib(fwd.decl, decl->exported ? &kAttribExport : &kAttribPrivate);

    return fwd;
}
