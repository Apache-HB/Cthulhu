#include "scan.h"

void ctuerror(where_t *where, void *state, scan_t *scan, const char *msg) {
    UNUSED(state);

    report(scan->reports, ERROR, node_new(scan, *where), "%s", msg);
}
