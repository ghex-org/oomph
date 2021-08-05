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

#include <oomph/communicator.hpp>
#include <algorithm>

//#include <boost/callable_traits.hpp>
//#include <functional>
//#include <memory>

///** @brief checks the arguments of callback function object */
//#define OOMPH_CHECK_CALLBACK_F(MESSAGE_TYPE, RANK_TYPE, TAG_TYPE)
//    using args_t = boost::callable_traits::args_t<CallBack>;
//    using arg0_t = std::tuple_element_t<0, args_t>;
//    using arg1_t = std::tuple_element_t<1, args_t>;
//    using arg2_t = std::tuple_element_t<2, args_t>;
//    static_assert(std::tuple_size<args_t>::value == 3, "callback must have 3 arguments");
//    static_assert(std::is_same<arg1_t, RANK_TYPE>::value,
//        "rank_type is not convertible to second callback argument type");
//    static_assert(std::is_same<arg2_t, TAG_TYPE>::value,
//        "tag_type is not convertible to third callback argument type");
//    static_assert(std::is_same<arg0_t, MESSAGE_TYPE>::value,
//        "first callback argument type is not a message_type");

namespace oomph
{
///** @brief shared request state for completion handlers returned by callback based sends/recvs. */
//struct request_state
//{
//    // volatile is needed to prevent the compiler
//    // from optimizing away the check of this member
//    volatile bool m_ready = false;
//    unsigned int  m_index = 0;
//    request_state() = default;
//    request_state(bool r) noexcept
//    : m_ready{r}
//    {
//    }
//    request_state(bool r, unsigned int i) noexcept
//    : m_ready{r}
//    , m_index{i}
//    {
//    }
//    bool is_ready() const noexcept { return m_ready; }
//    int  queue_index() const noexcept { return m_index; }
//};
//
///** @brief simple request with shared state as member returned by callback based send/recvs. */
//struct request
//{
//    std::shared_ptr<request_state> m_request_state;
//    bool                           is_ready() const noexcept { return m_request_state->is_ready(); }
//    void                           reset() { m_request_state.reset(); }
//    int queue_index() const noexcept { return m_request_state->queue_index(); }
//};
//
///** @brief simple wrapper around an l-value reference message (stores pointer and size)
//                  * @tparam T value type of the messages content */
//template<typename T>
//struct ref_message
//{
//    using value_type = T;
//    T*          m_data;
//    std::size_t m_size;
//    T*          data() noexcept { return m_data; }
//    const T*    data() const noexcept { return m_data; }
//    std::size_t size() const noexcept { return m_size; }
//};
//
///** @brief type erased message capable of holding any message. Uses optimized initialization for
//                  * ref_messages and std::shared_ptr pointing to messages. */
//struct any_message
//{
//    using value_type = unsigned char;
//
//    // common interface to a message
//    struct iface
//    {
//        virtual unsigned char*       data() noexcept = 0;
//        virtual const unsigned char* data() const noexcept = 0;
//        virtual std::size_t          size() const noexcept = 0;
//        virtual ~iface() {}
//    };
//
//    // struct which holds the actual message and provides access through the interface iface
//    template<class Message>
//    struct holder final : public iface
//    {
//        using value_type = typename Message::value_type;
//        Message m_message;
//        holder(Message&& m)
//        : m_message{std::move(m)}
//        {
//        }
//
//        unsigned char* data() noexcept override
//        {
//            return reinterpret_cast<unsigned char*>(m_message.data());
//        }
//        const unsigned char* data() const noexcept override
//        {
//            return reinterpret_cast<const unsigned char*>(m_message.data());
//        }
//        std::size_t size() const noexcept override { return sizeof(value_type) * m_message.size(); }
//    };
//
//    unsigned char* __restrict m_data = nullptr;
//    std::size_t            m_size;
//    std::unique_ptr<iface> m_ptr;
//    std::shared_ptr<char>  m_ptr2;
//
//    /** @brief Construct from an r-value: moves the message inside the type-erased structure.
//                      * Requires the message not to reallocate during the move. Note, that this operation will allocate
//                      * storage on the heap for the holder structure of the message.
//                      * @tparam Message a message type
//                      * @param m a message */
//    template<class Message>
//    any_message(Message&& m)
//    : m_data{reinterpret_cast<unsigned char*>(m.data())}
//    , m_size{m.size() * sizeof(typename Message::value_type)}
//    , m_ptr{std::make_unique<holder<Message>>(std::move(m))}
//    {
//    }
//
//    /** @brief Construct from a reference: copies the pointer to the data and size of the data.
//                      * Note, that this operation will not allocate storage on the heap.
//                      * @tparam T a message type
//                      * @param m a ref_message to a message. */
//    template<typename T>
//    any_message(ref_message<T>&& m)
//    : m_data{reinterpret_cast<unsigned char*>(m.data())}
//    , m_size{m.size() * sizeof(T)}
//    {
//    }
//
//    /** @brief Construct from a shared pointer: will share ownership with the shared pointer (aliasing)
//                      * and keeps the message wrapped by the shared pointer alive. Note, that this operation may
//                      * allocate on the heap, but does not allocate storage for the holder structure.
//                      * @tparam Message a message type
//                      * @param sm a shared pointer to a message*/
//    template<typename Message>
//    any_message(std::shared_ptr<Message>& sm)
//    : m_data{reinterpret_cast<unsigned char*>(sm->data())}
//    , m_size{sm->size() * sizeof(typename Message::value_type)}
//    , m_ptr2(sm, reinterpret_cast<char*>(sm.get()))
//    {
//    }
//
//    any_message(any_message&&) = default;
//    any_message& operator=(any_message&&) = default;
//
//    unsigned char*       data() noexcept { return m_data; }
//    const unsigned char* data() const noexcept { return m_data; }
//    std::size_t          size() const noexcept { return m_size; }
//};

/** @brief A container for storing callbacks and progressing them.
  * @tparam RequestType a request type */
template<class RequestType>
class callback_queue
{
  public: // member types
    using message_type = detail::message_buffer;
    using request_type = RequestType;
    using rank_type = communicator::rank_type;
    using tag_type = communicator::tag_type;
    using cb_type = std::function<void(message_type, rank_type, tag_type)>;
    //using ref_message_type = detail::message_buffer*;
    //using ref_cb_type = std::function<void(detail::message_buffer&, rank_type, tag_type)>;

    // internal element which is stored in the queue
    struct element_type
    {
        message_type m_msg;
        std::size_t  m_size;
        rank_type    m_rank;
        tag_type     m_tag;
        cb_type      m_cb;
        request_type m_request;
        //request      m_request;

        element_type(message_type&& msg, std::size_t s, rank_type r, tag_type t, cb_type&& cb,
            request_type&& req)
        : m_msg{std::move(msg)}
        , m_size{s}
        , m_rank{r}
        , m_tag{t}
        , m_cb{std::move(cb)}
        , m_request{std::move(req)}
        {
        }
        element_type(element_type const&) = delete;
        element_type(element_type&&) = default;
        element_type& operator=(element_type const&) = delete;
        element_type& operator=(element_type&& other) = default;
    };

    using queue_type = std::vector<element_type>;

  private: // members
    queue_type m_queue;
    queue_type m_tmp_queue_1;
    queue_type m_tmp_queue_2;
    bool       in_progress = false;

  public:
    int m_progressed_cancels = 0;

  public: // ctors
    callback_queue()
    {
        m_queue.reserve(256);
        m_tmp_queue_1.reserve(256);
        m_tmp_queue_2.reserve(256);
    }

  public: // member functions
    /** @brief Add a callback to the queue and receive a completion handle (request).
                      * @tparam Callback callback type
                      * @param msg the message (data)
                      * @param rank the destination/source rank
                      * @param tag the message tag
                      * @param fut the send/recv operation's return value
                      * @param cb the callback
                      * @return returns a completion handle */
    //template<typename Callback>
    //request
    void enqueue(message_type&& msg, std::size_t size, rank_type rank, tag_type tag,
        request_type&& req, cb_type&& cb)
    {
        //request m_req{std::make_shared<request_state>(false, m_queue.size())};
        m_queue.push_back(element_type{
            std::move(msg), size, rank, tag, std::move(cb), std::move(req) /*, m_req*/});
        //return m_req;
    }

    //using cb_type = std::function<void(request_type, message_type, rank_type, tag_type)>;
    template<typename CallBack>
    void dequeue(rank_type rank, tag_type tag, CallBack&& cb)
    {
        auto first = std::find_if(m_queue.begin(), m_queue.end(),
            [rank, tag](element_type& e) { return ((e.m_rank == rank) && (e.m_tag == tag)); });
        if (first == m_queue.end()) return;
        if (cb(std::move(first->m_request), std::move(first->m_msg), first->m_size))
        {
            for (auto it = first + 1; it != m_queue.end(); ++it, ++first) (*first) = std::move(*it);
            m_queue.erase(first, m_queue.end());
        }

        //element_type tmp = std::move(*first);
        //for (auto it=first+1; it!=m_queue.end(); ++it, ++first)
        //{
        //    (*first) = std::move(*it);
        //}
        //return true;
    }

    auto size() const noexcept { return m_queue.size(); }

    /** @brief progress the queue and call the callbacks if the futures are ready. Note, that the order
                      * of progression is not defined.
                      * @return number of progressed elements */
    int progress()
    {
        if (in_progress) return 0;
        in_progress = true;

        int completed = 0;
        while (true)
        {
            auto it = test_any(m_queue.begin(), m_queue.end(),
                [](auto& e) -> request_type& { return e.m_request; });
            if (it != m_queue.end())
            {
                ++completed;
                element_type e = std::move(*it);
                if ((std::size_t)((it - m_queue.begin()) + 1) < m_queue.size())
                    *it = std::move(m_queue.back());
                m_queue.pop_back();
                e.m_cb(std::move(e.m_msg), e.m_rank, e.m_tag);
            }
            else
                break;
        }
        in_progress = false;
        return completed;

        //m_tmp_queue_1.clear();
        //m_tmp_queue_1.reserve(m_queue.size());
        //m_tmp_queue_2.clear();
        //m_tmp_queue_2.reserve(m_queue.size());

        //std::size_t qs;
        //do
        //{
        //    qs = m_queue.size();
        //    for (auto& e : m_queue)
        //    {
        //        if (e.m_request.is_ready()) m_tmp_queue_2.push_back(std::move(e));
        //        else
        //            m_tmp_queue_1.push_back(std::move(e));
        //    }
        //} while (qs != m_queue.size());

        //{
        //    using namespace std;
        //    swap(m_queue, m_tmp_queue_1);
        //}

        //for (auto& e : m_tmp_queue_2) { e.m_cb(std::move(e.m_msg), e.m_rank, e.m_tag); }

        //in_progress = false;

        //return m_tmp_queue_2.size();

        //int completed = 0;
        ////while (true)
        ////{
        //const auto first = std::stable_partition(m_queue.begin(), m_queue.end(),
        //    [](auto& x) -> bool { return !(x.m_request.is_ready()); });
        ////auto it = future_type::test_any(
        ////    m_queue.begin(), m_queue.end(), [](auto& x) -> future_type& { return x.m_future; });
        ////if (it != m_queue.end())

        //for (auto it = first; it != m_queue.end(); ++it) { m_tmp_queue.push_back(std::move(*it)); }
        //m_queue.erase(first, m_queue.end());
        ////const std::size_t n_pops = m_queue.end()-first;
        ////for (auto it = first; it != m_queue.end(); ++it)
        //for (auto& element : m_tmp_queue)
        //{
        //    //auto& element = *it;
        //    //const std::size_t i = it - m_queue.begin();
        //    element.m_cb(std::move(element.m_msg), element.m_rank, element.m_tag);
        //    ++completed;
        //    //element.m_request.m_request_state->m_ready = true;
        //    /*if (i + 1 < m_queue.size())
        //        {
        //            element = std::move(m_queue.back());
        //            element.m_request.m_request_state->m_index = i;
        //        }
        //        */
        //}
        //m_tmp_queue.clear();
        ////else
        ////    break;
        ////}
        //
        //return completed;
    }

    ///** @brief Cancel a callback
    //                  * @param index the queue index - access to this index is given through the request returned when
    //                  * enqueing.
    //                  * @return true if cancelling was successful */
    //bool cancel(unsigned int index)
    //{
    //    auto& element = m_queue[index];
    //    auto  res = element.m_future.cancel();
    //    if (!res) return false;
    //    if (m_queue.size() > index + 1)
    //    {
    //        element = std::move(m_queue.back());
    //        element.m_request.m_request_state->m_index = index;
    //    }
    //    m_queue.pop_back();
    //    ++m_progressed_cancels;
    //    return true;
    //}
};

///** @brief a class to return the number of progressed callbacks */
//struct progress_status
//{
//    int m_num_sends = 0;
//    int m_num_recvs = 0;
//    int m_num_cancels = 0;
//
//    int num() const noexcept { return m_num_sends + m_num_recvs + m_num_cancels; }
//    int num_sends() const noexcept { return m_num_sends; }
//    int num_recvs() const noexcept { return m_num_recvs; }
//    int num_cancels() const noexcept { return m_num_cancels; }
//
//    progress_status& operator+=(const progress_status& other) noexcept
//    {
//        m_num_sends += other.m_num_sends;
//        m_num_recvs += other.m_num_recvs;
//        m_num_cancels += other.m_num_cancels;
//        return *this;
//    }
//};

template<class RequestType, class HandleType>
class callback_queue2
{
  public: // member types
    using request_type = RequestType;
    using rank_type = communicator::rank_type;
    using tag_type = communicator::tag_type;
    using cb_type = util::unique_function<void()>;
    using handle_type = HandleType;
    using handle_ptr = std::shared_ptr<handle_type>;

    struct element_type
    {
        //message_type* m_msg;
        //std::size_t  m_size;
        //rank_type    m_rank;
        //tag_type     m_tag;
        cb_type      m_cb;
        request_type m_request;
        handle_ptr   m_handle;

        element_type(/*message_type* msg, std::size_t s, rank_type r, tag_type t,*/ cb_type&& cb,
            request_type&& req, handle_ptr&& h)
        /*: m_msg{msg}
        , m_size{s}
        , m_rank{r}
        , m_tag{t}*/
        : m_cb{std::move(cb)}
        , m_request{std::move(req)}
        , m_handle{std::move(h)}
        {
        }
        element_type(element_type const&) = delete;
        element_type(element_type&&) = default;
        element_type& operator=(element_type const&) = delete;
        element_type& operator=(element_type&& other) = default;
    };

    using queue_type = std::vector<element_type>;

  private: // members
    queue_type m_queue;
    queue_type m_tmp_queue;
    bool       in_progress = false;

  public: // ctors
    callback_queue2()
    {
        m_queue.reserve(256);
        m_tmp_queue.reserve(256);
    }

  public:        // member functions
    void enqueue(/*message_type* msg, std::size_t size, rank_type rank, tag_type tag,*/
        request_type&& req, cb_type&& cb, handle_ptr&& h)
    {
        m_queue.push_back(
            element_type{/*msg, size, rank, tag, */ std::move(cb), std::move(req), std::move(h)});
        m_queue.back().m_handle->m_index = m_queue.size()-1;
    }

    //template<typename CallBack>
    //void dequeue(rank_type rank, tag_type tag, CallBack&& cb)
    //{
    //    auto first =
    //        std::find_if(m_queue.begin(), m_queue.end(), [rank, tag](element_type& e) {
    //            return ((e.m_rank == rank) && (e.m_tag == tag));
    //        });
    //    if (first == m_queue.end()) return;
    //    if (cb(std::move(first->m_request), std::move(first->m_msg), first->m_size))
    //    {
    //        for (auto it = first + 1; it != m_queue.end(); ++it, ++first) (*first) = std::move(*it);
    //        m_queue.erase(first, m_queue.end());
    //    }

    //}

    auto size() const noexcept { return m_queue.size(); }

    // don't call in a callback!
    int progress()
    {
        if (in_progress) return 0;
        in_progress = true;

        partition(m_queue, m_tmp_queue);

        //m_tmp_queue.clear();
        //const auto qs = m_queue.size();
        //m_tmp_queue.reserve(qs);
        //std::size_t j = 0;
        //for (std::size_t i=0; i<qs; ++i)
        //{
        //    auto& e = m_queue[i];
        //    if (e.m_request.is_ready())
        //    {
        //        m_tmp_queue.push_back(std::move(e));
        //    }
        //    else if (i>j)
        //    {
        //        e.m_handle->m_index = j;
        //        m_queue[j] = std::move(e);
        //        ++j;
        //    }
        //    else
        //    {
        //        ++j;
        //    }
        //}
        //m_queue.erase(m_queue.end()-m_tmp_queue.size(),m_queue.end());
        int completed = m_tmp_queue.size();
        for (auto& e : m_tmp_queue) e.m_cb();

        //int completed = 0;
        //while (true)
        //{
        //    auto it = test_any(m_queue.begin(), m_queue.end(),
        //        [](auto& e) -> request_type& { return e.m_request; });
        //    if (it != m_queue.end())
        //    {
        //        ++completed;
        //        element_type e = std::move(*it);
        //        if ((std::size_t)((it - m_queue.begin()) + 1) < m_queue.size())
        //        {
        //            *it = std::move(m_queue.back());
        //            it->m_handle->m_index = e.m_handle->m_index;
        //        }
        //        m_queue.pop_back();
        //        e.m_cb(); //*e.m_msg, e.m_rank, e.m_tag);
        //    }
        //    else
        //        break;
        //}
        in_progress = false;
        return completed;
    }

    bool cancel(std::size_t index)
    {
        if (m_queue[index].m_request.cancel())
        {
            if (index + 1 < m_queue.size())
            {
                m_queue[index] = std::move(m_queue.back());
                m_queue[index].m_handle->m_index = index;
            }
            m_queue.pop_back();
            return true;
        }
        else
            return false;
    }
};

} // namespace oomph
