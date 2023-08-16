#include "oberon/sema/decl.h"
#include "oberon/sema/type.h"
#include "oberon/sema/expr.h"

#include "report/report.h"

#include "base/util.h"
#include "base/panic.h"

static h2_visible_t remap_visibility(reports_t *reports, const node_t *node, obr_visibility_t vis)
{
    switch (vis)
    {
    case eObrVisPrivate: return eVisiblePrivate;
    case eObrVisPublic: return eVisiblePublic;
    case eObrVisPublicReadOnly:
        report(reports, eWarn, node, "public read-only is not yet supported");
        return eVisiblePublic;
    default: NEVER("obr-visibility %d", vis);
    }
}

static h2_link_t remap_linkage(obr_visibility_t vis)
{
    switch (vis)
    {
    case eObrVisPublic:
    case eObrVisPublicReadOnly:
        return eLinkExport;
    default: return eLinkModule;
    }
}

static void set_attribs(h2_t *sema, h2_t *decl, obr_visibility_t vis)
{
    h2_attrib_t attrib = {
        .link = remap_linkage(vis),
        .visibility = remap_visibility(sema->reports, decl->node, vis)
    };

    h2_set_attrib(decl, BOX(attrib));
}

static void resolve_const(h2_cookie_t *cookie, h2_t *sema, h2_t *self, void *user)
{
    obr_t *decl = user;
    CTASSERTF(decl->kind == eObrDeclConst, "decl %s is not a const", decl->name);

    h2_t *expr = obr_sema_rvalue(sema, decl->value, h2_get_type(self));
    h2_close_global(self, expr);
}

static void resolve_var(h2_cookie_t *cookie, h2_t *sema, h2_t *self, void *user)
{
    obr_t *decl = user;
    CTASSERTF(decl->kind == eObrDeclVar, "decl %s is not a var", decl->name);

    // TODO: get default value from type, or maybe noinit

    mpz_t zero;
    mpz_init(zero);
    h2_t *zeroLiteral = h2_expr_digit(decl->node, h2_get_type(self), zero);

    h2_close_global(self, zeroLiteral);
}

static h2_t *forward_const(h2_t *sema, obr_t *decl)
{
    h2_resolve_info_t resolve = {
        .sema = sema,
        .user = decl,
        .fnResolve = resolve_const
    };

    h2_t *type = obr_sema_type(sema, decl->type);
    h2_t *cnt = h2_qualify(decl->node, type, eQualDefault); // make sure it's not mutable
    h2_t *it = h2_open_global(decl->node, decl->name, cnt, resolve);
    set_attribs(sema, it, decl->visibility);

    return it;
}

static h2_t *forward_var(h2_t *sema, obr_t *decl)
{
    h2_resolve_info_t resolve = {
        .sema = sema,
        .user = decl,
        .fnResolve = resolve_var
    };

    h2_t *type = obr_sema_type(sema, decl->type);
    h2_t *mut = h2_qualify(decl->node, type, eQualMutable);
    h2_t *it = h2_open_global(decl->node, decl->name, mut, resolve);
    set_attribs(sema, it, decl->visibility);

    return it;
}

obr_forward_t obr_forward_decl(h2_t *sema, obr_t *decl)
{
    switch (decl->kind)
    {
    case eObrDeclConst: {
        obr_forward_t fwd = {
            .tag = eTagValues,
            .decl = forward_const(sema, decl)
        };
        return fwd;
    }

    case eObrDeclVar: {
        obr_forward_t fwd = {
            .tag = eTagValues,
            .decl = forward_var(sema, decl)
        };
        return fwd;
    }

    default: NEVER("obr-forward-decl %d", decl->kind);
    }
}