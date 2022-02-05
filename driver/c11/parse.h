#pragma once

#include "lex.h"
#include "cthulhu/ast/ast.h"
#include "cthulhu/util/report.h"
#include "cthulhu/hlir/hlir.h"

void c11_init_types(void);

hlir_t *c11_compile(reports_t *reports, scan_t *scan);
