#include "cthulhu/hlir/decl.h"
#include "cthulhu/hlir/query.h"

#include "base/panic.h"

#include "common.h"

#define IS_AGGREGATE(hlir) (hlir_is(hlir, eHlirStruct) || hlir_is(hlir, eHlirUnion))

///
/// builder functions
///

static hlir_t *hlir_begin_aggregate_with_fields(node_t *node, const char *name, vector_t *fields, hlir_kind_t type)
{
    hlir_t *self = hlir_decl_new(node, name, kMetaType, type);
    self->fields = fields;
    return self;
}

static hlir_t *hlir_begin_aggregate(node_t *node, const char *name, hlir_kind_t type)
{
    return hlir_begin_aggregate_with_fields(node, name, vector_new(4), type);
}

///
/// struct interface
///

hlir_t *hlir_begin_struct(node_t *node, const char *name)
{
    return hlir_begin_aggregate(node, name, eHlirStruct);
}

hlir_t *hlir_struct(node_t *node, const char *name, vector_t *fields)
{
    hlir_t *self = hlir_begin_aggregate_with_fields(node, name, fields, eHlirStruct);
    return self;
}

///
/// union interface
///

hlir_t *hlir_begin_union(node_t *node, const char *name)
{
    return hlir_begin_aggregate(node, name, eHlirUnion);
}

hlir_t *hlir_union(node_t *node, const char *name, vector_t *fields)
{
    hlir_t *self = hlir_begin_aggregate_with_fields(node, name, fields, eHlirUnion);
    return self;
}

///
/// generic aggregate interface
///

void hlir_add_field(hlir_t *self, hlir_t *field)
{
    CTASSERT(self != NULL);
    CTASSERTM(IS_AGGREGATE(self), "hlir-add-field called on non-aggregate hlir");
    CTASSERTM(hlir_is(field, eHlirRecordField), "hlir-add-field called with non-field hlir");

    vector_push(&self->fields, field);
}

///
/// alias interface
///

hlir_t *hlir_begin_alias(node_t *node, const char *name)
{
    return hlir_decl_new(node, name, kMetaType, eHlirAlias);
}

void hlir_build_alias(hlir_t *self, const hlir_t *alias, bool newtype)
{
    CTASSERT(hlir_is(self, eHlirAlias));
    self->alias = alias;
    self->newtype = newtype;
}

hlir_t *hlir_alias(node_t *node, const char *name, const hlir_t *type, bool newtype)
{
    hlir_t *self = hlir_begin_alias(node, name);
    hlir_build_alias(self, type, newtype);
    return self;
}

hlir_t *hlir_begin_global(node_t *node, const char *name, const hlir_t *type)
{
    return hlir_decl_new(node, name, type, eHlirGlobal);
}

void hlir_build_global(hlir_t *self, const hlir_t *init)
{
    CTASSERT(hlir_is(self, eHlirGlobal));
    self->value = init;
}

hlir_t *hlir_global(node_t *node, const hlir_attributes_t *attribs, const char *name, const hlir_t *type, const hlir_t *init)
{
    hlir_t *self = hlir_begin_global(node, name, type);
    hlir_build_global(self, init);
    hlir_set_attributes(self, attribs);
    return self;
}

hlir_t *hlir_local(node_t *node, const char *name, const hlir_t *type)
{
    return hlir_decl_new(node, name, type, eHlirLocal);
}

hlir_t *hlir_param(node_t *node, const char *name, const hlir_t *type)
{
    return hlir_decl_new(node, name, type, eHlirParam);
}

void hlir_build_function(hlir_t *self, hlir_t *body)
{
    CTASSERT(hlir_is(self, eHlirFunction));
    self->body = body;
}

static hlir_t *hlir_begin_function_with_locals(node_t *node, const char *name, vector_t *locals, signature_t signature)
{
    hlir_t *self = hlir_decl_new(node, name, kMetaType, eHlirFunction);
    self->params = signature.params;
    self->result = signature.result;
    self->variadic = signature.variadic;
    self->locals = locals;
    self->of = self; // TODO: this should be a closure
    return self;
}

hlir_t *hlir_begin_function(node_t *node, const char *name, signature_t signature)
{
    return hlir_begin_function_with_locals(node, name, vector_new(4), signature);
}

hlir_t *hlir_function(node_t *node, const char *name, signature_t signature, vector_t *locals, hlir_t *body)
{
    hlir_t *self = hlir_begin_function_with_locals(node, name, locals, signature);
    hlir_build_function(self, body);
    return self;
}

void hlir_add_local(hlir_t *self, hlir_t *local)
{
    CTASSERTM(hlir_is(self, eHlirFunction), "hlir-add-local called on non-function hlir");
    vector_push(&self->locals, local);
}

hlir_t *hlir_begin_module(node_t *node, const char *name)
{
    return hlir_decl_new(node, name, NULL, eHlirModule);
}

void hlir_update_module(hlir_t *self, vector_t *types, vector_t *globals, vector_t *functions)
{
    self->types = types;
    self->globals = globals;
    self->functions = functions;
}

void hlir_build_module(hlir_t *self, vector_t *types, vector_t *globals, vector_t *functions)
{
    CTASSERT(hlir_is(self, eHlirModule));

    self->types = types;
    self->globals = globals;
    self->functions = functions;
}

hlir_t *hlir_module(node_t *node, const char *name, vector_t *types, vector_t *globals, vector_t *functions)
{
    hlir_t *self = hlir_begin_module(node, name);
    hlir_build_module(self, types, globals, functions);
    return self;
}

void hlir_set_type(hlir_t *self, const hlir_t *type)
{
    self->of = type;
}
