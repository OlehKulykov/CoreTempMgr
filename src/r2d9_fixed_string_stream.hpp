/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_FIXED_STRING_STREAM_HPP__
#define __R2D9_FIXED_STRING_STREAM_HPP__ 1

#include <cstdio>
#include <chrono>
#include <stdexcept>
#include <climits>
#include <cassert>

#include "r2d9_time_utils.hpp"

namespace r2d9 {
    
    template<size_t MAX_LENGTH, bool THROWABLE = true>
    class FixedStringStream final {
    private:
        char _buffer[MAX_LENGTH + 1];
        size_t _length = 0;
        
        FixedStringStream & append(const char * R2D9_NONNULL str, const size_t len) {
            const size_t availableLen = MAX_LENGTH - _length;
            bool overflow = false;
            size_t writeLen;
            if (availableLen < len) {
                writeLen = availableLen;
                overflow = true;
            } else {
                writeLen = len;
            }
            if (writeLen > 0) {
                ::memcpy(static_cast<char *>(_buffer) + _length, str, writeLen);
                _length += writeLen;
                assert(_length <= MAX_LENGTH);
                _buffer[_length] = 0;
            }
            if (overflow && THROWABLE) {
                throw std::overflow_error("Not fully appended");
            }
            return *this;
        }
        
        template<typename T, size_t BUFF_SIZE>
        FixedStringStream & appendf(const char * R2D9_NONNULL format, T v) {
            char buff[BUFF_SIZE];
            const auto len = ::snprintf(static_cast<char *>(buff), BUFF_SIZE, format, v);
            if (THROWABLE && (len < 0)) {
                throw std::runtime_error("Encoding error");
            }
            return (len > 0) ? append(static_cast<const char *>(buff), static_cast<size_t>(len)) : *this;
        }
        
    public:
        constexpr static size_t maxLength() noexcept { return MAX_LENGTH; }
        
        constexpr static size_t bufferCapacity() noexcept { return (MAX_LENGTH + 1); }
        
        inline size_t length() const noexcept { return _length; }
        
        void setLength(const size_t length) noexcept {
            if (length <= MAX_LENGTH) {
                _length = length;
                _buffer[_length] = 0;
            }
        }
        
        inline size_t availableLength() const noexcept { return (MAX_LENGTH - _length); }
        
        template<typename T>
        inline operator T () noexcept { return static_cast<T>(_buffer); }
        
        template<typename T>
        inline operator T () const noexcept { return static_cast<T>(_buffer); }
        
        void clear(const bool withZeros = false) noexcept {
            if (withZeros) {
                ::memset(static_cast<void *>(_buffer), 0, MAX_LENGTH + 1);
            } else {
                _buffer[0] = 0;
            }
            _length = 0;
        }
        
        template<class C>
        FixedStringStream & append(const std::chrono::time_point<C> & v, const size_t format = TimePointFormatDefaultUTC) {
            char buff[32];
            const size_t strLen = timePointToString(v, buff, 32, format);
            if (strLen > 0) {
                return append(buff, strLen);
            } else if (THROWABLE) {
                throw std::overflow_error("Append UTC date, time");
            }
            return *this;
        }
        
        template<typename T, class R>
        FixedStringStream & operator << (std::chrono::duration<T, R> v) {
            char buff[40];
            const auto strLen = durationToString(v, buff, 40);
            if (strLen > 0) {
                return append(buff, strLen);
            } else if (THROWABLE) {
                throw std::overflow_error("Append duration");
            }
            return *this;
        }
        
        template<typename S>
        FixedStringStream & operator << (std::chrono::duration<S> v) {
            char buff[40];
            const auto strLen = durationToString(v, buff, 40);
            if (strLen > 0) {
                return append(buff, strLen);
            } else if (THROWABLE) {
                throw std::overflow_error("Append duration");
            }
            return *this;
        }
        
        template<class C>
        FixedStringStream & operator << (std::chrono::time_point<C> v) {
            char buff[32];
            const auto strLen = timePointToString<C>(v, buff, 32, TimePointFormatDefaultUTC);
            return  (strLen > 0) ? append(buff, strLen) : *this;
        }
        
#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ == 16)
        //TODO: optimize
        //TODO: split into 2 (u)int64's if ever needed
        // 39 decimal digits:
        // −170,141,183,460,469,231,731,687,303,715,884,105,728  -  170,141,183,460,469,231,731,687,303,715,884,105,727
        //  340,282,366,920,938,463,463,374,607,431,768,211,455
        FixedStringStream & operator << (__uint128_t v) {
            // if (v == 0) return (*this << '0');
            if (v < ULLONG_MAX) return (*this << static_cast<unsigned long long>(v));
            
            char buff[48] = { 0 };
            char * ptr = static_cast<char *>(buff) + 47;
            *ptr = 0;
            while (v > 0) {
                *--ptr = static_cast<char>((v % 10) + '0');
                v /= 10;
            }
            return (*this << static_cast<const char *>(ptr));
        }
        
        FixedStringStream & operator << (__int128_t v) {
            if (v < 0) {
                v = -v;
                *this << '-';
            }
            return (*this << static_cast<__uint128_t>(v));
        }
#endif
        
        FixedStringStream & operator << (int v) {
            return appendf<int, 16>("%i", v);
        }
        
        FixedStringStream & operator << (unsigned int v) {
            return appendf<unsigned int, 16>("%u", v);
        }
        
        FixedStringStream & operator << (unsigned long v) {
            return appendf<unsigned long, 24>("%lu", v);
        }
        
        FixedStringStream & operator << (unsigned long long v) {
            return appendf<unsigned long long, 24>("%llu", v);
        }
        
        FixedStringStream & operator << (float v) {
            return appendf<float, 64>("%f", v);
        }
        
        FixedStringStream & operator << (double v) {
            return appendf<double, 96>("%f", v); // __DBL_MAX__ -> 336
        }
        
        FixedStringStream & operator << (const char * R2D9_NULLABLE v) {
            const size_t len = v ? ::strlen(v) : 0;
            return (len > 0) ? append(v, len) : *this;
        }
        
        FixedStringStream & operator << (char * R2D9_NULLABLE v) {
            const size_t len = v ? ::strlen(v) : 0;
            return (len > 0) ? append(static_cast<const char *>(v), len) : *this;
        }
        
        FixedStringStream & operator << (char v) {
            if (_length < MAX_LENGTH) {
                _buffer[_length++] = v;
                _buffer[_length] = 0;
            } else if (THROWABLE) {
                throw std::overflow_error("Append char");
            }
            return *this;
        }
        
        FixedStringStream & operator << (bool v) {
            return v ? append("true", 4) : append("false", 5);
        }
        
        FixedStringStream() noexcept {
            _buffer[0] = 0;
        }
    };

} // namespace r2d9

#endif //!__R2D9_FIXED_STRING_STREAM_HPP__
