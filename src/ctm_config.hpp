/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __CTM_CONFIG_HPP__
#define __CTM_CONFIG_HPP__ 1

#include <filesystem>
#include <utility>
#include <cstdint>

#include "r2d9.hpp"

#include "r2d9_semi_fixed_array.hpp"
#include "r2d9_pair.hpp"

namespace ctm {
    
    typedef uint16_t CoreType;
    constexpr CoreType CoreTypeMax = UINT16_MAX;
    typedef r2d9::PairPOD<CoreType, CoreType> CorePair;
    typedef r2d9::SemiFixedArray<CorePair, uint16_t, 8, 8> CorePairsArray;
    typedef r2d9::SemiFixedArray<CoreType, uint16_t, 8, 8> CoresArray;
    
    // 65535 sec = 1092 min = 18 h
    class Config final {
    private:
        CoresArray _cores;
        char * R2D9_NULLABLE _path = nullptr;
        char * R2D9_NULLABLE _setFreqName = nullptr;
        char * R2D9_NULLABLE * R2D9_NULLABLE _setFreqArgv = nullptr;
        char * R2D9_NULLABLE _xName = nullptr;
        char * R2D9_NULLABLE _xConfigFilePath = nullptr;
        char * R2D9_NULLABLE * R2D9_NULLABLE _xArgv = nullptr;
        char * R2D9_NULLABLE _logFilePath = nullptr;
        uint32_t _checkTimeInc;
        uint32_t _checkTimeDec;
        uint32_t _coresSwapTime;
        uint16_t _freqMin;
        uint16_t _freqStart;
        uint16_t _freqOvercl;
        uint16_t _freqExit;
        uint16_t _stepInc;
        uint16_t _stepDec;
        uint16_t _tempMaxStable;
        uint16_t _tempMax;
        uint16_t _tempCrit;
        uint16_t _coresStart;
        uint16_t _coresMaxStable;
        
    public:
        inline const CoresArray & cores() const noexcept { return _cores; }
        inline const char * R2D9_NULLABLE path() const noexcept { return _path; }
        inline const char * R2D9_NULLABLE setFreqName() const noexcept { return _setFreqName; }
        inline char * const R2D9_NULLABLE * R2D9_NULLABLE setFreqArgv() const noexcept { return _setFreqArgv; }
        inline const char * R2D9_NULLABLE xName() const noexcept { return _xName; }
        inline const char * R2D9_NULLABLE xConfigFilePath() const noexcept { return _xConfigFilePath; }
        inline char * const R2D9_NULLABLE * R2D9_NULLABLE xArgv() const noexcept { return _xArgv; }
        inline const char * R2D9_NULLABLE logFilePath() const noexcept { return _logFilePath; }
        inline uint32_t checkTimeInc() const noexcept { return _checkTimeInc; }
        inline uint32_t checkTimeDec() const noexcept { return _checkTimeDec; }
        inline uint32_t coresSwapTime() const noexcept { return _coresSwapTime; }
        inline uint16_t freqMin() const noexcept { return _freqMin; }
        inline uint16_t freqStart() const noexcept { return _freqStart; }
        inline uint16_t freqOvercl() const noexcept { return _freqOvercl; }
        inline uint16_t freqExit() const noexcept { return _freqExit; }
        inline uint16_t stepInc() const noexcept { return _stepInc; }
        inline uint16_t stepDec() const noexcept { return _stepDec; }
        inline uint16_t tempMaxStable() const noexcept { return _tempMaxStable; }
        inline uint16_t tempMax() const noexcept { return _tempMax; }
        inline uint16_t tempCrit() const noexcept { return _tempCrit; }
        inline uint16_t coresStart() const noexcept { return _coresStart; }
        inline uint16_t coresMaxStable() const noexcept { return _coresMaxStable; }
        
        void load(const char * R2D9_NONNULL path);
        
        void clear() noexcept;
        
        Config() noexcept = default;
        ~Config() noexcept;
    };
    
} // namespace ctm

#endif //!__CTM_CONFIG_HPP__
