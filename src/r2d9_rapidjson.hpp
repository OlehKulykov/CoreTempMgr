/*
 * Copyright (C) 2018 - 2026 Oleh Kulykov <olehkulykov@gmail.com>
 * All Rights Reserved.
 *
 * Unauthorized copying of this file, transferring or reproduction of the
 * contents of this project, via any medium, is strictly prohibited.
 * The contents of this project are proprietary and confidential.
 */

#ifndef __R2D9_RAPIDJSON_HPP__
#define __R2D9_RAPIDJSON_HPP__ 1

#include <memory>

#include "r2d9.hpp"

#define RAPIDJSON_NAMESPACE r2d9rj
#define RAPIDJSON_NO_SIZETYPEDEFINE 1
#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#define RAPIDJSON_HAS_CXX11_NOEXCEPT 1

namespace RAPIDJSON_NAMESPACE {
    typedef ::size_t SizeType;
}

#include "rapidjson/document.h"

namespace RAPIDJSON_NAMESPACE {
    
    typedef RAPIDJSON_NAMESPACE::GenericDocument<RAPIDJSON_NAMESPACE::UTF8<>, RAPIDJSON_NAMESPACE::CrtAllocator> RJDocument;
    typedef RAPIDJSON_NAMESPACE::GenericValue<RAPIDJSON_NAMESPACE::UTF8<>, RAPIDJSON_NAMESPACE::CrtAllocator> RJValue;
    
} // namespace RAPIDJSON_NAMESPACE

#endif //!__R2D9_RAPIDJSON_HPP__
