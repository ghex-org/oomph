#pragma once

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <oomph/types.hpp>

namespace oomph
{

class comm_logger
{
  public:
    static comm_logger& get()
    {
        static comm_logger instance;
        return instance;
    }

    void log_send(rank_type rank, const void* comm, std::size_t group_id, rank_type peer,
        std::size_t size_bytes)
    {
        if (m_fd < 0) return;
        auto ns = timestamp_ns();
        char buf[256];
        int n = std::snprintf(buf, sizeof(buf), "%lld,%d,%p,%zu,send,%d,%zu\n", (long long)ns, rank,
            comm, group_id, peer, size_bytes);
        ::write(m_fd, buf, n);
    }

    void log_recv(rank_type rank, const void* comm, std::size_t group_id, rank_type peer,
        std::size_t size_bytes)
    {
        if (m_fd < 0) return;
        auto ns = timestamp_ns();
        char buf[256];
        int n = std::snprintf(buf, sizeof(buf), "%lld,%d,%p,%zu,recv,%d,%zu\n", (long long)ns, rank,
            comm, group_id, peer, size_bytes);
        ::write(m_fd, buf, n);
    }

  private:
    comm_logger()
    {
        auto* path = std::getenv("OOMPH_COMM_LOG");
        if (!path || std::strlen(path) == 0) return;
        m_fd = ::open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (m_fd < 0) return;
        const char header[] = "timestamp_ns,rank,comm,group_id,direction,peer,size_bytes\n";
        ::write(m_fd, header, sizeof(header) - 1);
    }

    ~comm_logger()
    {
        if (m_fd >= 0) ::close(m_fd);
    }

    comm_logger(const comm_logger&) = delete;
    comm_logger& operator=(const comm_logger&) = delete;

    static long long timestamp_ns()
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    int m_fd = -1;
};

} // namespace oomph
