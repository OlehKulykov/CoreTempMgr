/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>

#include "r2d9.hpp"

#include "ctm_config.hpp"
#include "r2d9_pair.hpp"
#include "r2d9_rapidjson.hpp"

#include "r2d9_c_string.h"

#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"

namespace ctm {
    
    static void configFreeArrayOfStrings(char * * array) {
        if (array) {
            char * * tmpArray = array;
            char * elem;
            while ( (elem = *tmpArray) ) {
                r2d9_c_str_release(elem);
                tmpArray++;
            }
            ::free(array);
        }
    }
    
    template<typename T>
    T parseConfigValueFromDoc(const r2d9rj::RJDocument & doc, const char * key);
    
    template<>
    uint16_t parseConfigValueFromDoc(const r2d9rj::RJDocument & doc, const char * key) {
        char reason[128];
        const auto it = doc.FindMember(key);
        if ((it != doc.MemberEnd()) && it->value.IsUint()) {
            const auto val = it->value.GetUint();
            if (val <= UINT16_MAX) {
                return static_cast<uint16_t>(val);
            }
            ::snprintf(reason, 128, "Config[\"%s\"] value out of range", key);
        } else {
            ::snprintf(reason, 128, "Config[\"%s\"] missed or not uint", key);
        }
        throw std::runtime_error(reason);
    }
    
    template<>
    char * parseConfigValueFromDoc(const r2d9rj::RJDocument & doc, const char * key) {
        const auto it = doc.FindMember(key);
        if ((it != doc.MemberEnd()) && it->value.IsString()) {
            return r2d9_c_str_copy_non_empty(it->value.GetString());
        }
        char reason[128];
        ::snprintf(reason, 128, "Config[\"%s\"] missed or not string", key);
        throw std::runtime_error(reason);
    }
    
    template<>
    CoresArray parseConfigValueFromDoc(const r2d9rj::RJDocument & doc, const char * key) {
        char reason[128];
        const auto it = doc.FindMember(key);
        if ((it != doc.MemberEnd()) && it->value.IsArray()) {
            CoresArray vec;
            const auto & arr = it->value.GetArray();
            const auto size = arr.Size();
            if (size) {
                for (size_t i = 0; i < size; i++) {
                    const auto & elem = arr[i];
                    uint32_t val;
                    if (elem.IsUint() && ((val = elem.GetUint()) <= UINT16_MAX)) {
                        vec.pushBack(static_cast<uint16_t>(val));
                    }
                }
            }
            if (vec.size() == size) {
                return vec;
            }
            ::snprintf(reason, 128, "Config[\"%s\"] error parsing array elemets", key);
        } else {
            ::snprintf(reason, 128, "Config[\"%s\"] missed or not array", key);
        }
        throw std::runtime_error(reason);
    }
    
    template<>
    char * *  parseConfigValueFromDoc(const r2d9rj::RJDocument & doc, const char * key) {
        char reason[128];
        char * * res = nullptr;
        const auto it = doc.FindMember(key);
        if ((it != doc.MemberEnd()) && it->value.IsArray()) {
            const auto & arr = it->value.GetArray();
            const auto size = arr.Size();
            const auto memSize = sizeof(char *) * (size + 1);
            res = static_cast<char * *>(::malloc(memSize));
            if (!res) {
                throw std::bad_alloc();
            }
            ::memset(res, 0, memSize);
            if (size) {
                for (size_t i = 0; i < size; i++) {
                    const auto & elem = arr[i];
                    if (elem.IsString()) {
                        res[i] = r2d9_c_str_copy_with_empty(elem.GetString());
                    }
                    if (!res[i]) {
                        configFreeArrayOfStrings(res);
                        ::snprintf(reason, 128, "Config[\"%s\"] array element not string", key);
                        throw std::runtime_error(reason);
                    }
                }
            }
            return res;
        }
        configFreeArrayOfStrings(res);
        ::snprintf(reason, 128, "Config[\"%s\"] missed or not array", key);
        throw std::runtime_error(reason);
    }
    
    void Config::clear() noexcept {
        r2d9_c_str_release(_path);
        r2d9_c_str_release(_setFreqName);
        configFreeArrayOfStrings(_setFreqArgv);
        r2d9_c_str_release(_xName);
        r2d9_c_str_release(_xConfigFilePath);
        configFreeArrayOfStrings(_xArgv);
        r2d9_c_str_release(_logFilePath);
        
        _path = _setFreqName = nullptr;
        _xName = _xConfigFilePath = _logFilePath = nullptr;
        _setFreqArgv = _xArgv = nullptr;
        
        _cores.clear();
        _checkTimeInc = _checkTimeDec = _freqMin = _freqStart = 0;
        _freqOvercl = _freqExit = _stepInc = _stepDec = _coresSwapTime = 0;
        _tempMaxStable = _tempMax = _tempCrit = _coresStart = _coresMaxStable = 0;
    }
    
    void Config::load(const char * R2D9_NONNULL path) {
        clear();
        
        if (!path) {
            throw std::invalid_argument("Config path is null");
        }
        
        r2d9rj::RJDocument doc;
        {
            std::ifstream ifs(path, std::ios_base::in | std::ios_base::binary);
            if (!ifs.is_open()) {
                throw std::runtime_error("Can't open config file path");
            }
            
            r2d9rj::IStreamWrapper rjifs(ifs);
            doc.ParseStream<r2d9rj::kParseStopWhenDoneFlag | r2d9rj::kParseTrailingCommasFlag | r2d9rj::kParseCommentsFlag | r2d9rj::kParseTrailingCommasFlag,  r2d9rj::UTF8<> >(rjifs);
        }
        
        if (doc.HasParseError()) {
            char reason[256];
            ::snprintf(reason, 256, "Config parse error: \'%s\', offset: %" PRIu64,
                       r2d9rj::GetParseError_En(doc.GetParseError()) ?: "Unknown",
                       static_cast<uint64_t>(doc.GetErrorOffset()));
            throw std::runtime_error(reason);
        }
        
        try {
            _cores = parseConfigValueFromDoc<CoresArray>(doc, "cores");
            _setFreqName = parseConfigValueFromDoc<char *>(doc, "set_freq_name");
            _setFreqArgv = parseConfigValueFromDoc<char * *>(doc, "set_freq_argv");
            _xName = parseConfigValueFromDoc<char *>(doc, "xname");
            _xConfigFilePath = parseConfigValueFromDoc<char *>(doc, "xconfig");
            _xArgv = parseConfigValueFromDoc<char * *>(doc, "xargv");
            _logFilePath = parseConfigValueFromDoc<char *>(doc, "log");
            _checkTimeInc = static_cast<uint32_t>(parseConfigValueFromDoc<uint16_t>(doc, "check_time_inc")) * 1000;
            _checkTimeDec = static_cast<uint32_t>(parseConfigValueFromDoc<uint16_t>(doc, "check_time_dec")) * 1000;
            _coresSwapTime = static_cast<uint32_t>(parseConfigValueFromDoc<uint16_t>(doc, "cores_swap_time")) * 1000;
            _freqMin = parseConfigValueFromDoc<uint16_t>(doc, "freq_min");
            _freqStart = parseConfigValueFromDoc<uint16_t>(doc, "freq_start");
            _freqOvercl = parseConfigValueFromDoc<uint16_t>(doc, "freq_overcl");
            _freqExit = parseConfigValueFromDoc<uint16_t>(doc, "freq_exit");
            _stepInc = parseConfigValueFromDoc<uint16_t>(doc, "step_inc");
            _stepDec = parseConfigValueFromDoc<uint16_t>(doc, "step_dec");
            _tempMaxStable = parseConfigValueFromDoc<uint16_t>(doc, "temp_max_stable");
            _tempMax = parseConfigValueFromDoc<uint16_t>(doc, "temp_max");
            _tempCrit = parseConfigValueFromDoc<uint16_t>(doc, "temp_crit");
            _coresStart = parseConfigValueFromDoc<uint16_t>(doc, "cores_start");
            _coresMaxStable = parseConfigValueFromDoc<uint16_t>(doc, "cores_max_stable");
            if ( !(_path = r2d9_c_str_copy_non_empty(path)) ) {
                throw std::bad_alloc();
            }
        } catch (...) {
            clear();
            throw;
        }
    }
    
    Config::~Config() noexcept {
        clear();
    }
    
} // namespace ctm
