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
#include <iostream>
#include <vector>

#ifdef OOMPH_BENCHMARKS_MT
#define THREADID omp_get_thread_num()
#else
#define THREADID 0
#endif

int
exit(char const* executable)
{
    std::cerr << "Usage: " << executable << " [niter] [msg_size] [inflight]" << std::endl;
    std::cerr << "       run with 2 MPI processes: e.g.: mpirun -np 2 ..." << std::endl;
    return 1;
}

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

        std::vector<message> smsgs(cmd_args.inflight);
        std::vector<message> rmsgs(cmd_args.inflight);
        std::vector<request> sreqs(cmd_args.inflight);
        std::vector<request> rreqs(cmd_args.inflight);
        for (int j = 0; j < cmd_args.inflight; j++)
        {
            smsgs[j] = comm.make_buffer<char>(cmd_args.buff_size);
            rmsgs[j] = comm.make_buffer<char>(cmd_args.buff_size);
            for (auto& c : smsgs[j]) c = 0;
            for (auto& c : rmsgs[j]) c = 0;
        }

        b(comm);

        int       dbg = 0;
        const int max_i = cmd_args.n_iter / cmd_args.num_threads;
        const int delta_i = max_i / 10;

        if (thread_id == 0)
        {
            if (rank == 0) std::cout << "number of threads: " << cmd_args.num_threads << "\n";
            t0.tic();
            t1.tic();
        }

        for (int i = 0; i < max_i; ++i, ++dbg)
        {
            if (thread_id == 0 && dbg == delta_i)
            {
                dbg = 0;
                std::cout << rank << " total bwdt MB/s:      "
                          << ((double)(delta_i * cmd_args.num_threads) * size *
                                 cmd_args.buff_size) /
                                 t0.toc()
                          << "\n";
                t0.tic();
            }

            /* submit comm */
            for (int j = 0; j < cmd_args.inflight; j++)
            {
                rreqs[j] = comm.recv(rmsgs[j], peer_rank, thread_id * cmd_args.inflight + j);
                sreqs[j] = comm.send(smsgs[j], peer_rank, thread_id * cmd_args.inflight + j);
            }

            /* wait for all */
            for (int j = 0; j < cmd_args.inflight; j++)
            {
                sreqs[j].wait();
                rreqs[j].wait();
            }
        }

        if (thread_id == 0 && rank == 0)
        {
            const auto t = t1.toc();
            std::cout << "time:                   " << t / 1000000 << "s\n";
            std::cout << "final MB/s:             "
                      << ((double)(max_i * cmd_args.num_threads) * size * cmd_args.buff_size) / t
                      << "\n";
        }
    }

    return 0;
}
