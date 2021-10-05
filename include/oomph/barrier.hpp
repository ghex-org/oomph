/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <oomph/communicator.hpp>
#include <atomic>

namespace oomph
{
/**
The barrier object synchronize threads or ranks, or both. When synchronizing
ranks, it also progress the communicator.

This facility id provided as a debugging tool, or as a seldomly used operation.
Halo-exchanges doe not need, in general, to call barriers.

Note on the implementation:

The implementation follows a two-counter approach:
First: one (atomic) counter is increased to the number of threads participating.
Second: one (atomic) counter is decreased from the numner of threads

The global barrier performs the up-counting while the thread that reaches the
final value also perform the rank-barrier. After that the downward count is
performed as usual.

This is why the barrier is split into is_node1 and in_node2. in_node1 returns
true to the thread selected to run the rank_barrier in the full barrier.
*/
class barrier
{
  private: // members
    std::size_t                 m_threads;
    mutable std::atomic<size_t> b_count{0};
    mutable std::atomic<size_t> b_count2;

    friend class test_barrier;

  public: // ctors
    barrier(size_t n_threads = 1);
    barrier(const barrier&) = delete;
    barrier(barrier&&) = delete;

  public: // public member functions
    int size() const noexcept { return m_threads; }

    /**
     * This is the most general barrier, it synchronize threads and ranks.
     *
     * @param comm The communicator object associated with the thread.
     */
    void operator()(communicator& comm) const;

    /**
     * This function can be used to synchronize ranks.
     * Only one thread per rank must call this function.
     * If other threads exist, they hace to be synchronized separately,
     * maybe using the in_node function.
     *
     * @param comm The communicator object associated with the thread.
     */
    void rank_barrier(communicator& comm) const;

    /**
     * This function synchronize the threads in a rank. The number of threads that need to participate
     * is indicated in the construction of the barrier object, whose reference is shared among the
     * participating threads.
     *
     * @param comm The communicator object associated with the thread.
     */
    void in_node(communicator& comm) const
    {
        in_node1(comm);
        in_node2(comm);
    }

  private:
    bool in_node1(communicator& comm) const;

    void in_node2(communicator& comm) const;
};

} // namespace oomph
