/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <oomph/tensor/layout.hpp>
#include <oomph/tensor/rt_layout.hpp>
#include <oomph/tensor/sender.hpp>
#include <oomph/tensor/receiver.hpp>

#include <gtest/gtest.h>
#include "./mpi_runner/mpi_test_fixture.hpp"
#include <iostream>
#include <array>
#include <iomanip>
#include <thread>
#include <vector>

template<std::size_t I, std::size_t J, std::size_t K>
void
test_layout()
{
    using namespace oomph;
    using namespace oomph::tensor;
    using layout_t = layout<I, J, K>;
    static_assert(layout_t::at(0) == I, "");
    static_assert(layout_t::at(1) == J, "");
    static_assert(layout_t::at(2) == K, "");

    static constexpr std::size_t pos_0 = (I == 0) ? 0 : ((J == 0) ? 1 : 2);
    static constexpr std::size_t pos_1 = (I == 1) ? 0 : ((J == 1) ? 1 : 2);
    static constexpr std::size_t pos_2 = (I == 2) ? 0 : ((J == 2) ? 1 : 2);

    static_assert(layout_t::find(0) == pos_0, "");
    static_assert(layout_t::find(1) == pos_1, "");
    static_assert(layout_t::find(2) == pos_2, "");

    auto l = make_rt_layout<layout_t>();

    EXPECT_EQ(l.at(0), layout_t::at(0));
    EXPECT_EQ(l.at(1), layout_t::at(1));
    EXPECT_EQ(l.at(2), layout_t::at(2));

    EXPECT_EQ(l.find(0), layout_t::find(0));
    EXPECT_EQ(l.find(1), layout_t::find(1));
    EXPECT_EQ(l.find(2), layout_t::find(2));

    EXPECT_TRUE( l.template is_equal<layout_t>() );
    EXPECT_FALSE( (l.template is_equal<layout<I,K,J>>()) );
    EXPECT_FALSE( (l.template is_equal<layout<J,I,K>>()) );
    EXPECT_FALSE( (l.template is_equal<layout<J,K,I>>()) );
    EXPECT_FALSE( (l.template is_equal<layout<K,I,J>>()) );
    EXPECT_FALSE( (l.template is_equal<layout<K,J,I>>()) );
}

TEST(layout, ctor)
{
    using namespace oomph;
    using namespace oomph::tensor;

    test_layout<0, 1, 2>();
    test_layout<0, 2, 1>();
    test_layout<1, 0, 2>();
    test_layout<1, 2, 0>();
    test_layout<2, 0, 1>();
    test_layout<2, 1, 0>();
    //test_layout<2,2,0>();
}

template<std::size_t I, std::size_t J, std::size_t K>
void
test_tensor_strides(std::size_t X, std::size_t Y, std::size_t Z, std::size_t padding = 0)
{
    using namespace oomph;
    using T = double;
    using layout_t = tensor::layout<I, J, K>;

    std::size_t dims[] = {X, Y, Z};

    std::vector<double> data(
        (layout_t::find(2) == 0
                ? (X + padding) * Y * Z
                : (layout_t::find(2) == 1 ? (Y + padding) * X * Z : (Z + padding) * X * Y)) -
        padding);

    tensor::detail::map<T, layout_t> m({X, Y, Z}, data.data(), (data.data() + data.size()) - 1);

    EXPECT_EQ(m.strides()[layout_t::find(2)], 1);
    EXPECT_EQ(m.strides()[layout_t::find(1)], dims[layout_t::find(2)] + padding);
    EXPECT_EQ(m.strides()[layout_t::find(0)],
        dims[layout_t::find(1)] * (dims[layout_t::find(2)] + padding));
}

TEST(tensor, strides)
{
    test_tensor_strides<2, 1, 0>(2, 3, 5);
    test_tensor_strides<2, 1, 0>(2, 3, 5, 1);
    test_tensor_strides<2, 1, 0>(2, 3, 5, 3);
    test_tensor_strides<2, 0, 1>(2, 3, 5);
    test_tensor_strides<2, 0, 1>(2, 3, 5, 1);
    test_tensor_strides<2, 0, 1>(2, 3, 5, 3);
    test_tensor_strides<1, 2, 0>(2, 3, 5);
    test_tensor_strides<1, 2, 0>(2, 3, 5, 1);
    test_tensor_strides<1, 2, 0>(2, 3, 5, 3);
    test_tensor_strides<1, 0, 2>(2, 3, 5);
    test_tensor_strides<1, 0, 2>(2, 3, 5, 1);
    test_tensor_strides<1, 0, 2>(2, 3, 5, 3);
}

TEST_F(mpi_test_fixture, ctor)
{
    using namespace oomph;
    auto ctxt = context(MPI_COMM_WORLD, false);

    std::size_t const halo = 2;
    std::size_t const x = 2;
    std::size_t const y = 3;
    std::size_t const z = 5;

    std::vector<double> data((x + 2 * halo) * (y + 2 * halo) * (z + 2 * halo), world_rank);

    data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 0))] = 9000;
    data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 0))] = 9001;
    data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 0))] = 9010;
    data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 0))] = 9011;
    data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 0))] = 9020;
    data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 0))] = 9021;

    data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 1))] = 9100;
    data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 1))] = 9101;
    data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 1))] = 9110;
    data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 1))] = 9111;
    data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 1))] = 9120;
    data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 1))] = 9121;

    data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 2))] = 9200;
    data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 2))] = 9201;
    data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 2))] = 9210;
    data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 2))] = 9211;
    data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 2))] = 9220;
    data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 2))] = 9221;

    data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 3))] = 9300;
    data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 3))] = 9301;
    data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 3))] = 9310;
    data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 3))] = 9311;
    data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 3))] = 9320;
    data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 3))] = 9321;

    data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 4))] = 9400;
    data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 4))] = 9401;
    data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 4))] = 9410;
    data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 4))] = 9411;
    data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 4))] = 9420;
    data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 4))] = 9421;

    using layout_t = tensor::layout<2, 1, 0>;
    using map_t = tensor::map<double, layout_t>;

    map_t t = ctxt.map_tensor<layout_t>({x + 2 * halo, y + 2 * halo, z + 2 * halo}, data.data(),
        (data.data() + data.size()) - 1);

    tensor::buffer_cache<double>                    s_cache;
    tensor::buffer_cache<double>                    r_cache;
    tensor::sender<tensor::map<double, layout_t>>   s = tensor::make_sender(t, s_cache);
    tensor::receiver<tensor::map<double, layout_t>> r = tensor::make_receiver(t, r_cache);

    if (world_rank == 0)
    {
        s
            // y-z plane, +x direction
            .add_dst({{halo + x - halo, halo, halo}, {halo, y, z}}, 1, 0, 1)
            // x-y plane, -z direction
            .add_dst({{halo, halo, halo}, {x, y, halo}}, 2, 0, 0)
            // whole x-y plane, -z direction
            .add_dst({{0, 0, halo}, {x + 2 * halo, y + 2 * halo, halo}}, 3, 0, 2)
            .connect();

        s.pack(1).wait();
        s.send(1).wait();
        s.pack(0).wait();
        s.send(0).wait();
        s.pack(2).wait();
        s.send(2).wait();
    }
    if (world_rank == 1)
    {
        r.add_src({{0, halo, halo}, {halo, y, z}}, 0, 0).connect();
        r.recv().wait();
        r.unpack().wait();

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 0))], 9000);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 0))], 9001);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 0))], 9010);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 0))], 9011);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 0))], 9020);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 0))], 9021);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 1))], 9100);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 1))], 9101);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 1))], 9110);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 1))], 9111);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 1))], 9120);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 1))], 9121);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 2))], 9200);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 2))], 9201);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 2))], 9210);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 2))], 9211);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 2))], 9220);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 2))], 9221);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 3))], 9300);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 3))], 9301);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 3))], 9310);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 3))], 9311);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 3))], 9320);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 3))], 9321);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 4))], 9400);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 4))], 9401);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 4))], 9410);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 4))], 9411);
        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 4))], 9420);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 4))], 9421);
    }
    if (world_rank == 2)
    {
        r.add_src({{halo, halo, halo + z}, {x, y, halo}}, 0, 0).connect();
        r.recv().wait();
        r.unpack().wait();

        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 9000);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 9001);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 9010);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 9011);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 9020);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 9021);

        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 9100);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 9101);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 9110);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 9111);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 9120);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 9121);
    }
    if (world_rank == 3)
    {
        r.add_src({{0, 0, halo + z}, {x + 2 * halo, y + 2 * halo, halo}}, 0, 0).connect();
        r.recv().wait();
        r.unpack().wait();

        EXPECT_EQ(data[0 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 9000);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 9001);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 9010);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 9011);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 9020);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 9021);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 5))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 5))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (0 + (y + 2 * halo) * (halo + 6))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (1 + (y + 2 * halo) * (halo + 6))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 9100);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 9101);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (halo + 0 + (y + 2 * halo) * (halo + 6))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 9110);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 9111);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (halo + 1 + (y + 2 * halo) * (halo + 6))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 9120);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 9121);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (halo + 2 + (y + 2 * halo) * (halo + 6))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (5 + (y + 2 * halo) * (halo + 6))], 0);

        EXPECT_EQ(data[0 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[1 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 0 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[halo + 1 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[4 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 6))], 0);
        EXPECT_EQ(data[5 + (x + 2 * halo) * (6 + (y + 2 * halo) * (halo + 6))], 0);
    }
}
