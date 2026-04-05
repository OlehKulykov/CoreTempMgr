/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_HPP__
#define __R2D9_HPP__ 1

#include <cstdint>      // INT_MAX|UINT_MIN, ...
#include <cinttypes>    // PRIi..., PRIu...

#include "r2d9.h"

#if defined(DEBUG)
#  include <iostream>   // std::cout
#endif

R2D9_C_API_PRIVATE(int) r2d9_stdio_to_dev_null(void);

#endif //!__R2D9_HPP__
