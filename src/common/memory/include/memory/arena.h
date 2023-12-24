#pragma once

#include "core/analyze.h"
#include "core/compiler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

BEGIN_API

/// @defgroup Memory Arena memory allocation
/// @brief Global and arena memory management
/// @ingroup Common
/// @{

#ifdef WITH_DOXYGEN
#   define CTU_TRACE_MEMORY 1
#endif

/// @def CTU_TRACE_MEMORY
/// @brief a compile time flag to enable memory tracing
/// @note this is enabled by default in debug builds, see [The build guide](@ref building) for more information

/// @def ALLOC_SIZE_UNKNOWN
/// @brief unknown allocation size constant
/// when freeing or reallocating memory, this can be used as the size
/// to indicate that the size is unknown. requires allocator support
#define ALLOC_SIZE_UNKNOWN SIZE_MAX

typedef struct arena_t arena_t;

/// @brief arena malloc callback
/// @pre @p size must be greater than 0.
///
/// @param header associated event
/// @param size the size of the allocation
///
/// @return the allocated pointer
/// @retval NULL if the allocation failed
typedef void *(*mem_alloc_t)(size_t size, void *user);

/// @brief arena realloc callback
/// @pre @p old_size must be either @ref ALLOC_SIZE_UNKNOWN or the size of the allocation.
/// @pre @p new_size must be greater than 0.
/// @pre @p ptr must not be NULL.
///
/// @param header associated event
/// @param ptr the pointer to reallocate
/// @param new_size the new size of the allocation
/// @param old_size the old size of the allocation
///
/// @return the reallocated pointer
/// @retval NULL if the allocation failed
typedef void *(*mem_resize_t)(void *ptr, size_t new_size, size_t old_size, void *user);

/// @brief arena free callback
/// @pre @p size must be either @ref ALLOC_SIZE_UNKNOWN or the size of the allocation.
///
/// @param header associated event
/// @param ptr the pointer to free
/// @param size the size of the allocation.
typedef void (*mem_release_t)(void *ptr, size_t size, void *user);

/// @brief arena rename callback
///
/// @param header associated event
/// @param ptr the pointer to rename
/// @param name the new name of the pointer
typedef void (*mem_rename_t)(const void *ptr, const char *name, void *user);

/// @brief arena reparent callback
///
/// @param user associated event
/// @param ptr the pointer to reparent
/// @param parent the new parent of the pointer
typedef void (*mem_reparent_t)(const void *ptr, const void *parent, void *user);

/// @brief an allocator object
typedef struct arena_t
{
    /// @brief the name of the allocator
    const char *name;

    /// @brief the malloc function
    mem_alloc_t fn_malloc;

    /// @brief the realloc function
    mem_resize_t fn_realloc;

    /// @brief the free function
    mem_release_t fn_free;

    /// @brief the rename function
    /// @note this feature is optional
    mem_rename_t fn_rename;

    /// @brief the reparent function
    /// @note this feature is optional
    mem_reparent_t fn_reparent;

    /// @brief the user data
    void *user;
} arena_t;

/// @brief release memory from a custom allocator
/// @note ensure the allocator is consistent with the allocator used to allocate @a ptr
///
/// @param arena the allocator to use
/// @param ptr the pointer to free
/// @param size the size of the allocation
void arena_free(
    OUT_PTR_INVALID void *ptr,
    IN_RANGE(!=, 0) size_t size,
    IN_NOTNULL arena_t *arena);

/// @brief allocate memory from a custom allocator
///
/// @param arena the allocator to use
/// @param size the size of the allocation, must be greater than 0
/// @param name the name of the allocation
/// @param parent the parent of the allocation
///
/// @return the allocated pointer
NODISCARD CT_ALLOC(arena_free) CT_ALLOC_SIZE(1)
RET_NOTNULL
void *arena_malloc(
    IN_RANGE(!=, 0) size_t size,
    const char *name,
    const void *parent,
    IN_NOTNULL arena_t *arena);

/// @brief resize a memory allocation from a custom allocator
/// @note ensure the allocator is consistent with the allocator used to allocate @a ptr
///
/// @param arena the allocator to use
/// @param ptr the pointer to reallocate
/// @param new_size the new size of the allocation
/// @param old_size the old size of the allocation
///
/// @return the reallocated pointer
NODISCARD
RET_NOTNULL
void *arena_realloc(
    OUT_PTR_INVALID void *ptr,
    IN_RANGE(!=, 0) size_t new_size,
    IN_RANGE(!=, 0) size_t old_size,
    IN_NOTNULL arena_t *arena);

NODISCARD CT_ALLOC(arena_free)
char *arena_strdup(
    IN_STRING const char *str,
    IN_NOTNULL arena_t *arena);

NODISCARD CT_ALLOC(arena_free)
char *arena_strndup(
    IN_READS(len) const char *str,
    IN_RANGE(>, 0) size_t len,
    IN_NOTNULL arena_t *arena);

NODISCARD CT_ALLOC(arena_free) CT_ALLOC_SIZE(2)
void *arena_memdup(
    IN_READS(size) const void *ptr,
    IN_RANGE(>, 0) size_t size,
    IN_NOTNULL arena_t *arena);

/// @brief rename a pointer in a custom allocator
///
/// @param arena the allocator to use
/// @param ptr the pointer to rename
/// @param name the new name of the pointer
void arena_rename(IN_NOTNULL const void *ptr, IN_STRING const char *name, IN_NOTNULL arena_t *arena);

/// @brief reparent a pointer in a custom allocator
///
/// @param arena the allocator to use
/// @param ptr the pointer to reparent
/// @param parent the new parent of the pointer
void arena_reparent(IN_NOTNULL const void *ptr, const void *parent, IN_NOTNULL arena_t *arena);

/// @def ARENA_RENAME(arena, ptr, name)
/// @brief rename a pointer in a custom allocator
/// @note this is a no-op if @ref CTU_TRACE_MEMORY is not defined
///
/// @param arena the allocator to use
/// @param ptr the pointer to rename
/// @param name the new name of the pointer

/// @def ARENA_REPARENT(arena, ptr, parent)
/// @brief reparent a pointer in a custom allocator
/// @note this is a no-op if @ref CTU_TRACE_MEMORY is not defined
///
/// @param arena the allocator to use
/// @param ptr the pointer to reparent
/// @param parent the new parent of the pointer

/// @def ARENA_MALLOC(arena, size, name, parent)
/// @brief allocate memory from a custom allocator
/// @note this is converted to @ref arena_malloc if @ref CTU_TRACE_MEMORY is not defined
///
/// @param arena the allocator to use
/// @param size the size of the allocation, must be greater than 0
/// @param name the name of the allocation
/// @param parent the parent of the allocation
///
/// @return the allocated pointer

#if CTU_TRACE_MEMORY
#   define ARENA_RENAME(arena, ptr, name) arena_rename(ptr, name, arena)
#   define ARENA_REPARENT(arena, ptr, parent) arena_reparent(ptr, parent, arena)
#   define ARENA_MALLOC(arena, size, name, parent) arena_malloc(size, name, parent, arena)
#else
#   define ARENA_RENAME(arena, ptr, name)
#   define ARENA_REPARENT(arena, ptr, parent)
#   define ARENA_MALLOC(arena, size, name, parent) arena_malloc(size, NULL, NULL, arena)
#endif

/// @def ARENA_IDENTIFY(arena, ptr, name, parent)
/// @brief rename and reparent a pointer in a custom allocator
/// @note this is a no-op if @ref CTU_TRACE_MEMORY is not defined
/// @warning identifying a pointer is not an atomic call and is implemented as
///          two separate calls to @ref ARENA_RENAME and @ref ARENA_REPARENT
///
/// @param arena the allocator to use
/// @param ptr the pointer to rename

#define ARENA_IDENTIFY(arena, ptr, name, parent)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        ARENA_RENAME(arena, ptr, name);                                                                                \
        ARENA_REPARENT(arena, ptr, parent);                                                                            \
    } while (0)

/// @} // Memory Management

END_API
