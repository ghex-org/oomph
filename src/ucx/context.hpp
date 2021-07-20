/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#pragma once

#include <oomph/context.hpp>
#include "../unique_ptr_set.hpp"
#include "./region.hpp"
#include <hwmalloc/register.hpp>
#include <hwmalloc/register_device.hpp>
#include <hwmalloc/heap.hpp>

//#include "../context.hpp"
//#include "./communicator.hpp"
//#include "../communicator.hpp"
//#include "../util/pthread_spin_mutex.hpp"
#include "./worker.hpp"

#ifdef OOMPH_USE_PMI
// use the PMI interface ...
#include "./address_db_pmi.hpp"
#else
// ... and go to MPI if not available
#include "./address_db_mpi.hpp"
#endif
#include "./address_db.hpp"

#include <vector>
#include <memory>

namespace oomph
{
class context_impl
{
  public: // member types
    using region_type = region;
    using device_region_type = region;
    using heap_type = hwmalloc::heap<context_impl>;
    using rank_type = communicator::rank_type;
    using worker_type = worker_t;
    //using communicator_type = tl::communicator<communicator>;

  private: // member types
    //using mutex_t = pthread_spin::recursive_mutex;

    struct ucp_context_h_holder
    {
        ucp_context_h m_context;
        ~ucp_context_h_holder() { ucp_cleanup(m_context); }
    };

    using worker_vector = std::vector<std::unique_ptr<worker_type>>;

  private: // members
    MPI_Comm m_mpi_comm;
    //const mpi::rank_topology&    m_rank_topology;
    type_erased_address_db_t           m_db;
    ucp_context_h_holder               m_context;
    heap_type                          m_heap;
    unique_ptr_set<communicator::impl> m_comms_set;

    std::size_t                  m_req_size;
    std::unique_ptr<worker_type> m_worker; // shared, serialized - per rank
    //worker_vector                m_workers; // per thread
    //mutex_t                      m_mutex;

    friend class worker_t;

  public: // ctors
    context_impl(MPI_Comm mpi_c)
    : m_mpi_comm
    {
        mpi_c
    }
#if defined OOMPH_USE_PMI
    , m_db(address_db_pmi(mpi_c))
#else
    , m_db(address_db_mpi(mpi_c))
#endif
          ,
        m_heap{this}
    {
        // read run-time context
        ucp_config_t* config_ptr;
        OOMPH_CHECK_UCX_RESULT(ucp_config_read(NULL, NULL, &config_ptr));

        // set parameters
        ucp_params_t context_params;
        // define valid fields
        context_params.field_mask =
            UCP_PARAM_FIELD_FEATURES            // features
            | UCP_PARAM_FIELD_REQUEST_SIZE      // size of reserved space in a non-blocking request
            | UCP_PARAM_FIELD_TAG_SENDER_MASK   // mask which gets sender endpoint from a tag
            | UCP_PARAM_FIELD_MT_WORKERS_SHARED // multi-threaded context: thread safety
            | UCP_PARAM_FIELD_ESTIMATED_NUM_EPS // estimated number of endpoints for this context
            //| UCP_PARAM_FIELD_REQUEST_INIT      // initialize request memory
            ;

        // features
        context_params.features = UCP_FEATURE_TAG   // tag matching
                                  | UCP_FEATURE_RMA // RMA access support
            ;
        // additional usable request size
        context_params.request_size = 128; //request_data_size::value;
        // thread safety
        // this should be true if we have per-thread workers,
        // otherwise, if one worker is shared by all thread, it should be false
        // requires benchmarking.
        context_params.mt_workers_shared = true;
        // estimated number of connections
        // affects transport selection criteria and theresulting performance
        context_params.estimated_num_eps = m_db.est_size();
        // mask
        // mask which specifies particular bits of the tag which can uniquely identify
        // the sender (UCP endpoint) in tagged operations.
        //context_params.tag_sender_mask  = 0x00000000fffffffful;
        context_params.tag_sender_mask = 0xfffffffffffffffful;
        // needed to zero the memory region. Otherwise segfaults occured
        // when a std::function destructor was called on an invalid object
        //context_params.request_init = &request_init;

        // initialize UCP
        OOMPH_CHECK_UCX_RESULT(ucp_init(&context_params, config_ptr, &m_context.m_context));
        ucp_config_release(config_ptr);

        // check the actual parameters
        ucp_context_attr_t attr;
        attr.field_mask = UCP_ATTR_FIELD_REQUEST_SIZE | // internal request size
                          UCP_ATTR_FIELD_THREAD_MODE;   // thread safety
        ucp_context_query(m_context.m_context, &attr);
        m_req_size = attr.request_size;
        if (attr.thread_mode != UCS_THREAD_MODE_MULTI)
            throw std::runtime_error("ucx cannot be used with multi-threaded context");

        // make shared worker
        // use single-threaded UCX mode, as per developer advice
        // https://github.com/openucx/ucx/issues/4609
        m_worker.reset(new worker_type{
            get(), m_db/*, m_mutex*/, UCS_THREAD_MODE_SINGLE /*, m_rank_topology*/});

        // intialize database
        m_db.init(m_worker->address());
    }

    ~context_impl()
    {
        //    // ucp_worker_destroy should be called after a barrier
        //    // use MPI IBarrier and progress all workers
        //    MPI_Request req = MPI_REQUEST_NULL;
        //    int         flag;
        //    MPI_Ibarrier(m_mpi_comm, &req);
        //    while (true)
        //    {
        //        // make communicators from workers and progress
        //        for (auto& w_ptr : m_workers) communicator_type{m_worker.get(), w_ptr.get()}.progress();
        //        communicator_type{m_worker.get(), m_worker.get()}.progress();
        //        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        //        if (flag) break;
        //    }
        //    // close endpoints
        //    for (auto& w_ptr : m_workers) w_ptr->m_endpoint_cache.clear();
        m_worker->m_endpoint_cache.clear();
        // another MPI barrier to be sure
        MPI_Barrier(m_mpi_comm);
    }

    context_impl(context_impl&&) = delete;
    context_impl& operator=(context_impl&&) = delete;

    //public: // ctors
    //  template<typename DB>
    //  transport_context(const mpi::rank_topology& t, DB&& db)
    //  : m_mpi_comm{t.mpi_comm()}
    //  , m_rank_topology{t}
    //  , m_db{std::forward<DB>(db)}
    //  {
    //  }

    //MPI_Comm mpi_comm() const noexcept { return m_mpi_comm; }

    //communicator_type get_serial_communicator() { return {m_worker.get(), m_worker.get()}; }

    //communicator_type get_communicator()
    //{
    //    std::lock_guard<mutex_t> lock(
    //        m_mutex); // we need to guard only the insertion in the vector,
    //                  // but this is not a performance critical section
    //    m_workers.push_back(std::make_unique<worker_type>(
    //        get(), m_db, m_mutex, UCS_THREAD_MODE_SERIALIZED, m_rank_topology));
    //    return {m_worker.get(), m_workers[m_workers.size() - 1].get()};
    //}

    //rank_type     rank() const { return m_db.rank(); }
    //rank_type     size() const { return m_db.size(); }
    
    ucp_context_h get() const noexcept { return m_context.m_context; }

    region make_region(void* ptr, std::size_t size, bool gpu = false)
    {
        return {get(), ptr, size, gpu};
    }

    auto& get_heap() noexcept { return m_heap; }

    communicator::impl* get_communicator();

    void deregister_communicator(communicator::impl* c) { m_comms_set.remove(c); }
};

} // namespace oomph
