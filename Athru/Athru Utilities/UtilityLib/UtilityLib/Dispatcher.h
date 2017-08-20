#pragma once

/*Contributed by StackOverflow user Yakk on Jan 5 2017 at 05:34 AEDT*/

// Coding style edited by Paul Ferris

#include <type_traits>
#include <tuple>
#include <utility>

template<std::size_t I>
using index_t = std::integral_constant<std::size_t, I>;

constexpr index_t<0> dispatch_index()
{
	return{};
}

template<class B0, class...Bools, class = std::enable_if_t<B0::value>>
constexpr index_t<0> dispatch_index(B0, Bools...)
{
	return{};
}

template<class B0, class...Bools, class = std::enable_if_t<!B0::value>>
constexpr auto dispatch_index(B0, Bools...bools)
{
	return index_t<dispatch_index(bools...) + 1>{};
}

template<class...Bools>
constexpr auto Dispatch(Bools...bools)
{
	using get_index = decltype(dispatch_index(bools...));
	return [](auto&&...args)
	{
		using std::get;
		return get<get_index::value>(std::forward_as_tuple(decltype(args)(args)...));
	};
}