/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __CTM_MANAGER_HPP__
#define __CTM_MANAGER_HPP__ 1

#include <memory>

#include "r2d9.hpp"

#include "ctm_config.hpp"
#include "ctm_sensors.hpp"
#include "r2d9_logger.hpp"

namespace ctm {
    
    class Manager final {        
    private:
        enum class State : uint16_t {
            none = 0,
            started,
            incremented,
            decremented,
            paused,
            stoped
        };
        
        std::unique_ptr<Config> _config;
        std::unique_ptr<Sensors> _sensors;
        std::unique_ptr<r2d9::Logger> _logger;
        
        CorePairsArray _corePairs;
        CoresArray _activeCores;
        
        int64_t _time = -1;
        int64_t _swapTime = -1;
        pid_t _xPID = -1;
        uint16_t _freq = 0;
        State _state = State::none;
        
        static int64_t currentTime();
        int64_t currentTemp() const;
        
        void performIncCores();
        void performIncFreq();
        bool performDecCores();
        bool performDecFreq();
        void performStartX();
        void performStopX();
        
        void performStart();
        void performInc();
        bool performDec();
        bool performPause() noexcept;
        bool performResume() noexcept;
        void performStop();
        void performSwap();
        
        bool canStart(const int64_t currTemp) const noexcept;
        bool canInc(const int64_t currTime) const noexcept;
        bool canDec(const int64_t currTime) const noexcept;
        bool canSwap(const int64_t currTime) const noexcept;
        bool canPause() const noexcept;
        bool canStop() const noexcept;
        
        void setFreq(const int cpuFreq);
        void setCores(const CoresArray & cores, const char * path);
        
        inline void setTimeState(const int64_t time, const State state) noexcept {
            _time = time;
            _state = state;
        }
        
        int executeSync(const char * command, char * const argv[], const size_t timeoutMs) noexcept;
        
        static CoresArray generateAppendingActiveCores(const CorePairsArray & allPairs,
                                                       const CoresArray & currCores,
                                                       const size_t appendCount);
        static int corePairsArraySortComparator(const void * a, const void * b) noexcept;
        static int corePairsArrayReverseSortComparator(const void * a, const void * b) noexcept;
        static bool corePairsArrayContains(const CorePairsArray & arr, const CorePair & pair) noexcept;
        static bool corePairsArrayContains(const CorePairsArray & arr,
                                           const CoreType core,
                                           size_t * index = nullptr) noexcept;
        static bool coresArrayContains(const CoresArray & arr, const CoreType core) noexcept;
        
    public:
        inline const std::unique_ptr<r2d9::Logger> & logger() const noexcept { return _logger; }
        
        void tick();
        
        void reloadConfig();
        
        void onExit() noexcept;
        
        void init(std::unique_ptr<Config> && config,
                  std::unique_ptr<Sensors> && sensors,
                  std::unique_ptr<r2d9::Logger> && logger);
        
        Manager() noexcept = default;
    };
    
} // namespace ctm

#endif //!__CTM_MANAGER_HPP__
