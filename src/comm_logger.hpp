#pragma once

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
        if (!m_file) return;
        auto ns = timestamp_ns();
        std::fprintf(m_file, "%lld,%d,%p,%zu,send,%d,%zu\n", (long long)ns, rank, comm, group_id,
            peer, size_bytes);
    }

    void log_recv(rank_type rank, const void* comm, std::size_t group_id, rank_type peer,
        std::size_t size_bytes)
    {
        if (!m_file) return;
        auto ns = timestamp_ns();
        std::fprintf(m_file, "%lld,%d,%p,%zu,recv,%d,%zu\n", (long long)ns, rank, comm, group_id,
            peer, size_bytes);
    }

  private:
    comm_logger()
    {
        auto* path = std::getenv("OOMPH_COMM_LOG");
        if (!path || std::strlen(path) == 0) return;
        m_file = std::fopen(path, "a");
        if (!m_file) return;
        std::fprintf(m_file, "timestamp_ns,rank,comm,group_id,direction,peer,size_bytes\n");
    }

    ~comm_logger()
    {
        if (m_file) std::fclose(m_file);
    }

    comm_logger(const comm_logger&) = delete;
    comm_logger& operator=(const comm_logger&) = delete;

    static long long timestamp_ns()
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }

    std::FILE* m_file = nullptr;
};

} // namespace oomph
