/**
 * @file array_utils.hpp
 * @brief Utility functions for working with arrays and index sequences.
 *
 * This file provides utility functions for creating arrays from index sequences
 * and computing products of elements in index sequences.
 */

#ifndef SQUINT_UTIL_ARRAY_UTILS_HPP
#define SQUINT_UTIL_ARRAY_UTILS_HPP

#include "squint/core/concepts.hpp"
#include "squint/core/layout.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace squint {

/**
 * @brief Creates an array from an index sequence.
 *
 * This function takes an index sequence and returns an std::array
 * containing the indices as elements.
 *
 * @tparam Ix Variadic template parameter for indices.
 * @param unused An index sequence (unused, only used for type deduction).
 * @return std::array<std::size_t, sizeof...(Ix)> An array containing the indices.
 */
template <std::size_t... Ix> constexpr auto make_array(std::index_sequence<Ix...> /*unused*/) {
    return std::array{Ix...};
}

/**
 * @brief Computes the product of elements in an index sequence.
 *
 * This function multiplies all the indices in the given index sequence.
 *
 * @tparam Ix Variadic template parameter for indices.
 * @param unused An index sequence (unused, only used for type deduction).
 * @return std::size_t The product of all indices in the sequence.
 */
template <std::size_t... Ix> constexpr auto product(std::index_sequence<Ix...> /*unused*/) -> std::size_t {
    return (Ix * ...);
}

/**
 * @brief Computes the sum of elements in an index sequence.
 *
 * This function adds all the indices in the given index sequence.
 *
 * @tparam Ix Variadic template parameter for indices.
 * @param unused An index sequence (unused, only used for type deduction).
 * @return std::size_t The sum of all indices in the sequence.
 */
template <std::size_t... Ix> constexpr auto sum(std::index_sequence<Ix...> /*unused*/) -> std::size_t {
    return (Ix + ...);
}

// check all elements of an index sequence are equal
template <std::size_t... Ix> constexpr auto all_equal(std::index_sequence<Ix...> /*unused*/) -> bool {
    if constexpr (sizeof...(Ix) == 0) {
        // An empty sequence is considered to have all elements equal
        return true;
    } else {
        // Use fold expression to compare all elements with the first one
        return ((Ix == std::get<0>(std::tuple{Ix...})) && ...);
    }
}

// Helper function to check if tensor dimensions are divisible by subview dimensions
template <fixed_tensor T, typename SubviewShape> constexpr auto dimensions_divisible() -> bool {
    constexpr auto shape_arr = make_array(typename T::shape_type{});
    constexpr auto subview_arr = make_array(SubviewShape{});
    std::size_t min_length = shape_arr.size() < subview_arr.size() ? shape_arr.size() : subview_arr.size();
    for (std::size_t i = 0; i < min_length; ++i) {
        if (shape_arr[i] % subview_arr[i] != 0) {
            return false;
        }
    }
    return true;
}

// Helper to remove the first element from a sequence
template <typename Sequence> struct tail_sequence;

template <std::size_t First, std::size_t... Rest> struct tail_sequence<std::index_sequence<First, Rest...>> {
    using type = std::index_sequence<Rest...>;
};

// Alias template for tail_sequence
template <typename Sequence> using tail_sequence_t = typename tail_sequence<Sequence>::type;

// Helper to add an element to the beginning of a sequence
template <typename Sequence, std::size_t New> struct prepend_sequence;

template <std::size_t... Rest, std::size_t New> struct prepend_sequence<std::index_sequence<Rest...>, New> {
    using type = std::index_sequence<New, Rest...>;
};

// Alias template for prepend_sequence
template <typename Sequence, std::size_t New> using prepend_sequence_t = typename prepend_sequence<Sequence, New>::type;

// Helper to remove the last element from a sequence
template <typename Sequence> struct init_sequence;

template <std::size_t... Is> struct init_sequence<std::index_sequence<Is...>> {
    template <std::size_t... Ns>
    static auto helper(std::index_sequence<Ns...>)
        -> std::index_sequence<std::get<Ns>(make_array(std::index_sequence<Is...>{}))...>;

    using all_but_last = std::make_index_sequence<sizeof...(Is) - 1>;
    using type = decltype(helper(all_but_last{}));
};

// Alias template for init_sequence
template <typename Sequence> using init_sequence_t = typename init_sequence<Sequence>::type;

// Helper to append an element to a sequence
template <typename Sequence, std::size_t New> struct append_sequence;

template <std::size_t... Indices, std::size_t New> struct append_sequence<std::index_sequence<Indices...>, New> {
    using type = std::index_sequence<Indices..., New>;
};

// Alias template for append_sequence
template <typename Sequence, std::size_t New> using append_sequence_t = typename append_sequence<Sequence, New>::type;

// Helper to remove the last N elements from a sequence
template <typename Sequence, std::size_t N> struct remove_last_n;

template <std::size_t... Is, std::size_t N> struct remove_last_n<std::index_sequence<Is...>, N> {
    static_assert(N <= sizeof...(Is), "Cannot remove more elements than the sequence contains");

    template <std::size_t... Ns>
    static auto helper(std::index_sequence<Ns...>) -> std::index_sequence<std::get<Ns>(std::array{Is...})...>;

    using type = decltype(helper(std::make_index_sequence<sizeof...(Is) - N>{}));
};

// Alias template for remove_last_n
template <typename Sequence, std::size_t N> using remove_last_n_t = typename remove_last_n<Sequence, N>::type;

} // namespace squint

#endif // SQUINT_UTIL_ARRAY_UTILS_HPP