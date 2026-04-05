/*
 * Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __CTM_SENSORS_HPP__
#define __CTM_SENSORS_HPP__ 1

#include <limits>

#include "r2d9.hpp"

namespace ctm {
    
    class Sensors final {
    private:
        struct Sensor {
            int chipNumber;
            int busType;
            int featureNumber;
            int featureType;
            int subFeatureNumber;
            int subFeatureType;
        };
        struct Sensor _sensor;
        bool _initialized = false;
        
    public:
        inline bool initialized() const noexcept { return _initialized; }
        
        double cpuTemp() const;
        
        void init();
        
        void cleanup() noexcept;
        
        Sensors() noexcept = default;
        ~Sensors() noexcept;
    };
    
} // namespace ctm

#endif //!__CTM_SENSORS_HPP__
