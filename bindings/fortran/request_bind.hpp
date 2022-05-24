#ifndef INCLUDED_OOMPH_FORTRAN_REQUEST_BIND_HPP
#define INCLUDED_OOMPH_FORTRAN_REQUEST_BIND_HPP

#include <cstdint>
#include <oomph/request.hpp>
#include <bindings/fortran/oomph_sizes.hpp>

namespace oomph {
    namespace fort {        
        struct frequest_type {
            int8_t data[OOMPH_REQUEST_SIZE];
            bool recv_request;
        };
    }
}

#endif /* INCLUDED_OOMPH_FORTRAN_REQUEST_BIND_HPP */
