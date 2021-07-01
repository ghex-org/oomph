#pragma once

#include <oomph/communicator.hpp>
#include <mutex>
#include <set>

namespace oomph
{
struct tl_comm
{
    context_impl*       m_context = nullptr;
    communicator::impl* m_communicator = nullptr;

    tl_comm(context_impl* c);
    ~tl_comm();
};

class tl_comm_holder
{
  private:
    std::set<communicator::impl**> m_ptrs;
    std::mutex                     m_mutex;

  public:
    tl_comm_holder() = default;

    // called at context destruction
    ~tl_comm_holder()
    {
        for (auto c : m_ptrs) destroy(c);
    }

    // called at thread construction
    void insert(communicator::impl** c)
    {
        m_mutex.lock();
        m_ptrs.insert(c);
        m_mutex.unlock();
    }

    // called at thread destruction
    void remove(communicator::impl** c)
    {
        m_mutex.lock();
        m_ptrs.erase(m_ptrs.find(c));
        destroy(c);
        m_mutex.unlock();
    }

  private:
    void destroy(communicator::impl** c);
};

} // namespace oomph
