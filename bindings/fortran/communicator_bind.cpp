/*
 * ghex-org
 *
 * Copyright (c) 2014-2021, ETH Zurich
 * All rights reserved.
 *
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <iostream>
#include <bindings/fortran/object_wrapper.hpp>
#include <bindings/fortran/context_bind.hpp>
#include <bindings/fortran/request_bind.hpp>
#include <bindings/fortran/message_bind.hpp>

using namespace oomph::fort;
using communicator_type = oomph::communicator;

namespace oomph {
    namespace fort {

        /* fortran-side user callback */
        typedef void (*f_callback)(void *mesg, int rank, int tag, void *user_data);

        struct callback {
            f_callback cb;
            void *user_data = nullptr;
            callback(f_callback pcb, void *puser_data = nullptr) : cb{pcb}, user_data{puser_data} {}
            void operator() (message_type message, int rank, int tag) const {
                if(cb) cb(&message, rank, tag, user_data);
            }
        };

        struct callback_ref {
            f_callback cb;
            void *user_data = nullptr;
            callback_ref(f_callback pcb, void *puser_data = nullptr) : cb{pcb}, user_data{puser_data} {}
            void operator() (message_type &message, int rank, int tag) const {
                if(cb) cb(&message, rank, tag, user_data);
            }
        };

        struct callback_multi {
            f_callback cb;
            void *user_data = nullptr;
            callback_multi(f_callback pcb, void *puser_data = nullptr) : cb{pcb}, user_data{puser_data} {}
            void operator() (message_type message, std::vector<int>, std::vector<int>) const {
                if(cb) cb(&message, -1, -1, user_data);
            }
        };

        struct callback_multi_ref {
            f_callback cb;
            void *user_data = nullptr;
            callback_multi_ref(f_callback pcb, void *puser_data = nullptr) : cb{pcb}, user_data{puser_data} {}
            void operator() (message_type &message, std::vector<int>, std::vector<int>) const {
                if(cb) cb(&message, -1, -1, user_data);
            }
        };
    }
}


extern "C"
void* oomph_get_communicator()
{
    return new obj_wrapper(get_context().get_communicator());
}

extern "C"
int oomph_comm_rank(obj_wrapper *wrapper)
{
    return get_object_ptr_unsafe<communicator_type>(wrapper)->rank();
}

extern "C"
int oomph_comm_size(obj_wrapper *wrapper)
{
    return get_object_ptr_unsafe<communicator_type>(wrapper)->size();
}

extern "C"
void oomph_comm_progress(obj_wrapper *wrapper)
{
    get_object_ptr_unsafe<communicator_type>(wrapper)->progress();
}


/*
  SEND requests
 */
extern "C"
void oomph_comm_post_send(obj_wrapper *wcomm, message_type *message, int rank, int tag, frequest_type *freq)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);

    auto req = comm->send(*message, rank, tag);
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = false;
}

extern "C"
void oomph_comm_post_send_cb_wrapped(obj_wrapper *wcomm, message_type *message, int rank, int tag, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);

    auto req = comm->send(*message, rank, tag, callback_ref{cb, user_data});
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = false;
}

extern "C"
void oomph_comm_send_cb_wrapped(obj_wrapper *wcomm, message_type **message_ref, int rank, int tag, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message_ref || nullptr==wcomm || nullptr == *message_ref){
        std::cerr << "ERROR: NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);
    
    auto req = comm->send(std::move(**message_ref), rank, tag, callback{cb, user_data});
    *message_ref = nullptr;
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = false;
}


/*
   SEND_MULTI requests
 */

extern "C"
void oomph_comm_post_send_multi_wrapped(obj_wrapper *wcomm, message_type *message, int *ranks, int nranks, int *tags, frequest_type *freq)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: trying to submit a NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);
    std::vector<int> ranks_array(nranks);
    ranks_array.assign(ranks, ranks+nranks);
    std::vector<int> tags_array(nranks);
    tags_array.assign(tags, tags+nranks);

    auto req = comm->send_multi(*message, ranks_array, tags_array);
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = false;
}

extern "C"
void oomph_comm_post_send_multi_cb_wrapped(obj_wrapper *wcomm, message_type *message, int *ranks, int nranks, int *tags, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: trying to submit a NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);
    std::vector<int> ranks_array(nranks);
    ranks_array.assign(ranks, ranks+nranks);
    std::vector<int> tags_array(nranks);
    tags_array.assign(tags, tags+nranks);

    auto req = comm->send_multi(*message, ranks_array, tags_array, callback_multi_ref{cb, user_data});
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = false;
}

extern "C"
void oomph_comm_send_multi_cb_wrapped(obj_wrapper *wcomm, message_type **message_ref, int *ranks, int nranks, int *tags, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message_ref || nullptr==wcomm || nullptr == *message_ref){
        std::cerr << "ERROR: NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);
    std::vector<int> ranks_array(nranks);
    ranks_array.assign(ranks, ranks+nranks);
    std::vector<int> tags_array(nranks);
    tags_array.assign(tags, tags+nranks);

    auto req = comm->send_multi(std::move(**message_ref), ranks_array, tags_array, callback_multi{cb, user_data});
    *message_ref = nullptr;
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = false;
}


/*
   RECV requests
 */
extern "C"
void oomph_comm_post_recv(obj_wrapper *wcomm, message_type *message, int rank, int tag, frequest_type *freq)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: trying to submit a NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);

    auto req = comm->recv(*message, rank, tag);
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = true;
}

extern "C"
void oomph_comm_post_recv_cb_wrapped(obj_wrapper *wcomm, message_type *message, int rank, int tag, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: trying to submit a NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);

    auto req = comm->recv(*message, rank, tag, callback_ref{cb, user_data});
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = true;
}

extern "C"
void oomph_comm_recv_cb_wrapped(obj_wrapper *wcomm, message_type **message_ref, int rank, int tag, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message_ref || nullptr==wcomm || nullptr == *message_ref){
        std::cerr << "ERROR: NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }
    
    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);

    auto req = comm->recv(std::move(**message_ref), rank, tag, callback{cb, user_data});
    *message_ref = nullptr;
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = true;
}


/*
   resubmission of recv requests from inside callbacks
 */
extern "C"
void oomph_comm_resubmit_recv_wrapped(obj_wrapper *wcomm, message_type *message, int rank, int tag, f_callback cb, frequest_type *freq, void *user_data)
{
    if(nullptr==message || nullptr==wcomm){
        std::cerr << "ERROR: trying to submit a NULL message or communicator in " << __FUNCTION__ << ". Terminating.\n";
        std::terminate();
    }

    communicator_type *comm = get_object_ptr_unsafe<communicator_type>(wcomm);

    auto req = comm->recv(std::move(*message), rank, tag, callback{cb, user_data});
    if(!freq) return;
    new(freq->data) decltype(req)(std::move(req));
    freq->recv_request = true;
}
