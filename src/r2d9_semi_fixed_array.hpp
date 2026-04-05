/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_SEMI_FIXED_ARRAY_HPP__
#define __R2D9_SEMI_FIXED_ARRAY_HPP__ 1

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "r2d9.hpp"

namespace r2d9 {
    
    template<class T, typename SIZE_TYPE = size_t, SIZE_TYPE FIXED_CAPACITY = 8, SIZE_TYPE CAPACITY_INC = FIXED_CAPACITY>
    class SemiFixedArray final {
    public:
        typedef SemiFixedArray<T, SIZE_TYPE, FIXED_CAPACITY, CAPACITY_INC> SelfType;
        typedef T ValueType;
        typedef SIZE_TYPE SizeType;
        
    private:
        T _fixed[FIXED_CAPACITY];
        T * R2D9_NULLABLE _dynamic = nullptr;
        SIZE_TYPE _capacity = FIXED_CAPACITY;
        SIZE_TYPE _size = 0;
        
        T * R2D9_NONNULL extend(const size_t inc = 1) {
            size_t capacity = inc + _size;
            if (capacity > _capacity) {
                if (inc < CAPACITY_INC) {
                    capacity = static_cast<size_t>(_size) + CAPACITY_INC;
                }
                T * dynamic = new T[capacity]();
                if (!dynamic) {
                    throw std::bad_alloc();
                }
                if (_dynamic) {
                    for (size_t i = 0; i < _size; i++) {
                        dynamic[i] = static_cast<T &&>(_dynamic[i]);
                    }
                    delete [] _dynamic;
                } else {
                    for (size_t i = 0; i < _size; i++) {
                        dynamic[i] = static_cast<T &&>(_fixed[i]);
                    }
                }
                _dynamic = dynamic;
                _capacity = static_cast<SIZE_TYPE>(capacity);
            }
            return _dynamic ? _dynamic : static_cast<T *>(_fixed);
        }
        
    public:
        inline T * R2D9_NONNULL data() noexcept {
            return _dynamic ? _dynamic : static_cast<T *>(_fixed);
        }
        
        inline const T * R2D9_NONNULL data() const noexcept {
            return _dynamic ? _dynamic : static_cast<const T *>(_fixed);
        }
        
        inline size_t size() const noexcept { return _size; }
        
        inline size_t capacity() const noexcept { return _capacity; }
        
        inline T & operator [] (size_t idx) {
            return _dynamic ? _dynamic[idx] : _fixed[idx];
        }
        
        inline const T & operator [] (size_t idx) const {
            return _dynamic ? _dynamic[idx] : _fixed[idx];
        }
        
        void pushBack(const T & elem) {
            *(extend() + _size) = elem;
            _size++;
        }
        
        void emplaceBack(T && elem) {
            *(extend() + _size) = static_cast<T &&>(elem);
            _size++;
        }
        
        void pushFront(const T & elem) {
            auto * data = extend();
            if (_size > 0) {
                for (size_t i = _size; i > 0; i--) {
                    data[i] = data[i - 1];
                }
            }
            data[0] = elem;
            _size++;
        }
        
        void emplaceFront(T && elem) {
            auto * data = extend();
            if (_size > 0) {
                for (size_t i = _size; i > 0; i--) {
                    data[i] = static_cast<T &&>(data[i - 1]);
                }
            }
            data[0] = static_cast<T &&>(elem);
            _size++;
        }
        
        void clear() noexcept {
            if (_dynamic) {
                delete [] _dynamic;
                _dynamic = nullptr;
            } else {
                for (size_t i = 0; i < _size; i++) {
                    T elem = static_cast<T &&>(_fixed[i]);
                    (void)elem;
                }
            }
            _size = 0;
            _capacity = FIXED_CAPACITY;
        }
        
        SelfType & operator = (SelfType && array) noexcept {
            clear();
            
            if (array._dynamic) {
                _dynamic = array._dynamic;
                array._dynamic = nullptr;
            } else {
                for (size_t i = 0, n = array._size; i < n; i++) {
                    _fixed[i] = static_cast<T &&>(array._fixed[i]);
                }
            }
            _size = array._size;
            _capacity = array._capacity;
            array._size = 0;
            array._capacity = FIXED_CAPACITY;
            return *this;
        }
        
        SemiFixedArray(SelfType && array) noexcept {
            *this = static_cast<SelfType &&>(array);
        }
        
        SemiFixedArray() noexcept = default;
        
        SemiFixedArray(const SIZE_TYPE capacity) {
            extend(capacity);
        }
        
        ~SemiFixedArray() noexcept {
            clear();
        }
    };
    
} // namespace r2d9

#endif //!__R2D9_SEMI_FIXED_ARRAY_HPP__
