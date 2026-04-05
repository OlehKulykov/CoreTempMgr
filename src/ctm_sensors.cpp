/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#include <stdexcept>
#include <cstring>

#include "r2d9.hpp"
#include "ctm_sensors.hpp"

#if !defined(__APPLE__)
#  include <sensors/sensors.h>
#  include <sensors/error.h>
#endif

namespace ctm {

#if !defined(__APPLE__)    
    void Sensors::cleanup() noexcept {
        if (_initialized) {
            ::sensors_cleanup();
            ::memset(&_sensor, 0, sizeof(struct Sensor));
            _initialized = false;
        }
    }
    
    void Sensors::init() {
        cleanup();
        
        const int err = ::sensors_init(nullptr);
        if (err != 0) {
            ::sensors_cleanup();
            char reason[128];
            ::snprintf(reason, 128, "Error init sensors: %i", err);
            throw std::runtime_error(reason);
        }
        
        const sensors_chip_name * chip = nullptr;
        int chipNum = 0;
        while ( (chip = ::sensors_get_detected_chips(nullptr, &chipNum)) != nullptr ) {
            // char chipName[256];
            // ::sensors_snprintf_chip_name(chipName, sizeof(chip_name), chip);
            // ::printf("Chip: %s, chipNum: %i, buss.type: %i\n", chipName, chipNum, (int)chip->bus.type);
            const sensors_feature * feature = nullptr;
            int featureNum = 0;
            while ( (feature = ::sensors_get_features(chip, &featureNum)) != nullptr ) {
                const sensors_subfeature * subfeature = nullptr;
                int subFeatureNum = 0;
                while ( (subfeature = ::sensors_get_all_subfeatures(chip, feature, &subFeatureNum)) != nullptr ) {
                    if ((subfeature->type == SENSORS_SUBFEATURE_TEMP_INPUT) &&
                        (feature->type == SENSORS_FEATURE_TEMP) &&
                        (chip->bus.type == SENSORS_BUS_TYPE_PCI)) {
                        _sensor.chipNumber = chipNum - 1;
                        _sensor.busType = chip->bus.type;
                        _sensor.featureNumber = featureNum - 1;
                        _sensor.featureType = static_cast<int>(feature->type);
                        _sensor.subFeatureNumber = subFeatureNum - 1;
                        _sensor.subFeatureType = static_cast<int>(subfeature->type);
                        _initialized = true;
                        return;
                    }
                    // double val;
                    // if (::sensors_get_value(chip, subfeature->number, &val) == 0) { ::printf("  %s: %.2f°C feature_nr: %i, feature.type: %i, sub-feature.type: %i, subfeature_nr: %i\n", label, val, feature_nr, (int)feature->type, (int)subfeature->type, subfeature_nr); }
                }
            }
        }
        
        ::sensors_cleanup();
        throw std::runtime_error("Sensors not found");
    }
    
    double Sensors::cpuTemp() const {
        if (_initialized) {
            int tmpNum = _sensor.chipNumber;
            const sensors_chip_name * chip = ::sensors_get_detected_chips(nullptr, &tmpNum);
            if (!chip || (chip->bus.type != _sensor.busType)) {
                throw std::runtime_error("Broken chip");
            }
            
            tmpNum = _sensor.featureNumber;
            const sensors_feature * feature = ::sensors_get_features(chip, &tmpNum);
            if (!feature || (feature->type != static_cast<sensors_feature_type>(_sensor.featureType))) {
                throw std::runtime_error("Broken feature");
            }
            
            tmpNum = _sensor.subFeatureNumber;
            const sensors_subfeature * subfeature = ::sensors_get_all_subfeatures(chip, feature, &tmpNum);
            if (!subfeature || (subfeature->type != static_cast<sensors_subfeature_type>(_sensor.subFeatureType))) {
                throw std::runtime_error("Broken feature");
            }
            
            double val = 0.0;
            if (::sensors_get_value(chip, subfeature->number, &val) == 0) {
                return val;
            }
            
            throw std::runtime_error("Error reading the value of a subfeature");
        }
        
        throw std::runtime_error("Not initialized");
    }
#else
    void Sensors::cleanup() noexcept { }
    void Sensors::init() { }
    double Sensors::cpuTemp() const { return std::numeric_limits<double>::min(); }
#endif
    
    Sensors::~Sensors() noexcept {
        cleanup();
    }
    
} // namespace ctm
