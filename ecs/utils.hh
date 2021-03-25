#pragma once
namespace ecs {
///
/// @brief Find the provided T in the variadic template pack
///
template <typename T, typename... Ts>
struct Index;
template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};
template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {};

///
/// @brief Apply a functor to tuple element at the given runtime index. Note the lambda must be able to take in _every_
/// tuple element since this is a runtime thing.
///
template <typename... Ts>
class ApplyByIndex {
public:
    using Tuple = std::tuple<Ts...>;
    ApplyByIndex(Tuple& tuple) : tuple_(tuple) {}

    template <typename F>
    void operator()(size_t index, F&& f) {
        apply<F, 0, Ts...>(index, f, tuple_);
    }

    template <typename F>
    void operator()(F&& f) {
        for (size_t i = 0; i < sizeof...(Ts); ++i) apply<F, 0, Ts...>(i, f, tuple_);
    }

private:
    template <typename F, size_t I>
    static void apply(size_t, const F&, Tuple&) {
        throw std::runtime_error("ApplyByIndex reached end of tuple.");
    }

    template <typename F, size_t I, typename T, typename... Rest>
    static void apply(size_t index, F& f, Tuple& tuple) {
        if (index == I) {
            f(std::get<I>(tuple));
            return;
        }
        apply<F, I + 1, Rest...>(index, f, tuple);
    }

    Tuple& tuple_;
};

}  // namespace ecs
