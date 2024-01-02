#pragma once

#include "core/compiler.h"
#include "core/analyze.h"
#include "core/text.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

BEGIN_API

/// @defgroup Stacktrace Stacktrace library
/// @brief Stacktrace library
/// @ingroup Common
/// @{

/// @brief an address of a symbol
typedef uint_least64_t bt_address_t;

/// @brief a symbol
typedef struct symbol_t
{
    /// @brief the line number
    size_t line;

    /// @brief a buffer to hold the name
    /// when neither @ref eResolveName nor @ref eResolveDemangledName is set this is set to 0
    text_t name;

    /// @brief a buffer to hold the path to the file
    /// when @ref eResolveFile is not set the first character is set to 0
    text_t path;
} symbol_t;

/// @brief a stacktrace frame
typedef struct frame_t
{
    /// @brief the frame address
    bt_address_t address;
} frame_t;

/// @brief how much of a frame was reconstructed
typedef enum frame_resolve_t
{
    /// @brief nothing was resolved
    eResolveNothing         = (0),

    /// @brief the line number was found
    /// @note this does not imply @ref eResolveFile
    eResolveLine            = (1 << 0),

    /// @brief the symbol name was found
    /// @note this does not imply @ref eResolveDemangledName
    eResolveName            = (1 << 1),

    /// @brief the symbol name was demangled
    eResolveDemangledName   = (1 << 2) | eResolveName,

    /// @brief the file path was found
    eResolveFile            = (1 << 3),

    eResolveCount
} frame_resolve_t;

/// @brief user callback for @ref bt_read
typedef void (*bt_frame_t)(void *user, const frame_t *frame);

/// @brief initialize the stacktrace backend
/// @note this function must be called before any other stacktrace function
void bt_init(void);

/// @brief get the stacktrace backend name
/// @return the stacktrace backend name
RET_STRING
const char *bt_backend(void);

/// @brief get a stacktrace from the current location using a callback
/// @note this function is not thread safe
/// @note @ref bt_init must be called before calling this function
///
/// @param callback the callback to call for each frame
/// @param user the user data to pass to the callback
void bt_read(bt_frame_t callback, void *user);

/// @brief resolve a frame to a symbol
///
/// @note @ref bt_init must be called before calling this function
///
/// @param frame the frame to resolve
/// @param symbol the symbol to fill
frame_resolve_t bt_resolve_symbol(IN_NOTNULL const frame_t *frame, symbol_t *symbol);

/// @brief print a stacktrace to a file
/// @note this follows the same precondition as @ref bt_read
///
/// @param file the file to print to
void bt_print_trace(IN_NOTNULL FILE *file);

/// @}

END_API
