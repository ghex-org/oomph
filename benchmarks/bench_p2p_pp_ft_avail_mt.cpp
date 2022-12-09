/*
 * GridTools
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/context.hpp>
#include <oomph/barrier.hpp>
#include "./mpi_environment.hpp"
#include "./args.hpp"
#include "./timer.hpp"
#include "./utils.hpp"
//
#include <atomic>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#ifdef USE_OPENMP
#include <omp.h>
#endif

// enable cleaned up debugging output
// clang-format off
#define buffered_out(x) {    \
    std::stringstream temp;  \
    temp << x << std::endl;  \
    std::cout << temp.str(); }
#define buffered_err(x) {    \
    std::stringstream temp;  \
    temp << x << std::endl;  \
    std::cerr << temp.str(); }
// clang-format on

std::string
print_send_recv_info(std::tuple<int, int, int, int, int, int>& tup)
{
    std::stringstream temp;
    temp << " Sends Posted " << std::get<0>(tup) << " Sends Completed " << std::get<1>(tup)
         << " Sends Readied " << std::get<2>(tup) << " Recvs Posted " << std::get<3>(tup)
         << " Recvs Completed " << std::get<4>(tup) << " Recvs Readied " << std::get<5>(tup) << " ";
    return temp.str();
}

const char* syncmode = "future";
const char* waitmode = "avail";

std::atomic<int> sends_posted(0);
std::atomic<int> sends_completed(0);
std::atomic<int> receives_posted(0);

// keep track of sends on a thread local basis
template<typename Future>
struct alignas(64) msg_tracker
{
    using message = oomph::message_buffer<char>;
    std::vector<message> msgs;
    std::vector<Future>  reqs;
    //
    msg_tracker() = default;
    //
    void init(oomph::communicator& comm, int inflight, int buff_size)
    {
        msgs.resize(inflight);
        reqs.resize(inflight);
        //
        for (int j = 0; j < inflight; j++)
        {
            msgs[j] = comm.make_buffer<char>(buff_size);
            for (auto& c : msgs[j]) c = 0;
        }
    }
};

int
main(int argc, char* argv[])
{
    using namespace oomph;
    using message = oomph::message_buffer<char>;

    args cmd_args(argc, argv, true);
    if (!cmd_args) return exit(argv[0]);
    bool const multi_threaded = (cmd_args.num_threads > 1);

    mpi_environment env(multi_threaded, argc, argv);
    if (env.size != 2) return exit(argv[0]);

    context ctxt(MPI_COMM_WORLD, multi_threaded);
    barrier b(ctxt, cmd_args.num_threads);
    timer   t0;
    timer   t1;

    const auto inflight = cmd_args.inflight;
    const auto num_threads = cmd_args.num_threads;
    const auto n_secs = cmd_args.n_secs;
    const auto buff_size = cmd_args.buff_size;

    if (env.rank == 0)
    {
        std::cout << "inflight = " << inflight << std::endl;
        std::cout << "size     = " << buff_size << std::endl;
        std::cout << "S        = " << n_secs << std::endl;
    }

    // How often do we display debug msgs
    const int debug_freq = 5;
    const int msecond = (1000 * n_secs) / debug_freq;

    // true when time exceeded
    std::atomic<bool> time_up = false;
    // official start time of the test
    auto start = std::chrono::steady_clock::now();
    // last time debug display was shown
    auto dbg_time = start;

    // an atomic counter we use to track wne alll threads have finished sending
    // so we can post a "done sends" message
    std::atomic<int> threads_completed(0);
    // only one thread is reponsible for the "done sends" msg
    std::atomic<int>  master_thread(-1);
    std::atomic<bool> sends_complete_checked_flag = false;

    std::atomic<int> num_messages_expected = std::numeric_limits<int>::max() / 2;

    //    int mode;
    double       elapsed;
    oomph::timer ttimer;

#ifdef OOMPH_BENCHMARKS_MT
#pragma omp parallel
#endif
    {
        // ----------------------------------------------------------------
        // variables in parallel section are thread local
        // ----------------------------------------------------------------
        auto       comm = ctxt.get_communicator();
        const auto rank = comm.rank();
        const auto size = comm.size();
        const auto thread_id = THREADID;
        const auto peer_rank = (rank + 1) % size;

        // track sends/recvs
        msg_tracker<oomph::send_request> sends;
        msg_tracker<oomph::recv_request> recvs;
        sends.init(comm, inflight, buff_size);
        recvs.init(comm, inflight, buff_size);

        // when all threads have finished sending,
        // we use these to sync total msg count between ranks
        message      done_send = comm.make_buffer<char>(sizeof(int));
        message      done_recv = comm.make_buffer<char>(sizeof(int));
        send_request fsend;
        recv_request frecv;

        // NB. these are thread local
        bool thread_sends_complete = false;      // true when thread completed sends
        bool thread_sends_complete_flag = false; // true after thread signals counter

        // loop for allowed time : sending and receiving
        do {
            // just one thread checks timout and does debug printf's
            if (thread_id == 0)
            {
                auto now = std::chrono::steady_clock::now();
                if (!time_up) time_up = (now > start + std::chrono::seconds{n_secs});

                // output debug info at periodic intervals
                if (now > dbg_time + std::chrono::milliseconds{msecond})
                {
                    dbg_time = now;
                    buffered_out("rank: " << rank << " \tsend: " << sends_posted
                                          << "\t recv: " << receives_posted);
                }
            }

            comm.progress();

            // always pre-post receives for all slots
            for (int j = 0; j < inflight; j++)
                if (recvs.reqs[j].is_ready())
                {
                    recvs.reqs[j] = comm.recv(recvs.msgs[j], peer_rank, j);
                    receives_posted++;
                }

            comm.progress();

            // if time available and a send slot is available, post a send
            for (int j = 0; j < inflight; j++)
                if (sends.reqs[j].is_ready() &&
                    !time_up /*&& (sends_posted<receives_posted + (inflight*num_threads))*/)
                {
                    sends.reqs[j] = comm.send(sends.msgs[j], peer_rank, j);
                    ++sends_posted;
                }

            comm.progress();

            // if time is up, keep polling until all send futures are ready
            if (time_up)
            {
                // are there any incomplete sends on this thread
                thread_sends_complete = true;
                for (int j = 0; j < inflight; j++)
                {
                    thread_sends_complete = thread_sends_complete && sends.reqs[j].test();
                }

                // if this thread has completed its sends
                if (thread_sends_complete)
                {
                    // last thread to be ready sends a single "done" message
                    // containing total sent, receive the same from peer
                    if (!thread_sends_complete_flag)
                    {
                        // don't re-enter this section
                        thread_sends_complete_flag = true;
                        // only last thread to finish can trigger this
                        if (++threads_completed == num_threads)
                        {
                            // we are the master thread
                            master_thread = thread_id;
                            // pre-post recv for total incoming messages
                            frecv = comm.recv(done_recv, peer_rank, 0xffff);
                            buffered_out("rank: " << rank << " thread " << thread_id
                                                  << " bcast SENDS = " << sends_posted);
                            std::memcpy(done_send.data(), &sends_posted, sizeof(int));
                            fsend = comm.send(done_send, peer_rank, 0xffff);
                        }
                    }
                    // only master thread checks "done" messages
                    if (thread_id == master_thread && !sends_complete_checked_flag)
                    {
                        // our send has completed, and we received peer's
                        if (fsend.test() && frecv.test())
                        {
                            int* temp = reinterpret_cast<int*>(done_recv.data());
                            num_messages_expected.store(*temp);
                            buffered_out(
                                "rank: " << rank << " thread " << thread_id << " expecting "
                                         << num_messages_expected << " need receives "
                                         << num_messages_expected + inflight * num_threads);
                            // don't re-enter this section
                            sends_complete_checked_flag = true;
                        }
                    }
                }
            }
            // when the number of receives posted is equal to the
            // number of messages sent by the peer + (inflight*num_threads)
            // then all messages sent by them have been received.
        } while (!sends_complete_checked_flag ||
                 receives_posted != (num_messages_expected + inflight * num_threads));

//        buffered_out("rank: " << rank << "\tthread "
//                              << " Done" << thread_id << "\tsend: " << sends_posted
//                              << "\trecv: " << receives_posted);

        // barrier + progress here before final checks
        b.thread_barrier();

        // all ranks have completed sends/recvs : test is over, stop the clock
        // timing includes a few bits of synchronization overhead, but
        // when running for more than a few seconds will be negligable
        elapsed = ttimer.toc();

        // cancel outstanding pre-posted receives that we will not use
        for (int j = 0; j < inflight; j++)
        {
            if (!recvs.reqs[j].test())
            {
                if (recvs.reqs[j].cancel()) receives_posted--;
                else
                    throw std::runtime_error("Receive cancel failed");
            }
            else
            {
                //throw std::runtime_error("All receive futures should be ready");
                buffered_out("ERROR: late receive : rank: " << rank << "\tthread " << thread_id
                                                            << ", slot " << j);
            }
        }

        // sync threads here before final error checks
        b.thread_barrier();

        if (receives_posted != num_messages_expected)
        {
            buffered_err("rank: " << rank << " receives_posted " << receives_posted
                                  << " != " << num_messages_expected << " num_messages_expected");
            throw std::runtime_error("Final message count mismatch");
        }

        // total traffic is amount sends_posted in both directions
        if (rank == 0 && thread_id == 0)
        {
            double bw = ((double)(sends_posted + receives_posted) * buff_size) / elapsed;
            // clang-format off
            std::cout << "time:       " << elapsed/1000000 << "s\n";
            std::cout << "final MB/s: " << bw << "\n";
            std::cout << "CSVData"
                      << ", niter, " << sends_posted + receives_posted
                      << ", buff_size, " << buff_size
                      << ", inflight, " << inflight
                      << ", num_threads, " << num_threads
                      << ", syncmode, " << syncmode
                      << ", waitmode, " << waitmode
                      << ", transport, " << ctxt.get_transport_option("name")
                      << ", BW MB/s, " << bw
                      << ", progress, " << ctxt.get_transport_option("progress")
                      << ", endpoint, " << ctxt.get_transport_option("endpoint")
                      << "\n";
            // clang-format on
        }
    }

    return 0;
}
