/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_C_STRING_H__
#define __R2D9_C_STRING_H__ 1

#include <stddef.h>

#include "r2d9.h"

R2D9_C_API_PRIVATE(const char * R2D9_NONNULL const) emptyCString;

R2D9_C_API_PRIVATE(char * R2D9_NULLABLE) r2d9_c_str_copy_non_empty(const char * R2D9_NULLABLE cString);
R2D9_C_API_PRIVATE(char * R2D9_NULLABLE) r2d9_c_str_copy_with_empty(const char * R2D9_NULLABLE cString);
R2D9_C_API_PRIVATE(void) r2d9_c_str_release(char * R2D9_NULLABLE cString);

#endif //!__R2D9_C_STRING_H__
