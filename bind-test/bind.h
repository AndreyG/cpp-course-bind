#pragma once

#include <utility>

template<typename F, typename IndexSequence, typename... Args>
class binder;

template<typename F, typename... Args, size_t... I>
class binder<F, std::index_sequence<I...>, Args...>
{
    F f;
    std::tuple<Args...> args;

private:
    template<typename Self>
    static decltype(auto) impl(Self && self)
    {
        return std::forward<Self>(self).f(std::get<I>(std::forward<Self>(self).args)...);
    }

public:
    binder(F && f, Args &&... args)
        : f(std::forward<F>(f))
        , args(std::forward<Args>(args)...)
    {}

    decltype(auto) operator() ()
    {
        return impl(*this);
    }

    decltype(auto) operator() () const
    {
        return impl(*this);
    }
};

template<typename F, typename... Args>
auto bind(F && f, Args &&... args)
{
    return binder<F, std::index_sequence_for<Args...>, Args...>(std::forward<F>(f), std::forward<Args>(args)...);
}