#define I_WILL_BE_INCLUDING_PLATFORM_CODE

#include "platform.h"

#include "cthulhu/util/str.h"
#include "platform.h"
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

library_handle_t native_library_open(const char *path, native_cerror_t *error)
{
    library_handle_t handle = dlopen(path, RTLD_LAZY);

    if (handle == NULL)
    {
        *error = native_get_last_error();
    }

    return handle;
}

void native_library_close(library_handle_t handle)
{
    dlclose(handle);
}

void *native_library_get_symbol(library_handle_t handle, const char *symbol, native_cerror_t *error)
{
    void *ptr = dlsym(handle, symbol);

    if (ptr == NULL)
    {
        *error = native_get_last_error();
    }

    return ptr;
}

static const char *kOpenModes[MODE_TOTAL][FORMAT_TOTAL] = {
    [MODE_READ] = {[FORMAT_TEXT] = "r", [FORMAT_BINARY] = "rb"},
    [MODE_WRITE] = {[FORMAT_TEXT] = "w", [FORMAT_BINARY] = "wb"},
};

file_handle_t native_file_open(const char *path, file_mode_t mode, file_format_t format, native_cerror_t *error)
{
    file_handle_t handle = fopen(path, kOpenModes[mode][format]);

    if (handle == NULL)
    {
        *error = native_get_last_error();
    }

    return handle;
}

void native_file_close(file_handle_t handle)
{
    fclose(handle);
}

file_read_t native_file_read(file_handle_t handle, void *buffer, file_read_t size, native_cerror_t *error)
{
    size_t read = fread(buffer, 1, size, handle);

    if (read != size)
    {
        *error = native_get_last_error();
    }

    return read;
}

file_write_t native_file_write(file_handle_t handle, const void *buffer, file_size_t size, native_cerror_t *error)
{
    file_write_t written = fwrite(buffer, 1, size, handle);

    if (written != size)
    {
        *error = native_get_last_error();
    }

    return written;
}

file_size_t native_file_size(file_handle_t handle, native_cerror_t *error)
{
    struct stat st;
    int fd = fileno(handle);

    if (fstat(fd, &st) != 0)
    {
        *error = native_get_last_error();
        return 0;
    }

    return st.st_size;
}

const void *native_file_map(file_handle_t handle, native_cerror_t *error)
{
    file_size_t size = native_file_size(handle, error);
    if (*error != 0)
    {
        return NULL;
    }

    int fd = fileno(handle);
    void *ptr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (ptr == MAP_FAILED)
    {
        *error = native_get_last_error();
        return NULL;
    }

    return ptr;
}

char *native_cerror_to_string(native_cerror_t error)
{
    char buffer[256];
    int result = strerror_r(error, buffer, sizeof(buffer));

    if (result != 0)
    {
        return format("unknown error %d", error);
    }

    return ctu_strdup(buffer);
}

native_cerror_t native_get_last_error(void)
{
    return errno;
}
