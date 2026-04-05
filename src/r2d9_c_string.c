/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

#include "r2d9_c_string.h"

const char * R2D9_NONNULL const emptyCString = "";

char * R2D9_NULLABLE r2d9_c_str_copy_non_empty(const char * R2D9_NULLABLE cString) {
    const size_t cStringLength = cString ? strlen(cString) : 0;
    if (cStringLength > 0) {
        char * cStringCopy = (char *)malloc(cStringLength + 1);
        if (cStringCopy) {
            memcpy(cStringCopy, cString, cStringLength);
            cStringCopy[cStringLength] = 0;
            return cStringCopy;
        }
    }
    return NULL;
}

char * R2D9_NULLABLE r2d9_c_str_copy_with_empty(const char * R2D9_NULLABLE cString) {
    if (cString) {
        const size_t cStringLength = strlen(cString);
        char * cStringCopy = (char *)malloc(cStringLength + 1);
        if (cStringCopy) {
            if (cStringLength > 0) {
                memcpy(cStringCopy, cString, cStringLength);
            }
            cStringCopy[cStringLength] = 0;
            return cStringCopy;
        }
    }
    return NULL;
}

void r2d9_c_str_release(char * R2D9_NULLABLE cString) {
    if (cString) {
        free(cString);
    }
}

#include <stdio.h>  // freopen
#include <unistd.h> // close

int r2d9_stdio_to_dev_null(void) {
    if ((close(STDIN_FILENO) != 0) ||
        (close(STDOUT_FILENO) != 0) ||
        (close(STDERR_FILENO) != 0)) {
        return -1;
    }
    const char * dnPath = "/dev/null";
    if (!freopen(dnPath, "r", stdin) ||
        !freopen(dnPath, "w", stdout) ||
        !freopen(dnPath, "w", stderr)) {
        return -2;
    }
    return 0;
}
