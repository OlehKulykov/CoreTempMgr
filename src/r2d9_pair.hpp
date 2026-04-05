/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_PAIR_HPP__
#define __R2D9_PAIR_HPP__ 1

#include <algorithm>

#include "r2d9.hpp"

namespace r2d9 {
    
    // POD(Plain Old Data)
    template<typename T1, typename T2>
    struct PairPOD final {
        T1 first;
        T2 second;
    };
    
    // #include <type_traits>
    //static_assert(std::is_pod<PairPOD<..., ...> >::value, "POD");
    //static_assert(std::is_standard_layout<PairPOD<..., ...> >::value, "standard layout");
    //static_assert(std::is_trivial<PairPOD<..., ...> >::value, "trivial");
    
} // namespace r2d9

#endif //!__R2D9_PAIR_HPP__
