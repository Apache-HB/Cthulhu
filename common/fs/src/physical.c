#include "common.h"

#include "std/str.h"
#include "std/map.h"

#include "io/io.h"

#include "base/panic.h"

typedef struct physical_t
{
    const char *root; ///< absolute path to root directory
} physical_t;

typedef struct physical_file_t
{
    const char *path; ///< path to file relative to root
} physical_file_t;

typedef struct physical_dir_t
{
    const char *path; ///< path to directory relative to root
} physical_dir_t;

static bool is_special(const char *path)
{
    return path == NULL
        || str_equal(path, ".")
        || str_equal(path, "..");
}

static const char *get_absolute(fs_t *fs, inode_t *node, const char *path)
{
    CTASSERT(fs != NULL);

    const physical_t *self = fs_data(fs);
    const physical_dir_t *dir = inode_data(node);

    CTASSERT(!is_special(self->root));

    if (is_special(dir->path) && is_special(path))
    {
        return self->root;
    }

    if (is_special(dir->path) && !is_special(path))
    {
        return format("%s" NATIVE_PATH_SEPARATOR "%s", self->root, path);
    }

    if (!is_special(dir->path) && is_special(path))
    {
        return format("%s" NATIVE_PATH_SEPARATOR "%s", self->root, dir->path);
    }

    return format("%s" NATIVE_PATH_SEPARATOR "%s" NATIVE_PATH_SEPARATOR "%s", self->root, dir->path, path);
}

static const char *get_relative(inode_t *node, const char *path)
{
    const physical_dir_t *dir = inode_data(node);

    if (is_special(dir->path) && !is_special(path))
    {
        return path;
    }

    if (!is_special(dir->path) && is_special(path))
    {
        return dir->path;
    }

    CTASSERT(!is_special(dir->path) && !is_special(path));

    return format("%s" NATIVE_PATH_SEPARATOR "%s", dir->path, path);
}

static inode_t *physical_dir(const char *path, arena_t *arena)
{
    physical_dir_t dir = {
        .path = path
    };

    return inode_dir(&dir, sizeof(physical_dir_t), arena);
}

static inode_t *physical_file(const char *path, arena_t *arena)
{
    physical_file_t file = {
        .path = path
    };

    return inode_file(&file, sizeof(physical_file_t), arena);
}

static inode_t *pfs_query_node(fs_t *fs, inode_t *self, const char *name)
{
    const char *absolute = get_absolute(fs, self, name);
    OS_RESULT(os_dirent_t) dirent = os_dirent_type(absolute);
    CTASSERTF(os_error(dirent) == 0, "failed to query dirent (path=%s, err=%s)", absolute, os_error_string(os_error(dirent)));

    const char *relative = get_relative(self, name);

    switch (OS_VALUE_OR(os_dirent_t, dirent, eOsNodeNone))
    {
    case eOsNodeFile:
        return physical_file(relative, fs->arena);
    case eOsNodeDir:
        return physical_dir(relative, fs->arena);
    default:
        return &gInvalidINode;
    }
}

static map_t *pfs_query_dirents(fs_t *fs, inode_t *self)
{
    const char *absolute = get_absolute(fs, self, NULL);

    OS_RESULT(os_iter_t) iter = os_iter_begin(absolute);
    OS_RESULT(os_dir_t) node = NULL;

    CTASSERTF(iter != NULL, "fs backing corrupted, expected dir `%s` to exist, it is missing", absolute);
    if (os_error(iter)) { return map_new(1); }

    os_iter_t *it = os_value(iter);

    map_t *dirents = map_new(64);

    while ((node = os_iter_next(it)) != NULL)
    {
        if (os_error(node)) { break; }
        os_dir_t *dir = os_value(node);
        const char *path = os_dir_name(dir);

        inode_t *inode = pfs_query_node(fs, self, path);
        CTASSERTF(inode != NULL, "failed to query node %s '%s'", absolute, path);
        map_set(dirents, path, inode);
    }

    os_iter_end(it);

    return dirents;
}

static io_t *pfs_query_file(fs_t *fs, inode_t *self, os_access_t flags)
{
    const char *absolute = get_absolute(fs, self, NULL);
    return io_file(absolute, flags, fs->arena);
}

static inode_t *pfs_file_create(fs_t *fs, inode_t *self, const char *name)
{
    const char *absolute = get_absolute(fs, self, name);
    OS_RESULT(bool) check = os_file_create(absolute);
    CTASSERTF(os_error(check) == 0, "failed to create file `%s` %s", absolute, os_error_string(os_error(check)));

    return physical_file(get_relative(self, name), fs->arena);
}

static inode_t *pfs_dir_create(fs_t *fs, inode_t *self, const char *name)
{
    const char *absolute = get_absolute(fs, self, name);
    OS_RESULT(bool) create = os_dir_create(absolute);
    CTASSERTF(os_error(create) == 0, "failed to create dir `%s` %s", absolute, os_error_string(os_error(create)));

    return physical_dir(get_relative(self, name), fs->arena);
}

static void pfs_dir_delete(fs_t *fs, inode_t *self, const char *name)
{
    const char *absolute = get_absolute(fs, self, name);
    OS_RESULT(bool) check = os_dir_delete(absolute);
    CTASSERT(os_error(check) == 0);
}

static void pfs_file_delete(fs_t *fs, inode_t *self, const char *name)
{
    const char *absolute = get_absolute(fs, self, name);
    OS_RESULT(bool) check = os_file_delete(absolute);
    CTASSERT(os_error(check) == 0);
}

static const fs_callbacks_t kPhysicalInterface = {
    .pfn_query_node = pfs_query_node,
    .pfn_query_dirents = pfs_query_dirents,
    .pfn_query_file = pfs_query_file,

    .pfn_create_dir = pfs_dir_create,
    .pfn_delete_dir = pfs_dir_delete,

    .pfn_create_file = pfs_file_create,
    .pfn_delete_file = pfs_file_delete
};

fs_t *fs_physical(const char *root, arena_t *arena)
{
    CTASSERT(root != NULL);

    OS_RESULT(bool) exist = os_dir_exists(root);
    if (!OS_VALUE_OR(bool, exist, false))
    {
        OS_RESULT(bool) create = mkdir_recursive(root);

        // TODO: make this work recursively
        CTASSERTF(os_error(create) == 0, "error creating root directory: %s. %s", root, os_error_string(os_error(create)));

        if (!OS_VALUE(bool, create))
        {
            return NULL;
        }
    }

    physical_t self = {
        .root = root
    };

    inode_t *dir = physical_dir(".", arena);

    return fs_new(dir, &kPhysicalInterface, &self, sizeof(physical_t), arena);
}
