/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_TRIO_HPP__
#define __R2D9_TRIO_HPP__ 1

#include <algorithm>

namespace r2d9 {
    
    // POD(Plain Old Data)
    template<typename T1, typename T2, typename T3>
    struct TrioPOD final {
        T1 first;
        T2 second;
        T3 third;
    };
    
    //static_assert(std::is_pod<TrioPOD<..., ..., ...> >::value, "POD");
    //static_assert(std::is_standard_layout<TrioPOD<..., ..., ...> >::value, "standard layout");
    //static_assert(std::is_trivial<TrioPOD<..., ..., ...> >::value, "trivial");
    
} // namespace r2d9

//Number    Group name
//  3         trio
//  4        quartet
//  5        quintet
//  6        sextet
//  7        septet
//  8         octet
//  9         nonet

#endif //!__R2D9_TRIO_HPP__
