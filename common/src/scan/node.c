#include "scan/node.h"

#include "common.h"

#include "base/macros.h"
#include "base/panic.h"
#include "base/memory.h"

#include <limits.h>

typedef struct node_t
{
    scan_t *scan;  ///< the source file
    where_t where; ///< the location of this node in the source file
} node_t;

node_t *node_builtin(void)
{
    return (void*)UINTPTR_MAX;
}

node_t *node_invalid(void)
{
    return NULL;
}

bool node_is_valid(const node_t *node)
{
    return node != node_invalid();
}

node_t *node_new(scan_t *scan, where_t where)
{
    if (scan == NULL)
    {
        return NULL;
    }

    node_t *node = arena_malloc(scan->config.nodeAlloc, sizeof(node_t), "node-new");
    node->scan = scan;
    node->where = where;

    return node;
}

scan_t *get_node_scanner(const node_t *node)
{
    return node != NULL ? node->scan : NULL;
}

where_t get_node_location(const node_t *node)
{
    CTASSERT(node != NULL);
    return node->where;
}
