#include "common.h"

hlir_t *hlir_digit(const node_t *node, const char *name, digit_t width, sign_t sign) {
    hlir_t *hlir = hlir_new_decl(node, name, NULL, HLIR_DIGIT);
    hlir->width = width;
    hlir->sign = sign;
    return hlir;
}

hlir_t *hlir_bool(const node_t *node, const char *name) {
    return hlir_new_decl(node, name, NULL, HLIR_BOOL);
}

hlir_t *hlir_string(const node_t *node, const char *name) {
    return hlir_new_decl(node, name, NULL, HLIR_STRING);
}

hlir_t *hlir_void(const node_t *node, const char *name) {
    return hlir_new_decl(node, name, NULL, HLIR_VOID);
}

hlir_t *hlir_closure(const node_t *node, const char *name, vector_t *params, hlir_t *result, bool variadic) {
    hlir_t *hlir = hlir_new_decl(node, name, NULL, HLIR_CLOSURE);
    hlir->params = params;
    hlir->result = result;
    hlir->variadic = variadic;
    return hlir;
}

hlir_t *hlir_pointer(const node_t *node, const char *name, hlir_t *ptr, bool indexable) {
    hlir_t *hlir = hlir_new_decl(node, name, NULL, HLIR_POINTER);
    hlir->ptr = ptr;
    hlir->indexable = indexable;
    return hlir;
}
