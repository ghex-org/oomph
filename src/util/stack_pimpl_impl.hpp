#pragma once

#include <utility>

namespace oomph
{
namespace util
{
template<typename T, std::size_t BufferSize, std::size_t Alignment>
stack_pimpl<T, BufferSize, Alignment>::stack_pimpl() = default;

template<typename T, std::size_t BufferSize, std::size_t Alignment>
template<typename... Args>
stack_pimpl<T, BufferSize, Alignment>::stack_pimpl(Args&&... args)
: m(std::forward<Args>(args)...)
{
}

template<typename T, std::size_t BufferSize, std::size_t Alignment>
stack_pimpl<T, BufferSize, Alignment>::stack_pimpl(
    stack_pimpl<T, BufferSize, Alignment>&&) = default;

template<typename T, std::size_t BufferSize, std::size_t Alignment>
stack_pimpl<T, BufferSize, Alignment>& stack_pimpl<T, BufferSize, Alignment>::operator=(
    stack_pimpl<T, BufferSize, Alignment>&&) = default;

template<typename T, std::size_t BufferSize, std::size_t Alignment>
stack_pimpl<T, BufferSize, Alignment>::~stack_pimpl() = default;

template<typename T, std::size_t BufferSize, std::size_t Alignment>
T*
stack_pimpl<T, BufferSize, Alignment>::operator->()
{
    return m.get();
}

template<typename T, std::size_t BufferSize, std::size_t Alignment>
T const*
stack_pimpl<T, BufferSize, Alignment>::operator->() const
{
    return m.get();
}

template<typename T, std::size_t BufferSize, std::size_t Alignment>
T&
stack_pimpl<T, BufferSize, Alignment>::operator*()
{
    return *m.get();
}

template<typename T, std::size_t BufferSize, std::size_t Alignment>
T const&
stack_pimpl<T, BufferSize, Alignment>::operator*() const
{
    return *m.get();
}

template<typename T, std::size_t BufferSize, std::size_t Alignment>
T
stack_pimpl<T, BufferSize, Alignment>::release()
{
    return m.release();
}

} // namespace util
} // namespace oomph
