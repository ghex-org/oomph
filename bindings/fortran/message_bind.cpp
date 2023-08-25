/*
 * ghex-org
 *
 * Copyright (c) 2014-2023, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <cstring>
#include <vector>
#include <bindings/fortran/oomph_defs.hpp>
#include <bindings/fortran/object_wrapper.hpp>
#include <bindings/fortran/context_bind.hpp>
#include <bindings/fortran/message_bind.hpp>

extern "C"
void *oomph_message_new(std::size_t size, int allocator_type)
{
    void *wmessage = nullptr;
    auto &context = oomph::fort::get_context();

    switch(allocator_type){
    case oomph::fort::OomphAllocatorHost:
	{
            wmessage = new message_type(std::move(context.make_buffer<unsigned char>(size)));
	    break;
	}
#if HWMALLOC_ENABLE_DEVICE
    case oomph::fort::OomphAllocatorDevice:
	{
            wmessage = new message_type(std::move(context.make_device_buffer<unsigned char>(size)));
	    break;
	}
#endif
    default:
	{
	    std::cerr << "BINDINGS: " << __FUNCTION__ << ": unsupported allocator type: " << allocator_type << "\n";
            std::terminate();
	    break;
	}
    }

    return wmessage;
}

extern "C"
void oomph_message_free(message_type **message_ref)
{
    if (nullptr == message_ref) return;
    delete *message_ref;
    *message_ref = nullptr;
}

extern "C"
void oomph_message_zero(message_type *message)
{
    if (nullptr == message) return;
    unsigned char* __restrict data = message->data();
    std::size_t size = message->size();
    std::memset(data, 0, size);
}

extern "C"
unsigned char *oomph_message_data_wrapped(message_type *message, std::size_t *size)
{
    if (nullptr == message) {
        *size = 0;
        return nullptr;
    }
    *size = message->size();
    return message->data();
}
