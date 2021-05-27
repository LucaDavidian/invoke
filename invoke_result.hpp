#ifndef INVOKE_RESULT_H
#define INVOKE_RESULT_H

#include <type_traits>

namespace Detail
{
    // variable template to detect if a type is a std::reference_wrapper<T>

    // primary variable template
    template <typename>
    constexpr bool IsReferenceWrapperV = false;

    // partial variable template specialization for std::reference_wrapper
    template <typename T>
    constexpr bool IsReferenceWrapperV<std::reference_wrapper<T>> = true;

    // primary template, instantiated when callable is not a pointer to member (it is a function object)
    template <typename T>
    struct InvokeResultImpl
    {
        template <typename F, typename... Args>
        static auto Call(F &&f, Args&&... args) -> decltype(std::forward<F>(f)(std::forward<Args>(args)...));
    };

    // partial specialization, instantiated when callable is a pointer to member
    template <typename C, typename T>
    struct InvokeResultImpl<T C::*>
    {
        // get the type of the callable (using SFINAE):

        // object
        template <typename U, typename Decayed = std::decay_t<U>, typename = typename std::enable_if<std::is_base_of<C, Decayed>::value>::type>
        static auto Get(U &&u) -> U&&;

        // reference wrapper
        template <typename U, typename = std::enable_if_t<IsReferenceWrapperV<U>>>
        static auto Get(U &&u) -> decltype(u.get());

        // pointer
        template <typename U, typename Decayed = std::decay_t<U>, typename = typename std::enable_if<!std::is_base_of<C, Decayed>::value>::type, typename = std::enable_if_t<!IsReferenceWrapperV<U>>>
        static auto Get(U &&u) -> decltype(*std::forward<U>(u));

        // call the callable and get the type of the result: 

        // pointer to member function (first argument is the base object)
        template <typename F, typename Arg0, typename... Args>
        static auto Call(F &&f, Arg0 &&arg0, Args... args) -> decltype((Get(std::forward<Arg0>(arg0)).*f)(std::forward<Args>(args)...));

        // pointer to data member (first argument is the base object)
        template <typename F, typename Arg0>
        static auto Call(F &&f, Arg0 &&arg0) -> decltype(Get(std::forward<Arg0>(arg0)).*f);
    };

    template <typename F, typename... Args, typename Decayed = std::decay_t<F>>
    auto InvokeImpl(F &&f, Args&&... args) ->decltype(InvokeResultImpl<Decayed>::Call(std::forward<F>(f), std::forward<Args>(args)...));

}  // namespace Detail

template <typename T>
struct ResultOf;

template <typename F, typename... Args>
struct ResultOf<F(Args...)>
{
    using Type = decltype(Detail::InvokeImpl(std::declval<F>(), std::declval<Args>()...));
};

template <typename T>
using ResultOfT = typename ResultOf<T>::Type;


#endif  // INVOKE_RESULT_H