#include "cthulhu/hlir/debug.h"

typedef struct {
    size_t depth;
    size_t index;
} debug_t;

static void dbg_indent(debug_t *dbg) {
    dbg->depth += 1;
}

static void dbg_dedent(debug_t *dbg) {
    dbg->depth -= 1;
}

static void dbg_reset(debug_t *dbg) {
    dbg->index = 0;
}

static size_t dbg_next(debug_t *dbg) {
    return dbg->index++;
}

static void dbg_line(debug_t *dbg, const char *line) {
    for (size_t i = 0; i < dbg->depth; i++) {
        printf(" ");
    }
    printf("%s\n", line);
}

static const char *hlir_type_name(hlir_type_t type) {
    switch (type) {
    case HLIR_VALUE: return "value";
    case HLIR_FUNCTION: return "function";
    case HLIR_DECLARE: return "declare";
    case HLIR_MODULE: return "module";
    default: return "unknown";
    }
}

static void hlir_emit(debug_t *dbg, const hlir_t *hlir);

static void hlir_emit_vec(debug_t *dbg, vector_t *vec, size_t len) {
    for (size_t i = 0; i < len; i++) {
        hlir_emit(dbg, vector_get(vec, i));
    }
}

static void dbg_section(debug_t *dbg, const char *name, vector_t *vec) {
    size_t len = vector_len(vec);

    if (len == 0) {
        dbg_line(dbg, format("%s(0) {}", name));
        return;
    }

    dbg_reset(dbg);
    dbg_line(dbg, format("%s(%zu) {", name, len));
    dbg_indent(dbg);
        hlir_emit_vec(dbg, vec, len);
    dbg_dedent(dbg);
    dbg_line(dbg, "}");
}

static void hlir_emit_module(debug_t *dbg, const hlir_t *hlir) {
    dbg_line(dbg, format("module(%s) {", hlir->mod));
    dbg_indent(dbg);
        dbg_section(dbg, "imports", hlir->imports);
        dbg_section(dbg, "globals", hlir->globals);
        dbg_section(dbg, "defines", hlir->defines);
    dbg_dedent(dbg);
    dbg_line(dbg, "}");
}

static void hlir_emit_declare(debug_t *dbg, const hlir_t *hlir) {
    size_t index = dbg_next(dbg);
    dbg_line(dbg, format("[%zu]: declare(%s) = %s", index, hlir->name, hlir_type_name(hlir->expect)));
}

static void hlir_emit(debug_t *dbg, const hlir_t *hlir) {
    if (hlir == NULL) {
        return;
    }

    switch (hlir->type) {
    case HLIR_DECLARE:
        hlir_emit_declare(dbg, hlir);
        return;
    case HLIR_MODULE:
        hlir_emit_module(dbg, hlir);
        return;
    default:
        dbg_line(dbg, format("unknown(%d)", (int)hlir->type));
        return;
    }
}

void hlir_debug(const hlir_t *hlir) {
    debug_t debug = {
        .depth = 0,
        .index = 0
    };

    hlir_emit(&debug, hlir);
}
