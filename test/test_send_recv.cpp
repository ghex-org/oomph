/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <oomph/context.hpp>
#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"
#include <iostream>
#include <iomanip>
#include <thread>

#define NITERS   50
#define SIZE     64
#define NTHREADS 4

struct test_environment_base
{
    using rank_type = oomph::communicator::rank_type;
    using tag_type = oomph::communicator::tag_type;
    using message = oomph::message_buffer<rank_type>;

    oomph::context&     ctxt;
    oomph::communicator comm;
    message             smsg;
    message             rmsg;
    rank_type           speer_rank;
    rank_type           rpeer_rank;
    int                 thread_id;
    int                 num_threads;
    tag_type            tag;

    test_environment_base(oomph::context& c, std::size_t size, int tid, int num_t)
    : ctxt(c)
    , comm(ctxt.get_communicator())
    , smsg(comm.make_buffer<rank_type>(size))
    , rmsg(comm.make_buffer<rank_type>(size))
    , speer_rank((comm.rank() + 1) % comm.size())
    , rpeer_rank((comm.rank() + comm.size() - 1) % comm.size())
    , thread_id(tid)
    , num_threads(num_t)
    , tag(tid)
    {
    }
};

struct test_environment : public test_environment_base
{
    using base = test_environment_base;

    test_environment(oomph::context& c, std::size_t size, int tid, int num_t)
    : base(c, size, tid, num_t)
    {
        smsg = comm.make_buffer<rank_type>(size);
        rmsg = comm.make_buffer<rank_type>(size);
        fill_send_buffer();
        fill_recv_buffer();
    }

    void fill_send_buffer()
    {
        for (auto& x : smsg) x = comm.rank();
    }

    void fill_recv_buffer()
    {
        for (auto& x : rmsg) x = -1;
    }

    bool check_recv_buffer()
    {
        for (auto const& x : rmsg)
            if (x != rpeer_rank) return false;
        return true;
    }
};

#if HWMALLOC_ENABLE_DEVICE
struct test_environment_device : public test_environment_base
{
    using base = test_environment_base;

    test_environment_device(oomph::context& c, std::size_t size, int tid, int num_t)
    : base(c, size, tid, num_t)
    {
        smsg = comm.make_device_buffer<rank_type>(size, 0);
        rmsg = comm.make_device_buffer<rank_type>(size, 0);
        fill_send_buffer();
        fill_recv_buffer();
    }

    void fill_send_buffer()
    {
        for (auto& x : smsg) x = comm.rank();
        smsg.clone_to_device();
    }

    void fill_recv_buffer()
    {
        for (auto& x : rmsg) x = -1;
        rmsg.clone_to_device();
    }

    bool check_recv_buffer()
    {
        rmsg.clone_to_host();
        for (auto const& x : rmsg)
            if (x != rpeer_rank) return false;
        return true;
    }
};
#endif

template<typename Func>
void
launch_test(Func f)
{
    // single threaded
    {
        oomph::context ctxt(MPI_COMM_WORLD, false);
        f(ctxt, SIZE, 0, 1);
    }

    // multi threaded
    {
        oomph::context           ctxt(MPI_COMM_WORLD, true);
        std::vector<std::thread> threads;
        threads.reserve(NTHREADS);
        for (int i = 0; i < NTHREADS; ++i)
            threads.push_back(std::thread{f, std::ref(ctxt), SIZE, i, NTHREADS});
        for (auto& t : threads) t.join();
    }
}

// no callback
// ===========
template<typename Env>
void
test_send_recv(oomph::context& ctxt, std::size_t size, int tid, int num_threads)
{
    Env env(ctxt, size, tid, num_threads);

    // use is_ready() -> must manually progress the communicator
    for (int i = 0; i < NITERS; i++)
    {
        auto rreq = env.comm.recv(env.rmsg, env.rpeer_rank, env.tag);
        auto sreq = env.comm.send(env.smsg, env.speer_rank, env.tag);
        while (!(rreq.is_ready() && sreq.is_ready())) { env.comm.progress(); };
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }

    // use test() -> communicator is progressed automatically
    for (int i = 0; i < NITERS; i++)
    {
        auto rreq = env.comm.recv(env.rmsg, env.rpeer_rank, env.tag);
        auto sreq = env.comm.send(env.smsg, env.speer_rank, env.tag);
        while (!(rreq.test() && sreq.test())) {};
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }

    // use wait() -> communicator is progressed automatically
    for (int i = 0; i < NITERS; i++)
    {
        auto rreq = env.comm.recv(env.rmsg, env.rpeer_rank, env.tag);
        env.comm.send(env.smsg, env.speer_rank, env.tag).wait();
        rreq.wait();
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
}

TEST_F(mpi_test_fixture, send_recv)
{
    launch_test(test_send_recv<test_environment>);
#if HWMALLOC_ENABLE_DEVICE
    launch_test(test_send_recv<test_environment_device>);
#endif
}

// callback: pass by l-value reference
// ===================================
template<typename Env>
void
test_send_recv_cb(oomph::context& ctxt, std::size_t size, int tid, int num_threads)
{
    using rank_type = test_environment::rank_type;
    using tag_type = test_environment::tag_type;
    using message = test_environment::message;

    Env env(ctxt, size, tid, num_threads);

    volatile int received = 0;
    volatile int sent = 0;

    auto send_callback = [&](message const&, rank_type, tag_type) { ++sent; };
    auto recv_callback = [&](message&, rank_type, tag_type) { ++received; };

    // use is_ready() -> must manually progress the communicator
    for (int i = 0; i < NITERS; i++)
    {
        auto rh = env.comm.recv(env.rmsg, env.rpeer_rank, 1, recv_callback);
        auto sh = env.comm.send(env.smsg, env.speer_rank, 1, send_callback);
        while (!rh.is_ready() || !sh.is_ready()) { env.comm.progress(); }
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);

    received = 0;
    sent = 0;
    // use test() -> communicator is progressed automatically
    for (int i = 0; i < NITERS; i++)
    {
        auto rh = env.comm.recv(env.rmsg, env.rpeer_rank, 1, recv_callback);
        auto sh = env.comm.send(env.smsg, env.speer_rank, 1, send_callback);
        while (!rh.test() || !sh.test()) {}
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);

    received = 0;
    sent = 0;
    // use wait() -> communicator is progressed automatically
    for (int i = 0; i < NITERS; i++)
    {
        auto rh = env.comm.recv(env.rmsg, env.rpeer_rank, 1, recv_callback);
        env.comm.send(env.smsg, env.speer_rank, 1, send_callback).wait();
        rh.wait();
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);
}

TEST_F(mpi_test_fixture, send_recv_cb)
{
    launch_test(test_send_recv_cb<test_environment>);
#if HWMALLOC_ENABLE_DEVICE
    launch_test(test_send_recv_cb<test_environment_device>);
#endif
}

// callback: pass by r-value reference (give up ownership)
// =======================================================
template<typename Env>
void
test_send_recv_cb_disown(oomph::context& ctxt, std::size_t size, int tid, int num_threads)
{
    using rank_type = test_environment::rank_type;
    using tag_type = test_environment::tag_type;
    using message = test_environment::message;

    Env env(ctxt, size, tid, num_threads);

    volatile int received = 0;
    volatile int sent = 0;

    auto send_callback = [&](message msg, rank_type, tag_type) {
        ++sent;
        env.smsg = std::move(msg);
    };
    auto recv_callback = [&](message msg, rank_type, tag_type) {
        ++received;
        env.rmsg = std::move(msg);
    };

    // use is_ready() -> must manually progress the communicator
    for (int i = 0; i < NITERS; i++)
    {
        auto rh = env.comm.recv(std::move(env.rmsg), env.rpeer_rank, 1, recv_callback);
        auto sh = env.comm.send(std::move(env.smsg), env.speer_rank, 1, send_callback);
        while (!rh.is_ready() || !sh.is_ready()) { env.comm.progress(); }
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);

    received = 0;
    sent = 0;
    // use test() -> communicator is progressed automatically
    for (int i = 0; i < NITERS; i++)
    {
        auto rh = env.comm.recv(std::move(env.rmsg), env.rpeer_rank, 1, recv_callback);
        auto sh = env.comm.send(std::move(env.smsg), env.speer_rank, 1, send_callback);
        while (!rh.test() || !sh.test()) {}
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);

    received = 0;
    sent = 0;
    // use wait() -> communicator is progressed automatically
    for (int i = 0; i < NITERS; i++)
    {
        auto rh = env.comm.recv(std::move(env.rmsg), env.rpeer_rank, 1, recv_callback);
        env.comm.send(std::move(env.smsg), env.speer_rank, 1, send_callback).wait();
        rh.wait();
        EXPECT_TRUE(env.check_recv_buffer());
        env.fill_recv_buffer();
    }
    EXPECT_EQ(received, NITERS);
    EXPECT_EQ(sent, NITERS);
}

TEST_F(mpi_test_fixture, send_recv_cb_disown)
{
    launch_test(test_send_recv_cb_disown<test_environment>);
#if HWMALLOC_ENABLE_DEVICE
    launch_test(test_send_recv_cb_disown<test_environment_device>);
#endif
}

// callback: pass by l-value reference, and resubmit
// =================================================
template<typename Env>
void
test_send_recv_cb_resubmit(oomph::context& ctxt, std::size_t size, int tid, int num_threads)
{
    using rank_type = test_environment::rank_type;
    using tag_type = test_environment::tag_type;
    using message = test_environment::message;

    Env env(ctxt, size, tid, num_threads);

    volatile int received = 0;
    volatile int sent = 0;

    struct recursive_send_callback
    {
        Env&          env;
        volatile int& sent;

        void operator()(message& msg, rank_type dst, tag_type tag)
        {
            ++sent;
            if (sent < NITERS) env.comm.send(msg, dst, tag, recursive_send_callback{*this});
        }
    };

    struct recursive_recv_callback
    {
        Env&          env;
        volatile int& received;

        void operator()(message& msg, rank_type src, tag_type tag)
        {
            ++received;
            EXPECT_TRUE(env.check_recv_buffer());
            env.fill_recv_buffer();
            if (received < NITERS) env.comm.recv(msg, src, tag, recursive_recv_callback{*this});
        }
    };

    env.comm.recv(env.rmsg, env.rpeer_rank, 1, recursive_recv_callback{env, received});
    env.comm.send(env.smsg, env.speer_rank, 1, recursive_send_callback{env, sent});

    while (sent < NITERS || received < NITERS) { env.comm.progress(); };
}

TEST_F(mpi_test_fixture, send_recv_cb_resubmit)
{
    launch_test(test_send_recv_cb_resubmit<test_environment>);
#if HWMALLOC_ENABLE_DEVICE
    launch_test(test_send_recv_cb_resubmit<test_environment_device>);
#endif
}

// callback: pass by r-value reference (give up ownership), and resubmit
// =====================================================================
template<typename Env>
void
test_send_recv_cb_resubmit_disown(oomph::context& ctxt, std::size_t size, int tid, int num_threads)
{
    using rank_type = test_environment::rank_type;
    using tag_type = test_environment::tag_type;
    using message = test_environment::message;

    Env env(ctxt, size, tid, num_threads);

    volatile int received = 0;
    volatile int sent = 0;

    struct recursive_send_callback
    {
        Env&          env;
        volatile int& sent;

        void operator()(message msg, rank_type dst, tag_type tag)
        {
            ++sent;
            if (sent < NITERS)
                env.comm.send(std::move(msg), dst, tag, recursive_send_callback{*this});
        }
    };

    struct recursive_recv_callback
    {
        Env&          env;
        volatile int& received;

        void operator()(message msg, rank_type src, tag_type tag)
        {
            ++received;
            env.rmsg = std::move(msg);
            EXPECT_TRUE(env.check_recv_buffer());
            env.fill_recv_buffer();
            if (received < NITERS)
                env.comm.recv(std::move(env.rmsg), src, tag, recursive_recv_callback{*this});
        }
    };

    env.comm.recv(std::move(env.rmsg), env.rpeer_rank, 1, recursive_recv_callback{env, received});
    env.comm.send(std::move(env.smsg), env.speer_rank, 1, recursive_send_callback{env, sent});

    while (sent < NITERS || received < NITERS) { env.comm.progress(); };
}

TEST_F(mpi_test_fixture, send_recv_cb_resubmit_disown)
{
    launch_test(test_send_recv_cb_resubmit_disown<test_environment>);
#if HWMALLOC_ENABLE_DEVICE
    launch_test(test_send_recv_cb_resubmit_disown<test_environment_device>);
#endif
}
