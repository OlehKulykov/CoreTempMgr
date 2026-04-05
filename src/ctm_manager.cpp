/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#include <limits>
#include <chrono>
#include <fstream>
#include <ostream>
#include <ctime>
#include <cmath>
#include <cstdlib>

#include "ctm_manager.hpp"
#include "r2d9_randomizer.hpp"
#include "r2d9_fixed_string_stream.hpp"

#include "r2d9_rapidjson.hpp"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"

#include "r2d9_c_string.h"

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#if defined(sigemptyset)
#  define SIGEMPTYSET_SAFE(m) sigemptyset(m)
#else
#  define SIGEMPTYSET_SAFE(m) ::sigemptyset(m)
#endif

namespace ctm {
    
    void Manager::performStartX() {
        const char * xName = _config->xName();
        char * const * xArgv = _config->xArgv();
        
        const pid_t pid = ::fork();
        if (pid < 0) {
            char reason[256];
            ::snprintf(reason, 256, "Failed to fork child process, errno: %i (%s)", errno, ::strerror(errno));
            throw std::runtime_error(reason);
        }
        
        if (pid == 0) {
            sigset_t mask;
            if (SIGEMPTYSET_SAFE(&mask) != 0) {
                r2d9::Logger::systemLog(r2d9::loggerTypeCritical, "Error cleanup signals for child process");
                ::_exit(EXIT_FAILURE);
            }
            
            if (::sigprocmask(SIG_SETMASK, &mask, nullptr) != 0) {
                r2d9::Logger::systemLog(r2d9::loggerTypeCritical, "Error set empty signals for child process");
                ::_exit(EXIT_FAILURE);
            }
            
#if !defined(DEBUG)
            r2d9_stdio_to_dev_null();
#endif
            // If the name is appended by a p, it will search for the file using the current environment variable PATH:
            //   /usr/bin/ | /usr/local/bin/ | ... etc.
            ::execvp(xName, xArgv);
            ::_exit(EXIT_FAILURE);
        }
        
        _xPID = pid;
        _logger->log(r2d9::loggerTypeInfo, "Child process %s[%" PRIi64 "] started", xName, static_cast<int64_t>(_xPID));
    }
    
    void Manager::performStopX() {
        if (_xPID > 0) {
            ::kill(_xPID, SIGTERM);
            
            int status = 0;
            if (::waitpid(_xPID, &status, 0) < 0) {
                ::kill(_xPID, SIGKILL);
                ::waitpid(_xPID, &status, 0);
            }
            
            _xPID = -1;
            _logger->log(r2d9::loggerTypeInfo, "Child process stopped, status: %i, errno: %i (%s)", status, errno, ::strerror(errno));
        }
    }
    
    void Manager::performStart() {
        _corePairs.clear();
        _activeCores.clear();
        _swapTime = _time = -1;
        _freq = 0;
        
        CorePairsArray corePairs;
        {
            const auto & cores = _config->cores();
            CorePair tmpPair;
            tmpPair.first = tmpPair.second = CoreTypeMax;
            for (size_t i = 0, n = cores.size(); i < n; i++) {
                if (tmpPair.first == CoreTypeMax) {
                    tmpPair.first = cores[i];
                } else {
                    tmpPair.second = cores[i];
                    corePairs.pushBack(tmpPair);
                    tmpPair.first = tmpPair.second = CoreTypeMax;
                }
            }
            if ((tmpPair.first != CoreTypeMax) && (tmpPair.second == CoreTypeMax)) {
                corePairs.pushBack(tmpPair);
            }
        }
        
        auto activeCores = generateAppendingActiveCores(corePairs, _activeCores, _config->coresStart());
        const auto freq = _config->freqStart();
        setCores(activeCores, _config->xConfigFilePath());
        setFreq(freq);
        _freq = freq;
        _activeCores = static_cast<CoresArray &&>(activeCores);
        _corePairs = static_cast<CorePairsArray &&>(corePairs);
        
        if (_state == State::paused) {
            if (performResume()) {
                return;
            }
            performStopX();
            _state = State::stoped;
        }
        
        if ((_state == State::none) || (_state == State::stoped) || (_xPID < 0)) {
            performStartX();
        }
    }
    
    void Manager::performIncCores() {
        auto activeCores = generateAppendingActiveCores(_corePairs, _activeCores, 1);
        if (activeCores.size() != (_activeCores.size() + 1)) {
            throw std::runtime_error("Error appending active cores");
        }
        setCores(activeCores, _config->xConfigFilePath());
        _activeCores = static_cast<CoresArray &&>(activeCores);
    }
    
    void Manager::performIncFreq() {
        const auto freq = _freq + _config->stepInc();
        setFreq(freq);
        _freq = freq;
    }
    
    void Manager::performInc() {
        if (_activeCores.size() < _config->coresStart()) {
            performIncCores();
            return;
        }
        
        if (_freq < _config->freqStart()) {
            performIncFreq();
            return;
        }
        
        if (_activeCores.size() < _config->coresMaxStable()) {
            performIncCores();
            return;
        }
        
        performIncFreq();
    }
    
    bool Manager::performDecCores() {
        CoresArray cores;
        for (size_t i = 1, n = _activeCores.size(); i < n; i++) {
            cores.pushBack(_activeCores[i]);
        }
        if (cores.size() > 0) {
            setCores(cores, _config->xConfigFilePath());
            _activeCores = static_cast<CoresArray &&>(cores);
            return true;
        }
        return false;
    }
    
    bool Manager::performDecFreq() {
        if (_freq > _config->freqMin()) {
            const auto freq = _freq - _config->stepDec();
            setFreq(freq);
            _freq = freq;
            return true;
        }
        return false;
    }
    
    bool Manager::performDec() {
        if ((_freq >= _config->freqOvercl()) && performDecFreq()) {
            return true;
        }
        
        if ((_activeCores.size() > _config->coresMaxStable()) && performDecCores()) {
            return true;
        }
        
        if ((_freq > _config->freqStart()) && performDecFreq()) {
            return true;
        }
        
        if ((_freq >= _config->freqStart()) && (_activeCores.size() > _config->coresStart())) {
            if (performDecCores()) {
                return true;
            }
        }
                
        if (performDecFreq()) {
            return true;
        }
        
        return performDecCores();
    }
    
    bool Manager::performPause() noexcept {
        if (_xPID > 0) {
            if (::kill(_xPID, SIGSTOP) == 0) {
                return true;
            }
            if ((errno == ESRCH) || (::kill(_xPID, 0) != 0)) {
                _xPID = -1;
            }
        }
        return false;
    }
    
    bool Manager::performResume() noexcept {
        if (_xPID > 0) {
            if (::kill(_xPID, SIGCONT) == 0) {
                return true;
            }
            if ((errno == ESRCH) || (::kill(_xPID, 0) != 0)) {
                _xPID = -1;
            }
        }
        return false;
    }
        
    void Manager::performStop() {
        performStopX();
        setFreq(_config->freqExit());
    }
    
    void Manager::performSwap() {
        const size_t coresCount = _activeCores.size();
        CorePairsArray usedPairs;
        for (size_t i = 0; i < coresCount; i++) {
            size_t index;
            if (corePairsArrayContains(_corePairs, _activeCores[i], &index)) {
                const auto & pair = _corePairs[index];
                if (!corePairsArrayContains(usedPairs, pair)) {
                    usedPairs.pushBack(pair);
                }
            }
        }
        
        CorePairsArray unusedPairs;
        for (size_t i = 0, n = _corePairs.size(); i < n; i++) {
            const auto & pair = _corePairs[i];
            if (!corePairsArrayContains(usedPairs, pair)) {
                unusedPairs.pushBack(pair);
            }
        }
        
        r2d9::randomShuffle(unusedPairs.data(), static_cast<uint32_t>(unusedPairs.size()));
        
        CorePairsArray activePairs;
        size_t addedCount = 0;
        for (size_t i = 0, n = unusedPairs.size(); ((i < n) && (addedCount < coresCount)); i++) {
            const auto & pair = unusedPairs[i];
            activePairs.pushBack(pair);
            addedCount++;
            if (pair.second != CoreTypeMax) {
                addedCount++;
            }
        }
        
        if (addedCount < coresCount) {
            r2d9::randomShuffle(usedPairs.data(), static_cast<uint32_t>(usedPairs.size()));
            
            for (size_t i = 0, n = usedPairs.size(); ((i < n) && (addedCount < coresCount)); i++) {
                const auto & pair = usedPairs[i];
                activePairs.pushBack(pair);
                addedCount++;
                if (pair.second != CoreTypeMax) {
                    addedCount++;
                }
            }
        }
        
        ::qsort(activePairs.data(), activePairs.size(), sizeof(CorePair), corePairsArrayReverseSortComparator);
        
        CoresArray activeCores;
        for (size_t i = 0, n = activePairs.size(); i < n; i++) {
            const auto pair = activePairs[i];
            if ((pair.first != CoreTypeMax) && (pair.second != CoreTypeMax)) {
                activeCores.pushFront(pair.second);
                if (activeCores.size() == coresCount) {
                    i = n;
                } else {
                    activeCores.pushFront(pair.first);
                }
            } else if (pair.second != CoreTypeMax) {
                activeCores.pushFront(pair.second);
            } else if (pair.first != CoreTypeMax) {
                activeCores.pushFront(pair.first);
            }
            if (activeCores.size() == coresCount) {
                i = n;
            }
        }
        
        if (activeCores.size() != coresCount) {
            throw std::runtime_error("Swap: size mismatch");
        }
        
        if (coresCount % 2) {
            size_t i1, i2;
            const auto c1 = _activeCores[0], c2 = activeCores[0];
            if (corePairsArrayContains(_corePairs, c1, &i1) && corePairsArrayContains(_corePairs, c2, &i2)) {
                const auto p1 = _corePairs[i1], p2 = _corePairs[i2];
                if ((p1.second == c1) && (p2.second == c2)) {
                    activeCores[0] = p2.first;
                } else if ((p1.first == c1) && (p2.first == c2) && (p2.second != CoreTypeMax)) {
                    activeCores[0] = p2.second;
                }
            }
        }
        
        setCores(activeCores, _config->xConfigFilePath());
        _activeCores = static_cast<CoresArray &&>(activeCores);
    }
    
    int Manager::executeSync(const char * command, char * const argv[], const size_t timeoutMs) noexcept {
        const pid_t pid = ::fork();
        if (pid < 0) {
            return -1;
        }
        
        if (pid == 0) {
            sigset_t mask;
            SIGEMPTYSET_SAFE(&mask);
            ::sigprocmask(SIG_SETMASK, &mask, nullptr);
            ::execvp(command, argv);
            ::_exit(EXIT_FAILURE);
        }
        
        size_t elapsed = 0;
        int status = 0;
        while (elapsed < timeoutMs) {
            pid_t result = ::waitpid(pid, &status, WNOHANG);
            if (result == pid) {
                return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            } else if (result == 0) {
                ::usleep(50000);    // 50 millisec * 1000 = 50000 nanosec
                elapsed += 50;      // 50 millisec
            } else {
                return -1;
            }
        }
        
        _logger->log(r2d9::loggerTypeCritical, "Sync command \'%s\' timed out, killing...", command);
        ::kill(pid, SIGKILL);
        ::waitpid(pid, &status, 0);
        return -1;
    }
    
    void Manager::tick() {
        const int64_t temp = currentTemp();
        const int64_t time = currentTime();
        
        if (canStart(temp)) {
            _logger->log(r2d9::loggerTypeInfo, "Starting, %" PRIi64 "°C", temp);
            performStart();
            setTimeState((_swapTime = time), State::started);
            return;
        }
        
        if (temp >= _config->tempCrit()) {
            if (performDec()) {
                setTimeState(time, State::decremented);
                _logger->log(r2d9::loggerTypeInfo, "Crit: decremented %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
                return;
            }
            if (canPause()) {
                if (performPause()) {
                    setTimeState(time, State::paused);
                    _logger->log(r2d9::loggerTypeInfo, "Crit: paused, %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
                    return;
                }
            }
            if (canStop()) {
                performStop();
                setTimeState(time, State::stoped);
                _logger->log(r2d9::loggerTypeInfo, "Crit: stoped, %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
                _swapTime = -1;
            }
            return;
        }
        
        if (temp >= _config->tempMax()) {
            if (canDec(time)) {
                if (performDec()) {
                    setTimeState(time, State::decremented);
                    _logger->log(r2d9::loggerTypeInfo, "Max: decremented, %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
                    return;
                }
                if (canPause()) {
                    if (performPause()) {
                        setTimeState(time, State::paused);
                        _logger->log(r2d9::loggerTypeInfo, "Max: paused, %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
                        return;
                    }
                }
                _logger->log(r2d9::loggerTypeInfo, "Max: can't dec|pause. Stopping, %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
                performStop();
                setTimeState(time, State::stoped);
                _swapTime = -1;
            }
            return;
        }
        
        if (canSwap(time)) { // before inc
            r2d9::FixedStringStream<127> fs; fs << '[';
            for (size_t i = 0, n = _activeCores.size(); i < n; i++) {
                if (i) fs << ',';
                fs << _activeCores[i];
            }
            fs << ']';
            performSwap();
            _swapTime = time;
            fs << " » [";
            for (size_t i = 0, n = _activeCores.size(); i < n; i++) {
                if (i) fs << ',';
                fs << _activeCores[i];
            }
            fs << ']';
            _logger->log(r2d9::loggerTypeInfo, "Swapped, %s, %" PRIu16 "MHz %" PRIi64 "°C", static_cast<const char *>(fs), _freq, temp);
        }
        
        if ((temp < _config->tempMaxStable()) && canInc(time)) {
            performInc();
            setTimeState(time, State::incremented);
            _logger->log(r2d9::loggerTypeInfo, "Stable: incremented, %" PRIu32 " core(s) %" PRIu16 "MHz %" PRIi64 "°C", static_cast<uint32_t>(_activeCores.size()), _freq, temp);
        }
    }
    
    bool Manager::canStart(const int64_t currTemp) const noexcept {
        switch (_state) {
            case State::none:
                return true;
                
            case State::paused:
            case State::stoped:
                return (currTemp < _config->tempMaxStable());
                
            default:
                break;
        }
        return false;
    }
    
    bool Manager::canInc(const int64_t currTime) const noexcept {
        switch (_state) {
            case State::started:
            case State::incremented:
            case State::decremented:
                return ((currTime - _time) >= _config->checkTimeInc());
                
            default:
                break;
        }
        return false;
    }
    
    bool Manager::canDec(const int64_t currTime) const noexcept {
        switch (_state) {
            case State::none:
            case State::paused:
            case State::stoped:
                return false;
                
            case State::started:
            case State::incremented:
                return true;
                
            case State::decremented:
                if ((currTime - _time) < _config->checkTimeDec()) {
                    return false;
                }
                break;
                
            default:
                break;
        }
        return true;
    }
    
    bool Manager::canSwap(const int64_t currTime) const noexcept {
        switch (_state) {
            case State::none:
            case State::stoped:
                return false;
                
            default:
                break;
        }
        
        return ( (_swapTime > 0) && ((currTime - _swapTime) > _config->coresSwapTime()) );
    }
    
    bool Manager::canPause() const noexcept {
        switch (_state) {
            case State::none:
            case State::paused:
            case State::stoped:
                return false;
                
            default:
                break;
        }
        return true;
    }
    
    bool Manager::canStop() const noexcept {
        return (_state != State::stoped);
    }
    
    void Manager::reloadConfig() {
        
    }
    
    void Manager::onExit() noexcept {
        try {
            _logger->log(r2d9::loggerTypeInfo, "Exit");
            performStop();
        } catch (const std::exception & exception) {
            _logger->log(exception);
        }
    }
    
    void Manager::init(std::unique_ptr<Config> && config,
                       std::unique_ptr<Sensors> && sensors,
                       std::unique_ptr<r2d9::Logger> && logger) {
        _config = static_cast<std::unique_ptr<Config> &&>(config);
        _sensors = static_cast<std::unique_ptr<Sensors> &&>(sensors);
        _logger = static_cast<std::unique_ptr<r2d9::Logger> &&>(logger);
    }
    
    int64_t Manager::currentTemp() const {
        const double temp = _sensors->cpuTemp();
        if (temp < -273.15) {
            char reason[96];
            ::snprintf(reason, 96, "CPU temperature out of range: %f°C", temp);
            throw std::range_error(reason);
        }
        return ::round(temp);
    }
    
    int64_t Manager::currentTime() {
        struct timespec tms;
        if (::clock_gettime(CLOCK_MONOTONIC, &tms) != 0) {
            char reason[256];
            ::snprintf(reason, 256, "Error retrieve current time, errno: %i (%s)", errno, ::strerror(errno));
            throw std::runtime_error(reason);
        }
        return ((static_cast<int64_t>(tms.tv_sec) * 1000) + (static_cast<int64_t>(tms.tv_nsec) / 1000000));
    }
    
    void Manager::setFreq(const int cpuFreq) {
        char * args[32] = { nullptr };
        char * const * tmpArgs = _config->setFreqArgv();
        char * arg;
        size_t i = 0;
        char freqArg[32];
        while ( (arg = *tmpArgs++) ) {
            if (i == 31) { // last args must be nullptr
                throw std::runtime_error("Too much arguments");
            }
            if (::strstr(arg, "%i")) {
                ::snprintf(freqArg, 32, arg, cpuFreq);
                args[i++] = static_cast<char *>(freqArg);
            } else {
                args[i++] = arg;
            }
        }
        if (executeSync(_config->setFreqName(), args, 2000) < 0) {
            char reason[256];
            ::snprintf(reason, 256, "Error set CPU frequency %iMHz, errno: %i (%s)", cpuFreq, errno, ::strerror(errno));
            throw std::runtime_error(reason);
        }
    }
    
    void Manager::setCores(const CoresArray & cores, const char * path) {
        r2d9rj::RJDocument doc;
        if (path) {
            std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
            if (!ifs.is_open()) {
                throw std::runtime_error("Can't open XConfig file path");
            }
            
            r2d9rj::IStreamWrapper rjifs(ifs);
            doc.ParseStream<r2d9rj::kParseStopWhenDoneFlag | r2d9rj::kParseTrailingCommasFlag, r2d9rj::UTF8<> >(rjifs);
            
            if (doc.HasParseError()) {
                char reason[256];
                ::snprintf(reason, 256, "XConfig parse error: \'%s\', offset: %" PRIu64,
                           r2d9rj::GetParseError_En(doc.GetParseError()) ?: "Unknown",
                           static_cast<uint64_t>(doc.GetErrorOffset()));
                throw std::runtime_error(reason);
            }
        } else {
            throw std::invalid_argument("Null XConfig path");
        }
        
        auto & cpu0 = doc["cpu"];
        if (!cpu0.IsObject()) {
            throw std::runtime_error("XConfig 'cpu' not found");
        }
        
        auto & allocator = doc.GetAllocator();
        r2d9rj::RJValue cpu1;
        cpu1.CopyFrom(cpu0, allocator);
        
        auto & rx = cpu1["rx"];
        if (!rx.IsArray()) {
            throw std::runtime_error("XConfig 'rx' not found");
        }
        
        rx.Clear();
        rx.Reserve(cores.size(), allocator);
        for (size_t i = 0, n = cores.size(); i < n; i++) {
            rx.PushBack(r2d9rj::RJValue(cores[i]), allocator);
        }
        
        cpu0.Swap(cpu1);
        
        FILE * f = ::fopen(path, "wb");
        if (!f) {
            char reason[256];
            ::snprintf(reason, 256, "XConfig can't open xconf file: \'%s\'", path);
            throw std::runtime_error(reason);
        }
        
        try {
            char reason[4096];
            r2d9rj::FileWriteStream osw(f, reason, 4096);
            r2d9rj::PrettyWriter<r2d9rj::FileWriteStream> writer(osw);
            writer.SetFormatOptions(r2d9rj::kFormatSingleLineArray);
            doc.Accept(writer);
            osw.Flush();
        } catch (...) {
            ::fclose(f);
            throw;
        }
        
        ::fclose(f);
    }
    
    CoresArray Manager::generateAppendingActiveCores(const CorePairsArray & allPairs,
                                                     const CoresArray & currCores,
                                                     const size_t appendCount) {
        CorePairsArray activePairs;
        for (size_t i = 0, n = currCores.size(); i < n; i++) {
            size_t index;
            if (corePairsArrayContains(allPairs, currCores[i], &index)) {
                const auto pair = allPairs[index];
                if (!corePairsArrayContains(activePairs, pair)) {
                    activePairs.pushBack(pair);
                }
            }
        }
        
        size_t addedCoresCount = 0;
        for (size_t i = 0, n = activePairs.size(); i < n; i++) {
            const auto pair = activePairs[i];
            if (coresArrayContains(currCores, pair.first)) {
                addedCoresCount++;
            }
            if (coresArrayContains(currCores, pair.second)) {
                addedCoresCount++;
            }
        }
        
        const size_t coresCount = currCores.size() + appendCount;
        uint32_t prevIndex = static_cast<uint32_t>(allPairs.size()), pairsMaxIndex = prevIndex - 1;
        
        while (addedCoresCount < coresCount) {
            uint32_t pairIndex;
            bool generating = true;
            do {
                if ( (pairIndex = r2d9::randomInRange<uint32_t>(0, pairsMaxIndex)) != prevIndex) {
                    const auto pair = allPairs[pairIndex];
                    if (!corePairsArrayContains(activePairs, pair)) {
                        activePairs.pushBack(pair);
                        if (pair.first != CoreTypeMax) {
                            addedCoresCount++;
                        }
                        if (pair.second != CoreTypeMax) {
                            addedCoresCount++;
                        }
                        generating = false;
                        prevIndex = pairIndex;
                    }
                }
            } while (generating);
        }
        ::qsort(activePairs.data(), activePairs.size(), sizeof(CorePair), corePairsArrayReverseSortComparator);
        
        CoresArray activeCores;
        for (size_t i = 0, n = activePairs.size(); i < n; i++) {
            const auto pair = activePairs[i];
            if ((pair.first != CoreTypeMax) && (pair.second != CoreTypeMax)) {
                activeCores.pushFront(pair.second);
                if (activeCores.size() == coresCount) {
                    i = n;
                } else {
                    activeCores.pushFront(pair.first);
                }
            } else if (pair.second != CoreTypeMax) {
                activeCores.pushFront(pair.second);
            } else if (pair.first != CoreTypeMax) {
                activeCores.pushFront(pair.first);
            }
            if (activeCores.size() == coresCount) {
                i = n;
            }
        }
        return activeCores;
    }
    
    int Manager::corePairsArraySortComparator(const void * a, const void * b) noexcept {
        const auto * p1 = static_cast<const CorePair *>(a), * p2 = static_cast<const CorePair *>(b);
        return (static_cast<int>(p1->first) - static_cast<int>(p2->first));
    }
    
    int Manager::corePairsArrayReverseSortComparator(const void * a, const void * b) noexcept {
        const auto * p1 = static_cast<const CorePair *>(a), * p2 = static_cast<const CorePair *>(b);
        return (static_cast<int>(p2->first) - static_cast<int>(p1->first));
    }
    
    bool Manager::corePairsArrayContains(const CorePairsArray & arr, const CorePair & pair) noexcept {
        const auto * data = arr.data();
        for (size_t i = 0, n = arr.size(); i < n; i++, data++) {
            if (data->first == pair.first) {
                return true;
            }
        }
        return false;
    }
    
    bool Manager::corePairsArrayContains(const CorePairsArray & arr,
                                         const CoreType core,
                                         size_t * index /* nullptr */) noexcept {
        const auto * data = arr.data();
        for (size_t i = 0, n = arr.size(); i < n; i++, data++) {
            if ((data->first == core) || (data->second == core)) {
                if (index) {
                    *index = i;
                }
                return true;
            }
        }
        return false;
    }
    
    bool Manager::coresArrayContains(const CoresArray & arr, const CoreType core) noexcept {
        const auto * data = arr.data();
        for (size_t i = 0, n = arr.size(); i < n; i++, data++) {
            if (*data == core) {
                return true;
            }
        }
        return false;
    }
    
} // namespace ctm
