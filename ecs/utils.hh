#pragma once
namespace ecs {
template <typename T, typename... Ts>
struct Index;
template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};
template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {};
}  // namespace ecs
