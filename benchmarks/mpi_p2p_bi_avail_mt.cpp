/* 
 * GridTools
 * 
 * Copyright (c) 2014-2020, ETH Zurich
 * All rights reserved.
 * 
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */
#include <iostream>
#include <mpi.h>
#include <string.h>
#include <atomic>

#include "./timer.hpp"
#include "./utils.hpp"

#ifdef OOMPH_BENCHMARKS_MT
#include <omp.h>
#endif /* OOMPH_BENCHMARKS_MT */

int main(int argc, char *argv[])
{
    using namespace oomph;

    int rank, size, peer_rank;
    int niter, buff_size;
    int inflight;
    timer t0, t1;

    int last_received = 0;
    int last_sent = 0;
#ifdef OOMPH_BENCHMARKS_MT
    std::atomic<int> sent(0);
    std::atomic<int> received(0);
    int is_mt = 1;
#else
    int sent = 0;
    int received = 0;
    int is_mt = 0;
#endif /* OOMPH_BENCHMARKS_MT */

    if(argc != 4){
        std::cerr << "Usage: bench [niter] [msg_size] [inflight]" << "\n";
        std::terminate();
    }
    niter = atoi(argv[1]);
    buff_size = atoi(argv[2]);
    inflight = atoi(argv[3]);
    
#ifdef OOMPH_BENCHMARKS_MT
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &is_mt);
    if(is_mt != MPI_THREAD_MULTIPLE){
        std::cerr << "MPI_THREAD_MULTIPLE not supported by MPI, aborting\n";
        std::terminate();
    }
    is_mt = 1;
#else
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SINGLE, &is_mt);
    is_mt = 0;
#endif

#ifdef OOMPH_BENCHMARKS_MT
#pragma omp parallel
#endif
    {
        int thrid = 0, nthr = 1;
        MPI_Comm mpi_comm = MPI_COMM_NULL;
        unsigned char **sbuffers = new unsigned char *[inflight];
        unsigned char **rbuffers = new unsigned char *[inflight];
        MPI_Request *sreq = new MPI_Request[inflight];
        MPI_Request *rreq = new MPI_Request[inflight];
        
#ifdef OOMPH_BENCHMARKS_MT
#pragma omp master
#endif
	{
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            peer_rank = (rank+1)%2;
            if(rank==0) std::cout << "\n\nrunning test " << __FILE__ << "\n\n";
        }

#ifdef OOMPH_BENCHMARKS_MT
        thrid = omp_get_thread_num();
        nthr = omp_get_num_threads();
#endif

        /* duplicate the communicator - all threads in order */
        for(int tid=0; tid<nthr; tid++){
            if(thrid==tid) {
                MPI_Comm_dup(MPI_COMM_WORLD, &mpi_comm);
            }
#ifdef OOMPH_BENCHMARKS_MT
#pragma omp barrier
#endif
        }
        
        for(int j=0; j<inflight; j++){
            MPI_Alloc_mem(buff_size, MPI_INFO_NULL, &sbuffers[j]);
            MPI_Alloc_mem(buff_size, MPI_INFO_NULL, &rbuffers[j]);
            memset(sbuffers[j], 1, buff_size);
            memset(rbuffers[j], 1, buff_size);
            sreq[j] = MPI_REQUEST_NULL;
            rreq[j] = MPI_REQUEST_NULL;
        }

	if(thrid==0) MPI_Barrier(mpi_comm);
#ifdef OOMPH_BENCHMARKS_MT
#pragma omp barrier
#endif

	if(thrid==0) {
            t0.tic();
            t1.tic();
            if(rank == 1) std::cout << "number of threads: " << nthr << ", multi-threaded: " << is_mt << "\n";
        }

        /* pre-post - required for testany to work */
        for(int j=0; j<inflight; j++){
            MPI_Irecv(rbuffers[j], buff_size, MPI_BYTE, peer_rank, thrid*inflight+j, mpi_comm, &rreq[j]);
            MPI_Isend(sbuffers[j], buff_size, MPI_BYTE, peer_rank, thrid*inflight+j, mpi_comm, &sreq[j]);
        }

        int dbg = 0, sdbg = 0, rdbg = 0, j;
        int lsent = 0, lrecv = 0;
#ifdef USE_TESTANY
	int flag;
#endif	
        char header[256];
        snprintf(header, 256, "%d total bwdt ", rank);
        while(sent<niter || received<niter){

            if(thrid==0 && sdbg>=(niter/10)) {
                std::cout << rank << "   " << sent << " sent\n";
                sdbg = 0;
            }

            if(thrid==0 && rdbg>=(niter/10)) {
                std::cout << rank << "   " << received << " received\n";
                rdbg = 0;
            }

            if(thrid == 0 && dbg >= (2*niter/10)) {
                dbg = 0;
                t0.vtoc(header, (double)(received-last_received + sent-last_sent)*size*buff_size/2);
                t0.tic();
                last_received = received;
                last_sent = sent;
            }

#ifdef USE_TESTANY
            MPI_Testany(inflight, rreq, &j, &flag, MPI_STATUS_IGNORE);
            if(flag) {
                MPI_Irecv(rbuffers[j], buff_size, MPI_BYTE, peer_rank, thrid*inflight+j, mpi_comm, &rreq[j]);
                dbg += nthr;
                rdbg += nthr;
                received++;
                lrecv++;
            }

            if(lsent < lrecv+2*inflight && sent<niter){
                MPI_Testany(inflight, sreq, &j, &flag, MPI_STATUS_IGNORE);
                if(flag) {
                    MPI_Isend(sbuffers[j], buff_size, MPI_BYTE, peer_rank, thrid*inflight+j, mpi_comm, &sreq[j]);
                    dbg += nthr;
                    sdbg += nthr;
                    sent++;
                    lsent++;
                }
            }
#else
            for(int i=0; i<inflight; i++){
                MPI_Test(&rreq[i], &j, MPI_STATUS_IGNORE);
                if(j){
                    MPI_Irecv(rbuffers[i], buff_size, MPI_BYTE, peer_rank, thrid*inflight+i, mpi_comm, &rreq[i]);
                    dbg += nthr;
                    rdbg += nthr;
                    received++;
                    lrecv++;
                }
            }
            if(lsent < lrecv+2*inflight){
                for(int i=0; i<inflight; i++){
                    MPI_Test(&sreq[i], &j, MPI_STATUS_IGNORE);
                    if(j){
                        MPI_Isend(sbuffers[i], buff_size, MPI_BYTE, peer_rank, thrid*inflight+i, mpi_comm, &sreq[i]);
                        dbg += nthr;
                        sdbg += nthr;
                        sent++;
                        lsent++;
                    }
                }
            }
#endif
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == 1) {
      t1.vtoc();
      t1.vtoc("final ", (double)niter*size*buff_size);
    } 

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}
