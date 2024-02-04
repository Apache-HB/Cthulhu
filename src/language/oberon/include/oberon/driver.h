#pragma once

#include "notify/diagnostic.h" // IWYU pragma: export

#define NEW_EVENT(id, ...) extern const diagnostic_t kEvent_##id;
#include "events.def"

typedef struct driver_t driver_t;
typedef struct context_t context_t;
typedef struct tree_context_t tree_context_t;

void obr_create(driver_t *driver);

void obr_forward_decls(context_t *context);
void obr_process_imports(context_t *context);
void obr_compile_module(context_t *context);
