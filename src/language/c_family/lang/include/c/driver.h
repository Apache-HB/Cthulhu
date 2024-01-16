#pragma once

#include "notify/notify.h" // IWYU pragma: keep

typedef struct driver_t driver_t;
typedef struct context_t context_t;
typedef struct tree_context_t tree_context_t;

void cc_create(driver_t *handle, tree_context_t *tree_context);
void cc_destroy(driver_t *handle);

void cc_forward_decls(context_t *context, tree_context_t *tree_context);
void cc_process_imports(context_t *context, tree_context_t *tree_context);
void cc_compile_module(context_t *context, tree_context_t *tree_context);

#define NEW_EVENT(name, ...) extern const diagnostic_t kEvent_##name;
#include "events.def"
