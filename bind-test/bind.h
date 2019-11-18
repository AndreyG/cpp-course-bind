#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace detail
{
    template<typename Arg>
    struct simple_arg_holder
    {
        Arg arg;

        template<typename... CallArgs>
        Arg& extract(CallArgs &&...)
        {
            return arg;
        }

        template<typename... CallArgs>
        Arg const & extract(CallArgs &&...) const
        {
            return arg;
        }
    };

    template<size_t I>
    class placeholder;

    template<typename Arg>
    constexpr bool is_placeholder = false;

    template<size_t I>
    constexpr bool is_placeholder<placeholder<I>> = true;

    template<typename Arg>
    struct make_arg_holder : std::conditional<is_placeholder<Arg>, Arg, simple_arg_holder<Arg>> {};

    template<typename Arg>
    using make_arg_holder_t = typename make_arg_holder<std::decay_t<Arg>>::type;

    template<>
    class placeholder<0>
    {
    public:
        template<typename Arg, typename... Rest>
        static decltype(auto) extract(Arg && arg, Rest && ...)
        {
            return std::forward<Arg>(arg);
        }
    };

    template<size_t I>
    class placeholder
    {
    public:
        template<typename Arg, typename... Rest>
        static decltype(auto) extract(Arg &&, Rest && ... rest)
        {
            return placeholder<I - 1>::extract(std::forward<Rest>(rest)...);
        }
    };
}

#define PLACEHOLDER(I) constexpr detail::placeholder<I - 1> _ ## I

PLACEHOLDER(1);
PLACEHOLDER(2);
PLACEHOLDER(3);
PLACEHOLDER(4);

#undef PLACEHOLDER

template<typename F, typename IndexSequence, typename... Args>
class binder;

template<typename F, typename... Args, size_t... I>
class binder<F, std::index_sequence<I...>, Args...>
{
    std::decay_t<F> f;
    std::tuple<detail::make_arg_holder_t<Args>...> args;

private:
    template<typename Self, typename... CallArgs>
    static decltype(auto) impl(Self && self, CallArgs &&... call_args)
    {
        return std::forward<Self>(self).f(std::get<I>(std::forward<Self>(self).args).extract(std::forward<CallArgs>(call_args)...)...);
    }

    template<typename Arg>
    static auto make_arg_holder(Arg && arg)
    {
        return detail::make_arg_holder_t<Arg> { std::forward<Arg>(arg) };
    }

public:
    binder(F && f, Args &&... args)
        : f(std::forward<F>(f))
        , args(make_arg_holder(std::forward<Args>(args))...)
    {}

    template<typename... CallArgs>
    decltype(auto) operator() (CallArgs &&... call_args)
    {
        return impl(*this, std::forward<CallArgs>(call_args)...);
    }

    template<typename... CallArgs>
    decltype(auto) operator() (CallArgs &&... call_args) const
    {
        return impl(*this, std::forward<CallArgs>(call_args)...);
    }
};

template<typename F, typename... Args>
auto bind(F && f, Args &&... args)
{
    return binder<F, std::index_sequence_for<Args...>, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
}