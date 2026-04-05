/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_TIME_UTILS_HPP__
#define __R2D9_TIME_UTILS_HPP__ 1

#include <chrono>
#include <cstdio>
#include <ctime>
#include <cstdio>

#include "r2d9.hpp"

namespace r2d9 {
    
    enum TimePointFormat : size_t {
        TimePointFormatLocal        = 1 << 0,
        TimePointFormatMillisec     = 1 << 1,
        TimePointFormatSpaceSep     = 1 << 2,
        TimePointFormatTimeZone     = 1 << 3,
        
        TimePointFormatDefaultUTC   = (TimePointFormatMillisec | TimePointFormatTimeZone),
        TimePointFormatDefaultLocal = (TimePointFormatLocal | TimePointFormatMillisec | TimePointFormatSpaceSep | TimePointFormatTimeZone),
        TimePointFormatUTCSpaceSep  = (TimePointFormatMillisec | TimePointFormatSpaceSep | TimePointFormatTimeZone)
    };
    
    // min 30(32) len
    template <class C>
    size_t timePointToString(const std::chrono::time_point<C> & timePoint,
                             char * R2D9_NONNULL buff,
                             size_t buffSize,
                             const size_t format = TimePointFormatDefaultUTC) {
        if (buffSize < 30) {
            return 0;
        }
        const time_t rawtime = C::to_time_t(timePoint);
        const bool isUTC = (format & TimePointFormatLocal) ? false : true;
        struct tm * timeinfo = isUTC ? ::gmtime(&rawtime) : ::localtime(&rawtime);
        const char * formatStr = (format & TimePointFormatSpaceSep) ? "%Y-%m-%d %H:%M:%S" : "%Y-%m-%dT%H:%M:%S";
        size_t dateLen = timeinfo ? ::strftime(buff, buffSize, formatStr, timeinfo) : 0;
        if (dateLen == 0) {
            return 0;
        }
        
        buffSize -= dateLen;
        buff += dateLen;
        
        if (format & TimePointFormatMillisec) {
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
            milliseconds %= 1000;
            
            const auto milliLen = ::snprintf(buff, buffSize, ".%03lli", static_cast<long long>(milliseconds));
            if (milliLen <= 0) {
                return 0;
            } else {
                dateLen += milliLen;
                buff += milliLen;
            }
        }
        
        if (format & TimePointFormatTimeZone) {
            if (isUTC) {
                *buff++ = 'Z'; // As format Z  This date time has no offset from UTC (or 0 hour offset).
                dateLen++;
            } else {
                const size_t tmpLen = ::strftime(buff, buffSize, "%z", timeinfo);
                if (tmpLen == 0) {
                    return 0;
                }
                dateLen += tmpLen;
                buffSize -= tmpLen;
                buff += tmpLen;
            }
        }
        
        *buff = 0;
        return dateLen;
    }
    
    // min 21(24) len
    template <typename T, class R>
    size_t durationToString(const std::chrono::duration<T, R> & duration, char * R2D9_NONNULL buff, size_t buffSize) {
        if (buffSize < 20) {
            return 0;
        }
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
        const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
        if ((hours < 0) || (minutes < 0) || (seconds < 0) || (milliseconds < 0)) {
            return 0;
        }
        
        const auto days = hours / 24;
        hours %= 24;
        
        size_t len = 0;
        if (days > 0) {
            const auto daysLen = ::snprintf(buff, buffSize, "%lu:", static_cast<unsigned long>(days));
            if (daysLen <= 0) {
                return 0;
            }
            buffSize -= daysLen;
            buff += daysLen;
            len += daysLen;
        }
        
        const auto tmpLen = ::snprintf(buff, buffSize, "%02lu:%02lu:%02lu.%03lu",
                                       static_cast<unsigned long>(hours),
                                       static_cast<unsigned long>(minutes),
                                       static_cast<unsigned long>(seconds),
                                       static_cast<unsigned long>(milliseconds));
        
        return (tmpLen > 0) ? (len + tmpLen) : 0;
    }
    
    template <typename S>
    size_t durationToString(const std::chrono::duration<S> & duration, char * R2D9_NONNULL buff, size_t buffSize) {
        return durationToString<std::chrono::milliseconds>(std::chrono::duration_cast<std::chrono::milliseconds>(duration), buff, buffSize);
    }
        
} // namespace r2d9

#endif //!__R2D9_TIME_UTILS_HPP__
