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
        Arg& extract(CallArgs &&...) &
        {
            return arg;
        }

        template<typename... CallArgs>
        Arg const & extract(CallArgs &&...) const &
        {
            return arg;
        }

        template<typename... CallArgs>
        Arg && extract(CallArgs &&...) &&
        {
            return std::move(arg);
        }
    };

    template<typename Arg>
    struct simple_arg_holder<std::reference_wrapper<Arg>>
    {
        std::reference_wrapper<Arg> arg;

        template<typename... CallArgs>
        Arg& extract(CallArgs &&...)
        {
            return arg.get();
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

    template<typename MemberPointer>
    struct get_class_from_member_pointer;

    template<typename T, typename C>
    struct get_class_from_member_pointer<T C::*>
    {
        using type = C;
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
    decltype(auto) operator() (CallArgs &&... call_args) &
    {
        return Derived::impl(self(), std::index_sequence_for<Args...>(), std::forward<CallArgs>(call_args)...);
    }

    template<typename... CallArgs>
    decltype(auto) operator() (CallArgs &&... call_args) const &
    {
        return Derived::impl(self(), std::index_sequence_for<Args...>(), std::forward<CallArgs>(call_args)...);
    }

    template<typename... CallArgs>
    decltype(auto) operator() (CallArgs &&... call_args) &&
    {
        return Derived::impl(std::move(self()), std::index_sequence_for<Args...>(), std::forward<CallArgs>(call_args)...);
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

template<class Derived, typename F, typename Object, typename... Args>
class member_pointer_binder
    : protected binder_base<Derived, F, Args...>
{
    using Base = binder_base<Derived, F, Args...>;
    using class_type = typename detail::get_class_from_member_pointer<std::remove_reference_t<F>>::type;

    template<typename T>
    static constexpr bool is_convertible_to_class_type
        = std::is_convertible_v<std::add_lvalue_reference_t<std::remove_cv_t<std::remove_reference_t<T>>>, class_type &>;

protected:
    detail::make_arg_holder_t<Object> object;

    template<typename Ptr>
    static decltype(auto) get_object(Ptr && ptr, ...)
    {
        return *std::forward<Ptr>(ptr);
    }

    template<typename T>
    static T& get_object(std::reference_wrapper<T> ref, int)
    {
        return ref.get();
    }

    template<typename T, typename = std::enable_if_t<is_convertible_to_class_type<T>>>
    static T&& get_object(T && t, int)
    {
        return std::forward<T>(t);
    }

    using Base::operator();

public:
    member_pointer_binder(F && f, Object && object, Args &&... args)
        : Base(std::forward<F>(f), std::forward<Args>(args)...)
        , object { std::forward<Object>(object) }
    {}
};

template<typename F, typename Object, typename... Args>
class member_function_binder : member_pointer_binder<member_function_binder<F, Object, Args...>, F, Object, Args...>
{
    using Base = member_pointer_binder<member_function_binder<F, Object, Args...>, F, Object, Args...>;

public:
    using Base::Base;
    using Base::operator();

    template<typename Self, size_t... I, typename... CallArgs>
    static decltype(auto) impl(Self && self, std::index_sequence<I...>, CallArgs &&... call_args)
    {
        return (Base::get_object(std::forward<Self>(self).object.extract(std::forward<CallArgs>(call_args)...), 0) .* std::forward<Self>(self).f)
            (std::get<I>(std::forward<Self>(self).args).extract(std::forward<CallArgs>(call_args)...)...);
    }
};

template<typename F, typename Object, typename... Garbage/* only for VS 2015*/>
class member_data_binder : member_pointer_binder<member_data_binder<F, Object>, F, Object>
{
    static_assert(sizeof...(Garbage) == 0, "no garbage expected");
    static_assert(std::is_member_object_pointer_v<std::remove_reference_t<F>>, "member object pointer expected");

    using Base = member_pointer_binder<member_data_binder<F, Object>, F, Object>;

public:
    using Base::Base;
    using Base::operator();

    template<typename Self, typename... CallArgs>
    static decltype(auto) impl(Self && self, std::index_sequence<>, CallArgs &&... call_args)
    {
        static_assert(sizeof...(call_args) <= 1, "sizeof...(call_args) > 1");
        return Base::get_object(std::forward<Self>(self).object.extract(std::forward<CallArgs>(call_args)...), 0) .* std::forward<Self>(self).f;
    }
};

template<typename F, typename... Args>
using select_member_pointer_binder =
    std::conditional_t<std::is_member_function_pointer_v<std::remove_reference_t<F>>
        , member_function_binder<F, Args...>
        , member_data_binder<F, Args...>
    >;

template<typename F, typename... Args>
using select_binder = std::conditional_t<std::is_member_pointer_v<std::remove_reference_t<F>>
    , select_member_pointer_binder<F, Args...>
    , free_function_binder<F, Args...>
>;

template<typename F, typename... Args>
auto bind(F && f, Args &&... args)
{
    return select_binder<F, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
}