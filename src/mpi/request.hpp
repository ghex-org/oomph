/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/request.hpp>
#include <oomph/util/mpi_error.hpp>

namespace oomph
{
class request::impl
{
  public:
    MPI_Request m_req;
    //bool        m_is_ready;
    //bool        m_send_req = true;

    impl() = default;
    //: m_is_ready(true)
    //{
    //}

    impl(MPI_Request req, bool /*send_req*/ = true)
    : m_req{req}
    //, m_is_ready(false)
    //, m_send_req(send_req)
    {
    }

    ~impl();

    bool is_ready()
    {
        //if (m_is_ready) return true;
        int flag;
        OOMPH_CHECK_MPI_RESULT(MPI_Test(&m_req, &flag, MPI_STATUS_IGNORE));
        return flag;
        //if (flag) m_is_ready = true;
        //return m_is_ready;
    }

    //void wait()
    //{
    //    if (m_is_ready) return;
    //    OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_req, MPI_STATUS_IGNORE));
    //    m_is_ready = true;
    //}

    bool cancel()
    {
        //if (m_send_req) return false;
        OOMPH_CHECK_MPI_RESULT(MPI_Cancel(&m_req));
        MPI_Status st;
        OOMPH_CHECK_MPI_RESULT(MPI_Wait(&m_req, &st));
        int flag = false;
        OOMPH_CHECK_MPI_RESULT(MPI_Test_cancelled(&st, &flag));
        return flag;
    }

    //bool is_recv() const noexcept { return !(m_send_req); }
};

//template<typename RandomAccessIterator>
//inline RandomAccessIterator
//test_any(RandomAccessIterator first, RandomAccessIterator last)
//{
//    const auto count = last - first;
//    if (count == 0) return last;
//    // should we handle null requests ourselves?
//    //for (auto it = first; it!=last; ++it)
//    //    if (*it==MPI_REQUEST_NULL)
//    //        return it;
//    // maybe static needed to avoid unnecessary allocations
//    static thread_local std::vector<MPI_Request> reqs;
//    reqs.resize(0);
//    reqs.reserve(count);
//    int indx, flag;
//    std::transform(first, last, std::back_inserter(reqs), [](auto& req) { return req.m_req; });
//    OOMPH_CHECK_MPI_RESULT(MPI_Testany(count, reqs.data(), &indx, &flag, MPI_STATUS_IGNORE));
//    if (flag && indx != MPI_UNDEFINED) return first + indx;
//    else
//        return last;
//}
//
//template<typename RandomAccessIterator, typename Func>
//inline RandomAccessIterator
//test_any(RandomAccessIterator first, RandomAccessIterator last, Func&& get)
//{
//    const auto count = last - first;
//    if (count == 0) return last;
//    static thread_local std::vector<MPI_Request> reqs;
//    reqs.resize(0);
//    reqs.reserve(count);
//    int indx, flag;
//    std::transform(first, last, std::back_inserter(reqs),
//        [g = std::forward<Func>(get)](auto& req) { return g(req).m_req; });
//    OOMPH_CHECK_MPI_RESULT(MPI_Testany(count, reqs.data(), &indx, &flag, MPI_STATUS_IGNORE));
//    if (flag && indx != MPI_UNDEFINED) return first + indx;
//    else
//        return last;
//}

template<typename Element>
void
partition(std::vector<Element>& queue, std::vector<Element>& ready_queue)
{
    static thread_local std::vector<MPI_Request> reqs;
    static thread_local std::vector<int>         indices;

    ready_queue.clear();
    const auto qs = queue.size();
    if (!qs) return;

    ready_queue.reserve(qs);
    reqs.resize(0);
    reqs.reserve(qs);
    indices.resize(qs + 1);

    std::transform(queue.begin(), queue.end(), std::back_inserter(reqs),
        [](auto& e) { return e.m_request.m_req; });

    int outcount;
    OOMPH_CHECK_MPI_RESULT(
        MPI_Testsome(qs, reqs.data(), &outcount, indices.data(), MPI_STATUSES_IGNORE));

    if (!outcount) return;

    indices[outcount] = qs;

    std::size_t k = 0;
    std::size_t j = 0;
    for (std::size_t i = 0; i < qs; ++i)
    {
        auto& e = queue[i];
        if ((int)i == indices[k])
        {
            ready_queue.push_back(std::move(e));
            ++k;
        }
        else if (i > j)
        {
            e.m_handle->m_index = j;
            queue[j] = std::move(e);
            ++j;
        }
        else
        {
            ++j;
        }
    }
    reqs.clear();
    queue.erase(queue.end() - ready_queue.size(), queue.end());
}

//template<>
//request::request<MPI_Request>(MPI_Request r);

} // namespace oomph
