#include "compile.h"
#include "interop.h"

#include "ctu/util/util.h"
#include "ctu/util/report.h"
#include "ctu/util/macros.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

static scan_t scan_new(reports_t *reports, const char *language, path_t *path) {
    scan_t scan = {
        .language = language,
        .path = path,
        .reports = reports
    };

    return scan;
}

scan_t scan_string(reports_t *reports, const char *language, path_t *path, const char *text) {
    text_t source = { .size = strlen(text), .text = text };
    scan_t scan = scan_new(reports, language, path);

    scan.source = source;

    return scan;
}

scan_t scan_file(reports_t *reports, const char *language, file_t *file) {
    FILE *fd = file->file;
    size_t size = file_size(file);
    scan_t scan = scan_new(reports, language, file->path);
    scan.data = fd;

    char *text;
    if (!(text = file_map(file))) {
        ctu_assert(reports, "failed to mmap file");
    }
    text_t source = { .size = size, .text = text };
    scan.source = source;

    return scan;
}

void scan_export(scan_t *scan, void *data) {
    scan->data = data;
}

void *compile_string(scan_t *extra, callbacks_t *callbacks) {
    int err;
    void *scanner;
    void *state;

    if ((err = callbacks->init(extra, &scanner))) {
        ctu_assert(extra->reports, "failed to init parser for %s due to %d", scan_path(extra), err);
        return NULL;
    }

    if (!(state = callbacks->scan(scan_text(extra), scanner))) {
        report(extra->reports, ERROR, NULL, "failed to scan %s", scan_path(extra));
        return NULL;
    }

    if ((err = callbacks->parse(scanner, extra))) {
        report(extra->reports, ERROR, NULL, "failed to parse %s", scan_path(extra));
        return NULL;
    }

    callbacks->destroy(scanner);

    return extra->data;
}

void *compile_file(scan_t *scan, callbacks_t *callbacks) {
    FILE *fd = scan->data;

    int err;
    void *state;

    if ((err = callbacks->init(scan, &state))) {
        return NULL;
    }

    callbacks->set_in(fd, state);

    if ((err = callbacks->parse(scan, state))) {
        return NULL;
    }

    callbacks->destroy(state);

    return scan->data;
}
