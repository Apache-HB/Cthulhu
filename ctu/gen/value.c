#include "value.h"

#include "ctu/util/str.h"

value_t *value_of(const type_t *type) {
    value_t *value = ctu_malloc(sizeof(value_t));
    value->type = type;
    return value;
}

value_t *value_poison(const char *msg) {
    return value_of(type_poison(msg));
}

value_t *value_digit(const type_t *type, mpz_t digit) {
    value_t *value = value_of(type);
    mpz_init_set(value->digit, digit);
    return value;
}

value_t *value_int(const type_t *type, int digit) {
    value_t *value = value_of(type);
    mpz_init_set_si(value->digit, digit);
    return value;
}

value_t *value_ptr(const type_t *type, value_t *ptr) {
    value_t *value = value_of(type);
    value->ptr = ptr;
    return value;
}

value_t *value_empty(void) {
    return value_of(type_void());
}

char *value_format(const value_t *value) {
    const type_t *type = value->type;

    if (is_void(type)) {
        return ctu_strdup("empty");
    }

    char *typestr = type_format(type);

    if (is_digit(type)) {
        char *str = mpz_get_str(NULL, 10, value->digit);
        return format("%s(%s)", typestr, str);
    }

    if (is_poison(type)) {
        return typestr;
    }

    /* trigraphs are fun */
    return format("%s(\?\?\?)", typestr);
}

void value_delete(value_t *value) {
    const type_t *type = value->type;

    if (is_digit(type)) {
        mpz_clear(value->digit);
    } else if (is_pointer(type)) {
        value_delete(value->ptr);
    }

    ctu_free(value, sizeof(value_t));
}
