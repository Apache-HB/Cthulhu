// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include <ctu_config.h>

#ifdef __cpp_lib_unreachable
#   include <utility>
#endif

/// @defgroup compiler Compiler specific macros
/// @brief Compiler detection macros and compiler specific functionality
/// @ingroup core
/// @{

#if defined(__has_cpp_attribute)
#   define CT_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#   define CT_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if defined(__has_c_attribute)
#   define CT_HAS_C_ATTRIBUTE(x) __has_c_attribute(x)
#else
#   define CT_HAS_C_ATTRIBUTE(x) 0
#endif

#if defined(__has_include)
#   define CT_HAS_INCLUDE(x) __has_include(x)
#else
#   define CT_HAS_INCLUDE(x) 0
#endif

// always detect clang first because it pretends to be gcc and msvc
// note: i hate that clang does this
#if defined(__clang__)
#   define CT_CC_CLANG 1
#elif defined(__GNUC__)
#   define CT_CC_GNU 1
#elif defined(_MSC_VER)
#   define CT_CC_MSVC 1
#else
#   error "unknown compiler"
#endif

#if defined(__clang__) && defined(_MSC_VER)
#   define CT_CC_CLANGCL 1
#endif

#if CT_CC_CLANG
#   define CT_CLANG_PRAGMA(x) _Pragma(#x)
#elif CT_CC_GNU
#   define CT_GNU_PRAGMA(x) _Pragma(#x)
#elif CT_CC_MSVC
#   define CT_MSVC_PRAGMA(x) __pragma(x)
#endif

#ifndef CT_CLANG_PRAGMA
#   define CT_CLANG_PRAGMA(x)
#endif

#ifndef CT_GNU_PRAGMA
#   define CT_GNU_PRAGMA(x)
#endif

#ifndef CT_MSVC_PRAGMA
#   define CT_MSVC_PRAGMA(x)
#endif

#if defined(__linux__)
#   define CT_OS_LINUX 1
#elif defined(_WIN32)
#   define CT_OS_WINDOWS 1
#elif defined(__APPLE__)
#   define CT_OS_APPLE 1
#elif defined(__EMSCRIPTEN__)
#   define CT_OS_WASM 1
#else
#   error "unknown platform"
#endif

#ifdef __cplusplus
#   define CT_NORETURN [[noreturn]] void
#endif

#ifndef CT_NORETURN
#   if CT_CC_CLANG || CT_CC_GNU
#      define CT_NORETURN __attribute__((noreturn)) void
#   elif CT_CC_MSVC
#      define CT_NORETURN __declspec(noreturn) void
#   else
#      define CT_NORETURN _Noreturn void
#   endif
#endif

#ifdef CT_OS_WINDOWS
#   define CT_NATIVE_PATH_SEPARATOR "\\"
#   define CT_PATH_SEPERATORS "\\/"
#else
#   define CT_NATIVE_PATH_SEPARATOR "/"
#   define CT_PATH_SEPERATORS "/"
#endif

/// @def CT_UNREACHABLE()
/// @brief mark a point in code as unreachable

#if __cpp_lib_unreachable
#   define CT_UNREACHABLE() std::unreachable()
#elif defined(CT_CC_MSVC)
#   define CT_UNREACHABLE() __assume(0)
#elif defined(CT_CC_GNU) || defined(CT_CC_CLANG)
#   define CT_UNREACHABLE() __builtin_unreachable()
#else
#   define CT_UNREACHABLE() ((void)0)
#endif

/// @def CT_ASSUME(expr)
/// @brief assume that @a expr is true
/// @warning this is a compiler hint that can be used to optimize code
///       use with caution

#if CT_HAS_CPP_ATTRIBUTE(assume)
#   define CT_ASSUME(expr) [[assume(expr)]]
#elif defined(CT_CC_MSVC)
#   define CT_ASSUME(expr) __assume(expr)
#elif defined(CT_CC_CLANG)
#   define CT_ASSUME(expr) __builtin_assume(expr)
#elif defined(CT_CC_GNU)
#   define CT_ASSUME(expr) __attribute__((assume(expr)))
#else
#   define CT_ASSUME(expr) do { if (!(expr)) { CT_UNREACHABLE(); } } while (0)
#endif

// clang-format off
#ifdef __cplusplus
#    define CT_BEGIN_API extern "C"  {
#    define CT_END_API }
#else
#    define CT_BEGIN_API
#    define CT_END_API
#endif
// clang-format on

/// @def CT_FUNCNAME
/// @brief the name of the current function
/// @warning the format of the string is compiler dependant, please dont try and parse it

#if CT_CC_GNU && CTU_HAS_PRETTY_FUNCTION
#   define CT_FUNCNAME __PRETTY_FUNCTION__
#endif

#ifndef CT_FUNCNAME
#   define CT_FUNCNAME __func__
#endif

// byteswapping
#if CT_CC_MSVC
#   define CT_BSWAP_U16(x) _byteswap_ushort(x)
#   define CT_BSWAP_U32(x) _byteswap_ulong(x)
#   define CT_BSWAP_U64(x) _byteswap_uint64(x)
#else
#   define CT_BSWAP_U16(x) __builtin_bswap16(x)
#   define CT_BSWAP_U32(x) __builtin_bswap32(x)
#   define CT_BSWAP_U64(x) __builtin_bswap64(x)
#endif

#ifdef __cplusplus
#   define CT_LINKAGE_C extern "C"
#else
#   define CT_LINKAGE_C
#endif

// we use _MSC_VER rather than CT_CC_MSVC because both clang-cl and msvc define it
// and we want to detect both
#ifdef _MSC_VER
#   define CT_EXPORT __declspec(dllexport)
#   define CT_IMPORT __declspec(dllimport)
#   define CT_LOCAL
#elif __GNUC__ >= 4
#   define CT_EXPORT __attribute__((visibility("default")))
#   define CT_IMPORT
#   define CT_LOCAL __attribute__((visibility("internal")))
#else
#   define CT_EXPORT
#   define CT_IMPORT
#   define CT_LOCAL
#endif

#ifdef __cplusplus
#   define CT_ENUM_FLAGS(X, T) \
    constexpr X operator|(X lhs, X rhs) { return X((T)rhs | (T)lhs); } \
    constexpr X operator&(X lhs, X rhs) { return X((T)rhs & (T)lhs); } \
    constexpr X operator^(X lhs, X rhs) { return X((T)rhs ^ (T)lhs); } \
    constexpr X operator~(X rhs)        { return X(~(T)rhs); } \
    constexpr X& operator|=(X& lhs, X rhs) { return lhs = lhs | rhs; } \
    constexpr X& operator&=(X& lhs, X rhs) { return lhs = lhs & rhs; } \
    constexpr X& operator^=(X& lhs, X rhs) { return lhs = lhs ^ rhs; }
#else
#   define CT_ENUM_FLAGS(X, T)
#endif

/// @}
