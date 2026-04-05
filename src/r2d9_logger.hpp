/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_LOGGER_HPP__
#define __R2D9_LOGGER_HPP__ 1

#include <mutex>
#include <chrono>
#include <cstdio>

#include "r2d9.hpp"

namespace r2d9 {

    R2D9_CPP_API_PRIVATE(const char * R2D9_NONNULL const) loggerTypeException;
    R2D9_CPP_API_PRIVATE(const char * R2D9_NONNULL const) loggerTypeInfo;
    R2D9_CPP_API_PRIVATE(const char * R2D9_NONNULL const) loggerTypeCritical;
    
    class Logger final {
    private:
        int _fd = -1;
        
        static void systemLog(const int option,
                              const int priority,
                              const char * R2D9_NONNULL format,
                              va_list vaList) noexcept;
        
    public:
        void log(const char * R2D9_NULLABLE type, const char * R2D9_NONNULL format, ...) const noexcept;
        void log(const std::exception & exception) const noexcept;
        
        void open(const int fd);
        void open(const char * R2D9_NONNULL path);
        
        void close() noexcept;
        
        Logger() noexcept = default;
        ~Logger() noexcept;
        
        static void systemLog(const char * R2D9_NULLABLE type, const char * R2D9_NONNULL format, ...) noexcept;
    };

} // namespace r2d9

#endif //!__R2D9_LOGGER_HPP__
