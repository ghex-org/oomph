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
#include <vector>

int
main(int argc, char** argv)
{
    using namespace oomph;
    using message = oomph::message_buffer<char>;

    mpi_environment env(argc, argv);
    if (env.size != 2) return exit(argv[0]);

    args cmd_args(argc, argv);
    if (!cmd_args) return exit(argv[0]);

    context ctxt(MPI_COMM_WORLD);
    barrier b(cmd_args.num_threads);
    timer   t0;
    timer   t1;

    const auto inflight = cmd_args.inflight;
    const auto num_threads = cmd_args.num_threads;
    const auto buff_size = cmd_args.buff_size;
    const auto niter = cmd_args.n_iter;

#ifdef OOMPH_BENCHMARKS_MT
#pragma omp parallel
#endif
    {
        auto       comm = ctxt.get_communicator();
        const auto rank = comm.rank();
        const auto size = comm.size();
        const auto thread_id = THREADID;
        const auto peer_rank = (rank + 1) % size;

        if (thread_id == 0 && rank == 0)
        { std::cout << "\n\nrunning test " << __FILE__ << "\n\n"; };

        std::vector<message>      smsgs(inflight);
        std::vector<message>      rmsgs(inflight);
        std::vector<send_request> sreqs(inflight);
        std::vector<recv_request> rreqs(inflight);
        for (int j = 0; j < cmd_args.inflight; j++)
        {
            smsgs[j] = comm.make_buffer<char>(buff_size);
            rmsgs[j] = comm.make_buffer<char>(buff_size);
            for (auto& c : smsgs[j]) c = 0;
            for (auto& c : rmsgs[j]) c = 0;
        }

        b(comm);

        int       dbg = 0;
        int       sent = 0, received = 0;
        int       last_received = 0;
        int       last_sent = 0;
        const int delta_i = niter / 10;

        if (thread_id == 0)
        {
            if (rank == 0) std::cout << "number of threads: " << num_threads << "\n";
            t0.tic();
            t1.tic();
        }

        while (sent < niter || received < niter)
        {
            if (thread_id == 0 && dbg >= delta_i)
            {
                dbg = 0;
                std::cout << rank << " total bwdt MB/s:      "
                          << ((double)(received - last_received + sent - last_sent) * size *
                                 buff_size / 2) /
                                 t0.stoc()
                          << "\n";
                t0.tic();
                last_received = received;
                last_sent = sent;
            }

            /* submit comm */
            for (int j = 0; j < inflight; j++)
            {
                dbg += num_threads;
                sent += num_threads;
                received += num_threads;

                rreqs[j] = comm.recv(rmsgs[j], peer_rank, thread_id * inflight + j);
                sreqs[j] = comm.send(smsgs[j], peer_rank, thread_id * inflight + j);
            }

            /* wait for all */
            for (int j = 0; j < inflight; j++)
            {
                sreqs[j].wait();
                rreqs[j].wait();
            }
        }

        b(comm);

        if (thread_id == 0 && rank == 0)
        {
            const auto t = t1.toc();
            std::cout << "time:                   " << t / 1000000 << "s\n";
            std::cout << "final MB/s:             " << (double)(niter * size * buff_size) / t
                      << std::endl;
        }
    }

    // tail loops - not needed in wait benchmarks

    return 0;
}
