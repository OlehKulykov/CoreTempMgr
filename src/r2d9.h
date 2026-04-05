/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_H__
#define __R2D9_H__ 1

#include <stdint.h>

#if !defined(DEBUG) && defined(_DEBUG)
#  define DEBUG 1
#endif

#if defined(DEBUG) && defined(NDEBUG)
#  undef DEBUG
#endif

// C / C++ extern
#if defined(__cplusplus)
#  define R2D9_C_EXTERN extern "C"
#  define R2D9_CPP_EXTERN extern
#else
#  define R2D9_C_EXTERN extern
#  define R2D9_CPP_EXTERN extern "C++"
#endif

// Attribute
#if defined(__GNUC__) && (__GNUC__ >= 4)
#  define R2D9_ATTRIB __attribute__((visibility("default")))
#  define R2D9_ATTRIB_PRIVATE __attribute__((visibility("hidden")))
#else
#  define R2D9_ATTRIB
#  define R2D9_ATTRIB_PRIVATE
#endif

// Combined lib API
#define R2D9_C_API(RETURN_TYPE) R2D9_C_EXTERN R2D9_ATTRIB RETURN_TYPE
#define R2D9_C_API_PRIVATE(RETURN_TYPE) R2D9_C_EXTERN R2D9_ATTRIB_PRIVATE RETURN_TYPE
#define R2D9_CPP_API(RETURN_TYPE) R2D9_CPP_EXTERN R2D9_ATTRIB RETURN_TYPE
#define R2D9_CPP_API_PRIVATE(RETURN_TYPE) R2D9_CPP_EXTERN R2D9_ATTRIB_PRIVATE RETURN_TYPE
#define R2D9_CPP_CLASS_API R2D9_ATTRIB
#define R2D9_CPP_CLASS_API_PRIVATE R2D9_ATTRIB_PRIVATE

#if defined(__APPLE__) && defined(__clang__)
#  define R2D9_NULLABLE __nullable
#  define R2D9_NONNULL __nonnull
#else
#  define R2D9_NULLABLE
#  define R2D9_NONNULL
#endif

#if !defined(__has_include)
#  define __has_include(x) 0
#endif

#endif //!__R2D9_H__
