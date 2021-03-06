/// @file
/// @author uentity
/// @date 15.03.2017
/// @brief 
/// @copyright
/// This Source Code Form is subject to the terms of the Mozilla Public License,
/// v. 2.0. If a copy of the MPL was not distributed with this file,
/// You can obtain one at https://mozilla.org/MPL/2.0/

#pragma once

#include "array.h"
//#include "../kernel.h"
#include "serialize/decl.h"
#include "serialize/macro.h"
#if defined(BSPY_EXPORTING) || defined(BSPY_EXPORTING_PLUGIN)
#include "../python/nparray.h"
#endif

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/polymorphic_oarchive.hpp>

#define BS_ARRAY_GUID_VALUE(T, cont_traits) \
BLUE_SKY_TYPE_SERIALIZE_GUID_EXT(blue_sky::bs_array, 2, (T, blue_sky::cont_traits))

BLUE_SKY_TYPE_SERIALIZE_DECL_EXT(blue_sky::bs_array, 2, (class, template< class > class))

BLUE_SKY_CLASS_SRZ_FCN_DECL_EXT(serialize, blue_sky::bs_array, 2, (class, template< class > class))

BS_ARRAY_GUID_VALUE(int, vector_traits)
BS_ARRAY_GUID_VALUE(unsigned int, vector_traits)
BS_ARRAY_GUID_VALUE(std::intmax_t, vector_traits)
BS_ARRAY_GUID_VALUE(std::uintmax_t, vector_traits)
BS_ARRAY_GUID_VALUE(float, vector_traits)
BS_ARRAY_GUID_VALUE(double, vector_traits)

BS_ARRAY_GUID_VALUE(int, bs_vector_shared)
BS_ARRAY_GUID_VALUE(unsigned int, bs_vector_shared)
BS_ARRAY_GUID_VALUE(std::intmax_t, bs_vector_shared)
BS_ARRAY_GUID_VALUE(std::uintmax_t, bs_vector_shared)
BS_ARRAY_GUID_VALUE(float, bs_vector_shared)
BS_ARRAY_GUID_VALUE(double, bs_vector_shared)

#if defined(BSPY_EXPORTING) || defined(BSPY_EXPORTING_PLUGIN)
BS_ARRAY_GUID_VALUE(int                , bs_nparray_traits)
BS_ARRAY_GUID_VALUE(unsigned int       , bs_nparray_traits)
BS_ARRAY_GUID_VALUE(std::intmax_t      , bs_nparray_traits)
BS_ARRAY_GUID_VALUE(std::uintmax_t     , bs_nparray_traits)
BS_ARRAY_GUID_VALUE(float              , bs_nparray_traits)
BS_ARRAY_GUID_VALUE(double             , bs_nparray_traits)
#endif

