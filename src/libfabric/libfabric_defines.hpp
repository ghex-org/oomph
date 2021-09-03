#pragma once

#if __has_include(<hpx/config/parcelport_defines.hpp>)
#include <hpx/config/parcelport_defines.hpp>
#elif __has_include("oomph_libfabric_defines.hpp")
#include "oomph_libfabric_defines.hpp"
#endif
