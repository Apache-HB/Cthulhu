// clang-format off
#include "platform/platform.h"
#include "platform/file.h"
// clang-format on

#include "base/panic.h"

USE_DECL
cerror_t make_directory(const char *path)
{
    return native_make_directory(path);
}

USE_DECL
cerror_t delete_file(const char *path)
{
    return native_delete_file(path);
}

static file_format_t get_format(file_flags_t flags)
{
    file_flags_t fmt = flags & (eFileText | eFileBinary);
    CTASSERTM(fmt, "no file format specified");
    CTASSERTM(fmt == eFileText || fmt == eFileBinary, "invalid file format flags");

    return (fmt == eFileText) ? eFormatText : eFormatBinary;
}

static file_mode_t get_mode(file_flags_t flags)
{
    file_flags_t fmt = flags & (eFileRead | eFileWrite);
    CTASSERTM(fmt, "no open mode specified");
    CTASSERTM(fmt == eFileRead || fmt == eFileWrite, "invalid open mode flags");

    return (fmt == eFileRead) ? eModeRead : eModeWrite;
}

USE_DECL
file_t file_open(const char *path, file_flags_t flags, cerror_t *error)
{
    file_format_t fmt = get_format(flags);
    file_mode_t mode = get_mode(flags);

    native_cerror_t nativeError = 0;
    file_handle_t result = native_file_open(path, mode, fmt, &nativeError);
    *error = (cerror_t)nativeError;

    file_t file = {.handle = result, .path = path};

    return file;
}

void file_close(file_t file)
{
    native_file_close(file.handle);
}

USE_DECL
bool file_valid(file_t file)
{
    return file.handle != INVALID_FILE_HANDLE;
}

USE_DECL
size_t file_read(file_t file, void *buffer, size_t size, cerror_t *error)
{
    native_cerror_t nativeError = 0;
    size_t result = (size_t)native_file_read(file.handle, buffer, (file_read_t)size, &nativeError);
    *error = (cerror_t)nativeError;
    return result;
}

USE_DECL
size_t file_write(file_t file, const void *buffer, size_t size, cerror_t *error)
{
    native_cerror_t nativeError = 0;
    size_t result = (size_t)native_file_write(file.handle, buffer, (file_write_t)size, &nativeError);
    *error = (cerror_t)nativeError;
    return result;
}

USE_DECL
size_t file_size(file_t file, cerror_t *error)
{
    native_cerror_t nativeError = 0;
    size_t result = native_file_size(file.handle, &nativeError);
    *error = (cerror_t)nativeError;
    return result;
}

USE_DECL
const void *file_map(file_t file, cerror_t *error)
{
    native_cerror_t nativeError = 0;
    const void *result = native_file_map(file.handle, &nativeError);
    *error = (cerror_t)nativeError;
    return result;
}
