/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#include <memory>
#include <cstring>
#include <cstdarg>
#include <climits>
#include <ctime>
#include <stdexcept>

#include "r2d9_logger.hpp"
#include "r2d9_fixed_string_stream.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>

namespace r2d9 {
    
    const char * R2D9_NONNULL const loggerTypeException     = "EXC";
    const char * R2D9_NONNULL const loggerTypeInfo          = "INF";
    const char * R2D9_NONNULL const loggerTypeCritical      = "CRIT";
    
    void Logger::log(const std::exception & exception) const noexcept {
        const char * what = exception.what();
        if (what) {
            log(loggerTypeException, what);
        }
    }
    
    void Logger::log(const char * R2D9_NULLABLE type, const char * R2D9_NONNULL format, ...) const noexcept {
        const auto now = std::chrono::system_clock::now();
        if (_fd >= 0) {
            FixedStringStream<63, false> stream; // 64 == bufferCapacity()
            stream << '[';
            stream.append(now, (TimePointFormatLocal | TimePointFormatSpaceSep)) << ']' << ' ';
            
            bool isCritical = false;
            if (type) {
                isCritical = (::strcmp(type, loggerTypeCritical) == 0);
                stream << ' ' << type << ' ';
                if (!isCritical) {
                    stream << ' ';
                }
            }
            
            ssize_t wrRes = ::write(_fd, static_cast<const char *>(stream), stream.length());
            {
                va_list vaList;
                va_start(vaList, format);
                wrRes = ::vdprintf(_fd, format, vaList);
                va_end(vaList);
            }
            wrRes = ::write(_fd, "\n", 1);
            (void)wrRes;
            
            if (type && isCritical) {
                va_list vaList;
                va_start(vaList, format);
                Logger::systemLog(static_cast<int>(LOG_NDELAY | LOG_PERROR), static_cast<int>(LOG_ALERT | LOG_CRIT), format, vaList);
                va_end(vaList);
            }
        }
    }
    
    void Logger::open(const int fd) {
        close();
        if ((fd >= 0) && (::lseek(fd, 0, SEEK_END) >= 0)) {
            _fd = fd;
        } else {
            if (fd >= 0) {
                ::close(fd);
            }
            char reason[128];
            ::snprintf(reason, 128, "Invalid file descriptor: %i", fd);
            throw std::invalid_argument(reason);
        }
    }
    
    void Logger::open(const char * R2D9_NONNULL path) {
        close();
        if (!path) {
            throw std::invalid_argument("Logger path is null");
        }
        int fd;
        if ( (fd = ::open(path, static_cast<int>(O_WRONLY))) >= 0 ) {
            if (::lseek(fd, 0, SEEK_END) < 0) {
                ::close(fd);
                char reason[128];
                ::snprintf(reason, 128, "Invalid file descriptor: %i", fd);
                throw std::invalid_argument(reason);
            }
        } else if ( (fd = ::open(path, static_cast<int>(O_WRONLY | O_CREAT), static_cast<mode_t>(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))) < 0 ) {
            char errDescr[64];
            errDescr[0] = 0;
            const int errNo = errno;
            if (errNo != 0) {
                const auto seRes = ::strerror_r(errNo, errDescr, 64);
                (void)seRes;
            }
            char reason[256];
            ::snprintf(reason, 256, "Open file: %s, errno: %i, descr: %s", path, errNo, errDescr);
            throw std::runtime_error(reason);
        }
        _fd = fd;
    }
    
    void Logger::close() noexcept {
        if (_fd >= 0) {
            ::close(_fd);
            _fd = -1;
        }
    }
    
    Logger::~Logger() noexcept {
        close();
    }
  
    void Logger::systemLog(const int option,
                           const int priority,
                           const char * R2D9_NONNULL format,
                           va_list vaList) noexcept {
        ::openlog("coretempmgr", option, LOG_DAEMON);
        ::vsyslog(priority, format, vaList);
        ::closelog();
    }
    
    void Logger::systemLog(const char * R2D9_NULLABLE type, const char * R2D9_NONNULL format, ...) noexcept {
        int option = LOG_NDELAY, priority = LOG_INFO;
        if (type) {
            if (::strcmp(type, loggerTypeCritical) == 0) {
                option |= LOG_PERROR;
                priority = LOG_ALERT | LOG_CRIT;
            } else if (::strcmp(type, loggerTypeException) == 0) {
                option |= LOG_PERROR;
                priority = LOG_ALERT | LOG_ERR;
            }
        }
        
        va_list vaList;
        va_start(vaList, format);
        Logger::systemLog(option, priority, format, vaList);
        va_end(vaList);
    }
    
} // namespace r2d9
