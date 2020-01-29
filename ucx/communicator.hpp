/* 
 * GridTools
 * 
 * Copyright (c) 2014-2019, ETH Zurich
 * All rights reserved.
 * 
 * Please, refer to the LICENSE file in the root directory.
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 */
#ifndef INCLUDED_GHEX_TL_UCX_COMMUNICATOR_CONTEXT_HPP
#define INCLUDED_GHEX_TL_UCX_COMMUNICATOR_CONTEXT_HPP

#include <atomic>
#include "../shared_message_buffer.hpp"
#include "./future.hpp"
#include <iostream>

namespace gridtools {
    namespace ghex {
        namespace tl {
		    
            namespace ucx {    

                template<typename ThreadPrimitives>
                struct communicator
                {
                    using worker_type            = worker_t<ThreadPrimitives>;
                    using thread_primitives_type = ThreadPrimitives;
                    using thread_token           = typename thread_primitives_type::token;
                    using rank_type              = endpoint_t::rank_type;
                    using tag_type               = typename worker_type::tag_type;
                    using request                = request_ft<ThreadPrimitives>;
                    template<typename T>
                    using future                 = future_t<T,ThreadPrimitives>;
                    // needed for now for high-level API
                    using address_type           = rank_type;
                    
                    using request_cb_type        = request_cb<ThreadPrimitives>;
                    using request_cb_data_type   = typename request_cb_type::data_type;
                    using request_cb_state_type  = typename request_cb_type::state_type;
                    using message_type           = typename request_cb_type::message_type;
                    
                    worker_type*  m_recv_worker;
                    worker_type*  m_send_worker;
                    ucp_worker_h  m_ucp_rw;
                    ucp_worker_h  m_ucp_sw;
                    rank_type     m_rank;
                    rank_type     m_size;

                    communicator(worker_type* rw, worker_type* sw) noexcept
                    : m_recv_worker{rw}
                    , m_send_worker{sw}
                    , m_ucp_rw{rw->get()}
                    , m_ucp_sw{sw->get()}
                    , m_rank{m_send_worker->rank()}
                    , m_size{m_send_worker->size()}
                    {}

                    communicator(const communicator&) = default;
                    communicator(communicator&&) = default;
                    communicator& operator=(const communicator&) = default;
                    communicator& operator=(communicator&&) = default;

                    rank_type rank() const noexcept { return m_rank; }
                    rank_type size() const noexcept { return m_size; }
                    address_type address() const { return rank(); }

                    template <typename MsgType>
                    [[nodiscard]] future<void> send(const MsgType &msg, rank_type dst, tag_type tag)
                    {
                        const auto& ep = m_send_worker->connect(dst);
                        const auto stag = ((std::uint_fast64_t)tag << 32) | 
                                           (std::uint_fast64_t)(rank());
                        auto ret = ucp_tag_send_nb(
                            ep.get(),                                        // destination
                            msg.data(),                                      // buffer
                            msg.size()*sizeof(typename MsgType::value_type), // buffer size
                            ucp_dt_make_contig(1),                           // data type
                            stag,                                            // tag
                            &communicator::empty_send_callback);             // callback function pointer: empty here
                        
                        if (reinterpret_cast<std::uintptr_t>(ret) == UCS_OK)
                        {
                            // send operation is completed immediately and the call-back function is not invoked
                            return request{nullptr};
                        } 
                        else if(!UCS_PTR_IS_ERR(ret))
                        {
                            return request{request::data_type::construct(ret, m_recv_worker, m_send_worker, request_kind::send)};
                        }
                        else
                        {
                            // an error occurred
                            throw std::runtime_error("ghex: ucx error - send operation failed");
                        }
                    }
		
                    template <typename MsgType>
                    [[nodiscard]] future<void> recv(MsgType &msg, rank_type src, tag_type tag)
                    {
                        const auto rtag = ((std::uint_fast64_t)tag << 32) | 
                                           (std::uint_fast64_t)(src);
                        return m_send_worker->m_thread_primitives->critical(
                            [this,rtag,&msg,src,tag]()
                            {
                                auto ret = ucp_tag_recv_nb(
                                    m_recv_worker->get(),                            // worker
                                    msg.data(),                                      // buffer
                                    msg.size()*sizeof(typename MsgType::value_type), // buffer size
                                    ucp_dt_make_contig(1),                           // data type
                                    rtag,                                            // tag
                                    ~std::uint_fast64_t(0ul),                        // tag mask
                                    &communicator::empty_recv_callback);             // callback function pointer: empty here
                                if(!UCS_PTR_IS_ERR(ret))
                                {
			                        if (UCS_INPROGRESS != ucp_request_check_status(ret))
                                    {
				                        // recv completed immediately
		    		                    // we need to free the request here, not in the callback
                                        auto ucx_ptr = ret;
                                        request_init(ucx_ptr);
				                        ucp_request_free(ucx_ptr);
                                        return request{nullptr};
                                    }
                                    else
                                    {
                                        return request{request::data_type::construct(ret, m_recv_worker, m_send_worker, request_kind::recv)};
                                    }
                                }
                                else
                                {
                                    // an error occurred
                                    throw std::runtime_error("ghex: ucx error - recv operation failed");
                                }
                            }
                        );
                    }

                    /** Function to invoke to poll the transport layer and check for the completions
                     * of the operations without a future associated to them (that is, they are associated
                     * to a call-back). When an operation completes, the corresponfing call-back is invoked
                     * with the rank and tag associated with that request.
                     *
                     * @return unsigned Non-zero if any communication was progressed, zero otherwise.
                     */
                    unsigned progress()
                    {
                        int p = 0;
                        p+= ucp_worker_progress(m_ucp_sw);
                        p+= ucp_worker_progress(m_ucp_sw);
                        p+= ucp_worker_progress(m_ucp_sw);
                        m_send_worker->m_thread_primitives->critical(
                            [this,&p]()
                            {
                                p+= ucp_worker_progress(m_ucp_rw);
                                p+= ucp_worker_progress(m_ucp_rw);
                            }
                        );
                        return p;
                    }
	    
                    template<typename CallBack>
                    request_cb_type send(message_type&& msg, rank_type dst, tag_type tag, CallBack&& callback)
                    {
                        const auto& ep = m_send_worker->connect(dst);
                        const auto stag = ((std::uint_fast64_t)tag << 32) | 
                                           (std::uint_fast64_t)(rank());
                        auto ret = ucp_tag_send_nb(
                            ep.get(),                                        // destination
                            msg.data(),                                      // buffer
                            msg.size(),                                      // buffer size
                            ucp_dt_make_contig(1),                           // data type
                            stag,                                            // tag
                            &communicator::send_callback);                   // callback function pointer
                        
                        if (reinterpret_cast<std::uintptr_t>(ret) == UCS_OK)
                        {
                            // send operation is completed immediately and the call-back function is not invoked
                            // call the callback
                            callback(std::move(msg), dst, tag);
                            return {};
                        } 
                        else if(!UCS_PTR_IS_ERR(ret))
                        {
                            auto req_ptr = request_cb_data_type::construct(ret,
                                m_send_worker,
                                request_kind::send,
                                std::move(msg),
                                dst,
                                tag,
                                std::forward<CallBack>(callback),
                                std::make_shared<request_cb_state_type>(false));
                            return {req_ptr, req_ptr->m_completed};
                        }
                        else
                        {
                            // an error occurred
                            throw std::runtime_error("ghex: ucx error - send operation failed");
                        }
                    }

                    template<typename CallBack>
                    request_cb_type recv(message_type&& msg, rank_type src, tag_type tag, CallBack&& callback)
                    {
                        const auto rtag = ((std::uint_fast64_t)tag << 32) | 
                                           (std::uint_fast64_t)(src);
                        return m_send_worker->m_thread_primitives->critical(
                            [this,rtag,&msg,src,tag,&callback]()
                            {
                                auto ret = ucp_tag_recv_nb(
                                    m_ucp_rw,                                        // worker
                                    msg.data(),                                      // buffer
                                    msg.size(),                                      // buffer size
                                    ucp_dt_make_contig(1),                           // data type
                                    rtag,                                            // tag
                                    ~std::uint_fast64_t(0ul),                        // tag mask
                                    &communicator::recv_callback);                   // callback function pointer
                                if(!UCS_PTR_IS_ERR(ret))
                                {
			                        if (UCS_INPROGRESS != ucp_request_check_status(ret))
                                    {
                                        // early completed
                                        callback(std::move(msg), src, tag);
		    		                    // we need to free the request here, not in the callback
                                        auto ucx_ptr = ret;
                                        request_cb_data_type::get(ucx_ptr).m_kind = request_kind::none;
				                        ucp_request_free(ucx_ptr);
                                        return request_cb_type{};
                                    }
                                    else
                                    {
                                        auto req_ptr = request_cb_data_type::construct(ret,
                                            m_recv_worker,
                                            request_kind::recv,
                                            std::move(msg),
                                            src,
                                            tag,
                                            std::forward<CallBack>(callback),
                                            std::make_shared<request_cb_state_type>(false));
                                        return request_cb_type{req_ptr, req_ptr->m_completed};
                                    }
                                }
                                else
                                {
                                    // an error occurred
                                    throw std::runtime_error("ghex: ucx error - recv operation failed");
                                }
                            }
                        );
                    }

                    void barrier()
                    {
                        // a trivial barrier implementation for debuging purposes only!
                        // send a message to all other ranks, wait for their message
                        static unsigned char msgid = 0;
                        auto bfunc = [this]()
                            {
                                volatile int sent = 0;
                                std::vector<unsigned char> smsg(1), rmsg(1);

                                auto send_callback = [&](message_type, int, int) {sent++;};

                                smsg[0] = msgid;
                                using ref_msg = ::gridtools::ghex::tl::cb::ref_message<unsigned char>;
                                for(rank_type r=0; r<m_size; r++)
                                    if(r != m_rank)
                                        send(message_type{ref_msg{smsg.data(),smsg.size()}}, r, 0xdeadbeef, send_callback);

                                for(rank_type r=0; r<m_size; r++){
                                    if(r != m_rank){
                                        recv(rmsg, r, 0xdeadbeef).wait();
                                        if (*reinterpret_cast<unsigned char*>(rmsg.data()) != msgid)
                                            throw std::runtime_error("UCX barrier error: unexpected message id.");
                                    }
                                }
                                
                                while(sent!=size()-1) progress();
                                msgid++;
                            };

                        if(m_send_worker->m_token_ptr)
                        {
                            thread_token &token = *m_send_worker->m_token_ptr;
                            m_send_worker->m_thread_primitives->barrier(token);
                            m_send_worker->m_thread_primitives->single(token, bfunc);
                            // this thread barrier is needed to flush the progress queue:
                            // if we omit this barrier, the other threads may see a progress
                            // when calling progress()
                            m_send_worker->m_thread_primitives->barrier(token);
                        } 
                        else 
                        {
                            bfunc();
                        }
                    }

                private:
                    
                    static void empty_send_callback(void *, ucs_status_t) {}

                    static void empty_recv_callback(void *, ucs_status_t, ucp_tag_recv_info_t*) {}

                    inline static void send_callback(void * __restrict ucx_req, ucs_status_t __restrict status)
                    {
                        auto& req = request_cb_data_type::get(ucx_req);
                        if (status == UCS_OK)
                            // call the callback
                            req.m_cb(std::move(req.m_msg), req.m_rank, req.m_tag);
                        // else: cancelled - do nothing
                        // set completion bit
                        *req.m_completed = true;
                        // destroy the request - releases the message
                        req.m_kind = request_kind::none;
                        req.~request_cb_data_type();
                        // free ucx request
				        ucp_request_free(ucx_req);
                    }

                    inline static void recv_callback(void * __restrict ucx_req, ucs_status_t __restrict status, ucp_tag_recv_info_t* /*info*/)
                    {
                        auto& req = request_cb_data_type::get(ucx_req);

                        if (status == UCS_OK)
                        {
                            if (static_cast<int>(req.m_kind) == 0)
                            {
                                // we're in early completion mode
                                return;
                            }

                            req.m_cb(std::move(req.m_msg), req.m_rank, req.m_tag);
                            // set completion bit
                            *req.m_completed = true;
                            // destroy the request - releases the message
                            req.m_kind = request_kind::none;
                            req.~request_cb_data_type();
                            // free ucx request
                            ucp_request_free(ucx_req);
                        }
                        else if (status == UCS_ERR_CANCELED)
                        {
			                // canceled - do nothing
                            // set completion bit
                            *req.m_completed = true;
                            // destroy the request - releases the message
                            req.m_kind = request_kind::none;
                            req.~request_cb_data_type(); 
                            // free ucx request
                            ucp_request_free(ucx_req);
                        }
                        else
                        {
                            // an error occurred
                            throw std::runtime_error("ghex: ucx error - recv message truncated");
                        }
                    }
                };

            } // namespace ucx
        } // namespace tl
    } // namespace ghex
} // namespace gridtools

#endif /* INCLUDED_GHEX_TL_UCX_COMMUNICATOR_CONTEXT_HPP */

