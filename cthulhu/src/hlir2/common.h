#pragma once

#include "cthulhu/hlir2/h2.h"

typedef struct h2_cookie_t {
    h2_t *parent;
    vector_t *stack;
} h2_cookie_t;

h2_t *h2_new(h2_kind_t kind, const node_t *node, const h2_t *type);
h2_t *h2_decl(h2_kind_t kind, const node_t *node, const h2_t *type, const char *name);
