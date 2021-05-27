#ifndef INVOKE_H
#define INVOKE_H

#include <type_traits>
#include <utility>

namespace Detail
{

    // trait to detect if type is a std::reference_wrapper<T>
    template <typename T>
    struct IsReferenceWrapper : std::false_type {};

    template <typename T>
    struct IsReferenceWrapper<std::reference_wrapper<T>> : std::true_type {};

    // callable is a pointer to member (first argument is the base object)
    template <typename C, typename T, typename Arg0, typename... Args>
    decltype(auto) Invoke(T C::*ptr, Arg0 &&arg0, Args&&... args)
    {
        if constexpr (std::is_member_function_pointer<std::remove_reference_t<decltype(ptr)>>::value)
        {
            if constexpr (std::is_base_of<std::decay_t<Arg0>, C>::value)
                return (std::forward<Arg0>(arg0).*ptr)(std::forward<Args>(args)...);
            else if constexpr (IsReferenceWrapper<std::decay_t<Arg0>>::value)
                return (arg0.get().*ptr)(std::forward<Args>(args)...);
            else // arg0 is a pointer to an object of class type
                return ((*std::forward<Arg0>(arg0)).*ptr)(std::forward<Args>(args)...);
        }
        else  
        {
            static_assert(std::is_member_object_pointer<std::remove_reference_t<decltype(ptr)>>::value && sizeof...(Args) == 0);

            if constexpr (std::is_base_of<std::decay_t<Arg0>, C>::value)
                return std::forward<Arg0>(arg0).*ptr;
            else if constexpr (IsReferenceWrapper<std::decay_t<Arg0>>::value)
                return arg0.get().*ptr;
            else // arg0 is a pointer to an object of class type
                return (*std::forward<Arg0>(arg0)).*ptr;
        }
    }

    // callable is not a pointer to member (it is a function object)
    template <typename F, typename... Args>
    constexpr decltype(auto) Invoke(F &&f, Args&&... args)
    {
        return std::forward<F>(f)(std::forward<Args>(args)...);
    }

}  // namespace Detail

#include "invoke_result.hpp"

template <typename F, typename... Args>
//constexpr decltype(auto) Invoke(F &&f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
//constexpr typename ResultOf<F(Args...)>::Type Invoke(F &&f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
constexpr auto Invoke(F &&f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>) -> decltype(Detail::Invoke(std::forward<F>(f), std::forward<Args>(args)...))
{
    return Detail::Invoke(std::forward<F>(f), std::forward<Args>(args)...);  // chooses the right function template overload
}

#endif  // INVOKE_H