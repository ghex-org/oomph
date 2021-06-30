#pragma once

#include <oomph/util/stack_storage.hpp>

namespace oomph
{
namespace util
{
template<typename T, std::size_t BufferSize,
    std::size_t Alignment = std::alignment_of<std::max_align_t>::value>
class stack_pimpl
{
    util::stack_storage<T, BufferSize, Alignment> m;

  public:
    template<typename... Args>
    stack_pimpl(Args&&... args);
    stack_pimpl(stack_pimpl const&) = delete;
    stack_pimpl(stack_pimpl&&);
    stack_pimpl& operator=(stack_pimpl const&) = delete;
    stack_pimpl& operator=(stack_pimpl&&);
    ~stack_pimpl();

    T*       operator->();
    T const* operator->() const;
    T&       operator*();
    T const& operator*() const;
};

} // namespace util
} // namespace oomph
