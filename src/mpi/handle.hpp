#pragma once

#include <oomph/util/mpi_error.hpp>

namespace oomph
{
struct handle
{
    using key_type = MPI_Aint;

    void*       m_ptr;
    std::size_t m_size;

    key_type get_remote_key() const noexcept
    {
        MPI_Aint address;
        OOMPH_CHECK_MPI_RESULT_NOEXCEPT(MPI_Get_address(m_ptr, &address));
        return address;
        //return ((char*)m_ptr - MPI_BOTTOM);
    }
};
} // namespace oomph
