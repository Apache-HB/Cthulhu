// SPDX-License-Identifier: LGPL-3.0-only

#include "os/core.h"

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

static long gMaxNameLength = 0;
static long gMaxPathLength = 0;

size_t impl_maxname(void)
{
    return gMaxNameLength;
}

size_t impl_maxpath(void)
{
    return gMaxPathLength;
}

void impl_init(void)
{
    long path = pathconf(".", _PC_PATH_MAX);
    if (path < 0)
    {
        // best guess if pathconf() fails
        // TODO: should there be a way to notify the user that the path length is unknown?
        gMaxPathLength = PATH_MAX;
    }
    else
    {
        gMaxPathLength = path;
    }

    long name = pathconf(".", _PC_NAME_MAX);
    if (name < 0)
    {
        // best guess if pathconf() fails
        gMaxNameLength = NAME_MAX;
    }
    else
    {
        gMaxNameLength = name;
    }
}

void impl_exit(os_exitcode_t code)
{
    exit(code); // NOLINT(concurrency-mt-unsafe)
}

void impl_thread_exit(os_status_t status)
{
    pthread_exit((void*)(intptr_t)status);
}

void impl_abort(void)
{
    abort(); // NOLINT(concurrency-mt-unsafe)
}
