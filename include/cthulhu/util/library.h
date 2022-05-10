#pragma once

#include "cthulhu/util/defs.h"

typedef void *library_t;

library_t library_open(const char *path, cerror_t *error);
void library_close(library_t library);

void *library_get(library_t library, const char *symbol, cerror_t *error);
