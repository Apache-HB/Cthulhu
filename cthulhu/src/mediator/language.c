#include "cthulhu/mediator/language.h"

#include "common.h"

#include "base/memory.h"
#include "base/panic.h"

typedef struct lang_handle_t
{
    lifetime_t *parent;
    const language_t *language;

    void *user;
} lang_handle_t;

lang_handle_t *lang_init(lifetime_t *lifetime, const language_t *lang)
{
    CTASSERT(lifetime != NULL);
    CTASSERT(lang != NULL);

    lang_handle_t *handle = ctu_malloc(sizeof(lang_handle_t));
    handle->parent = lifetime;
    handle->language = lang;
    handle->user = NULL;

    EXEC(lang, fnInit, handle);

    return handle;
}

void lang_compile(lang_handle_t *handle, hlir_t *mod)
{
    CTASSERT(handle != NULL);
    CTASSERT(mod != NULL);

    EXEC(handle->language, fnCompile, handle);
}