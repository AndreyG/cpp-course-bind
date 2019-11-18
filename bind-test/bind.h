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

template<class Derived, typename F, typename... Args>
class binder_base
{
    template<typename Arg>
    static auto make_arg_holder(Arg && arg)
    {
        return detail::make_arg_holder_t<Arg> { std::forward<Arg>(arg) };
    }

    Derived       & self()       { return static_cast<Derived&>(*this); }
    Derived const & self() const { return static_cast<Derived const &>(*this); }

protected:
    std::decay_t<F> f;
    std::tuple<detail::make_arg_holder_t<Args>...> args;

public:
    binder_base(F && f, Args &&... args)
        : f(std::forward<F>(f))
        , args(make_arg_holder(std::forward<Args>(args))...)
    {}

public:
    template<typename... CallArgs>
    decltype(auto) operator() (CallArgs &&... call_args)
    {
        return Derived::impl(self(), std::index_sequence_for<Args...>(), std::forward<CallArgs>(call_args)...);
    }

    template<typename... CallArgs>
    decltype(auto) operator() (CallArgs &&... call_args) const
    {
        return Derived::impl(self(), std::index_sequence_for<Args...>(), std::forward<CallArgs>(call_args)...);
    }
};

template<typename F, typename... Args>
class free_function_binder
    : binder_base<free_function_binder<F, Args...>, F, Args...>
{
    using Base = binder_base<free_function_binder, F, Args...>;

public:
    using Base::operator ();
    using Base::Base;

    template<typename Self, size_t... I, typename... CallArgs>
    static decltype(auto) impl(Self && self, std::index_sequence<I...>, CallArgs &&... call_args)
    {
        return std::forward<Self>(self).f(std::get<I>(std::forward<Self>(self).args).extract(std::forward<CallArgs>(call_args)...)...);
    }
};

template<typename F, typename Object, typename... Args>
class member_function_binder
    : binder_base<member_function_binder<F, Object, Args...>, F, Args...>
{
    using Base = binder_base<member_function_binder, F, Args...>;

    std::decay_t<Object> object;

public:
    using Base::operator ();

    member_function_binder(F && f, Object && object, Args &&... args)
        : Base(std::forward<F>(f), std::forward<Args>(args)...)
        , object(std::forward<Object>(object))
    {}

    template<typename Self, size_t... I, typename... CallArgs>
    static decltype(auto) impl(Self && self, std::index_sequence<I...>, CallArgs &&... call_args)
    {
        return (std::forward<Self>(self).object .* std::forward<Self>(self).f)
            (std::get<I>(std::forward<Self>(self).args).extract(std::forward<CallArgs>(call_args)...)...);
    }
};

template<typename F, typename... Args>
using select_binder = std::conditional_t<std::is_member_function_pointer_v<std::remove_reference_t<F>>
    , member_function_binder<F, Args...>
    , free_function_binder<F, Args...>
>;

template<typename F, typename... Args>
auto bind(F && f, Args &&... args)
{
    return select_binder<F, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
}