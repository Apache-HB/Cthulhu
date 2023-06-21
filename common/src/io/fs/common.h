#pragma once

#include "io/fs.h"
#include <stddef.h>

typedef struct map_t map_t;

typedef enum inode_type_t
{
    eNodeFile,
    eNodeDir,
    eNodeInvalid,

    eNodeTotal
} inode_type_t;

typedef struct inode_t
{
    inode_type_t type;

    char data[];
} inode_t;

typedef inode_t *(*fs_query_node_t)(fs_t *fs, inode_t *node, const char *name);
typedef map_t *(*fs_query_dirents_t)(fs_t *fs, inode_t *node);
typedef io_t *(*fs_query_file_t)(fs_t *fs, inode_t *node, os_access_t flags);

typedef inode_t *(*fs_dir_create_t)(fs_t *fs, inode_t *node, const char *name);
typedef inode_t *(*fs_file_create_t)(fs_t *fs, inode_t *node, const char *name);

typedef void (*fs_dir_delete_t)(fs_t *fs, inode_t *node, const char *name);
typedef void (*fs_file_delete_t)(fs_t *fs, inode_t *node, const char *name);

typedef struct fs_interface_t
{
    fs_query_node_t fnQueryNode;
    fs_query_dirents_t fnQueryDirents;
    fs_query_file_t fnQueryFile;

    fs_dir_create_t fnCreateDir;
    fs_dir_delete_t fnDeleteDir;

    fs_file_create_t fnCreateFile;
    fs_file_delete_t fnDeleteFile;
} fs_interface_t;

typedef struct fs_t 
{
    const fs_interface_t *cb; ///< callbacks
    reports_t *reports; ///< reports
    inode_t *root; ///< root inode

    char data[];
} fs_t;

// inode api

extern inode_t kInvalidINode;

inode_t *inode_file(const void *data, size_t size);
inode_t *inode_dir(const void *data, size_t size);
void *inode_data(inode_t *inode);
bool inode_is(inode_t *inode, inode_type_t type);

// fs api

fs_t *fs_new(reports_t *reports, inode_t *root, const fs_interface_t *cb, const void *data, size_t size);

void *fs_data(fs_t *fs);
