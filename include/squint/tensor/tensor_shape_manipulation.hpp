/**
 * @file tensor_shape_manipulations.hpp
 * @brief Implementation of tensor shape manipulation methods.
 *
 * This file contains the implementations of various methods for manipulating the shape
 * of tensors, including reshaping, flattening, and transposing. These methods are
 * implemented for both fixed and dynamic shapes, and for const and non-const tensors.
 *
 */

#ifndef SQUINT_TENSOR_TENSOR_SHAPE_MANIPULATIONS_HPP
#define SQUINT_TENSOR_TENSOR_SHAPE_MANIPULATIONS_HPP

#include "squint/tensor/tensor.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace squint {

// helper all_less_than to check if all elements of a std::vector are less than a given value
auto all_less_than(const std::vector<size_t> &vec, size_t value) -> bool {
    return std::all_of(vec.begin(), vec.end(), [value](size_t x) { return x < value; });
}

// helper to apply index permutation to a std::vector
auto apply_permutation_vector(const std::vector<size_t> &vec, const std::vector<size_t> &permutation,
                       std::size_t pad_value) -> std::vector<size_t> {
    std::vector<size_t> result(permutation.size(), pad_value);
    for (std::size_t i = 0; i < vec.size(); ++i) {
        result[permutation[i]] = vec[i];
    }
    return result;
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <size_t... NewDims>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::reshape()
    requires(fixed_shape<Shape> && OwnershipType == ownership_type::owner)
{
    constexpr size_t new_size = (NewDims * ...);
    constexpr size_t current_size = product(Shape{});
    static_assert(new_size == current_size, "New shape must have the same number of elements as the original tensor");

    using NewShape = std::index_sequence<NewDims...>;
    using NewStrides = typename std::conditional_t<std::is_same_v<Strides, strides::column_major<Shape>>,
                                                   strides::column_major<NewShape>, strides::row_major<NewShape>>;

    return tensor<T, NewShape, NewStrides, ErrorChecking, ownership_type::reference, MemorySpace>(this->data());
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <size_t... NewDims>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::reshape() const
    requires(fixed_shape<Shape> && OwnershipType == ownership_type::owner)
{
    constexpr size_t new_size = (NewDims * ...);
    constexpr size_t current_size = product(Shape{});
    static_assert(new_size == current_size, "New shape must have the same number of elements as the original tensor");

    using NewShape = std::index_sequence<NewDims...>;
    using NewStrides = typename std::conditional_t<std::is_same_v<Strides, strides::column_major<Shape>>,
                                                   strides::column_major<NewShape>, strides::row_major<NewShape>>;

    return tensor<const T, NewShape, NewStrides, ErrorChecking, ownership_type::reference, MemorySpace>(this->data());
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::flatten()
    requires(OwnershipType == ownership_type::owner)
{
    if constexpr (fixed_shape<Shape>) {
        constexpr size_t total_size = product(Shape{});
        using FlatShape = std::index_sequence<total_size>;
        using FlatStrides = std::index_sequence<1>;

        return tensor<T, FlatShape, FlatStrides, ErrorChecking, ownership_type::reference, MemorySpace>(this->data());
    } else {
        std::vector<size_t> flat_shape = {this->size()};
        std::vector<size_t> flat_strides = {1};

        return tensor<T, std::vector<size_t>, std::vector<size_t>, ErrorChecking, ownership_type::reference,
                      MemorySpace>(this->data(), flat_shape, flat_strides);
    }
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::flatten() const
    requires(OwnershipType == ownership_type::owner)
{
    if constexpr (fixed_shape<Shape>) {
        constexpr size_t total_size = product(Shape{});
        using FlatShape = std::index_sequence<total_size>;
        using FlatStrides = std::index_sequence<1>;

        return tensor<const T, FlatShape, FlatStrides, ErrorChecking, ownership_type::reference, MemorySpace>(
            this->data());
    } else {
        std::vector<size_t> flat_shape = {this->size()};
        std::vector<size_t> flat_strides = {1};

        return tensor<const T, std::vector<size_t>, std::vector<size_t>, ErrorChecking, ownership_type::reference,
                      MemorySpace>(this->data(), flat_shape, flat_strides);
    }
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
void tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::reshape(std::vector<size_t> new_shape,
                                                                                   layout l)
    requires(dynamic_shape<Shape> && OwnershipType == ownership_type::owner)
{
    if constexpr (ErrorChecking == error_checking::enabled) {
        size_t new_size = std::accumulate(new_shape.begin(), new_shape.end(), 1ULL, std::multiplies<>());
        if (new_size != this->size()) {
            throw std::invalid_argument("New shape must have the same number of elements as the original tensor");
        }
    }

    shape_ = std::move(new_shape);
    strides_ = compute_strides(l);
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
void tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::reshape(std::vector<size_t> new_shape,
                                                                                   layout l) const
    requires(dynamic_shape<Shape> && OwnershipType == ownership_type::owner)
{
    if constexpr (ErrorChecking == error_checking::enabled) {
        size_t new_size = std::accumulate(new_shape.begin(), new_shape.end(), 1ULL, std::multiplies<>());
        if (new_size != this->size()) {
            throw std::invalid_argument("New shape must have the same number of elements as the original tensor");
        }
    }

    // For const version, we can't modify the tensor itself, so we return a new view
    return tensor<const T, std::vector<size_t>, std::vector<size_t>, ErrorChecking, ownership_type::reference,
                  MemorySpace>(this->data(), new_shape, compute_strides(l));
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <valid_index_permutation IndexPermutation>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::transpose()
    requires fixed_shape<Shape>
{

    static_assert(Shape::size() <= IndexPermutation::size(), "Index permutation must be at least as long as the shape");
    constexpr std::size_t last_stride = std::get<Shape::size() - 1>(make_array(Strides{}));
    return tensor<T, apply_permutation_t<Shape, IndexPermutation, 1>,
                  apply_permutation_t<Strides, IndexPermutation, last_stride>, ErrorChecking, ownership_type::reference,
                  MemorySpace>(this->data());
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <valid_index_permutation IndexPermutation>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::transpose() const
    requires fixed_shape<Shape>
{
    static_assert(Shape::size() <= IndexPermutation::size(), "Index permutation must be at least as long as the shape");
    constexpr std::size_t last_stride = std::get<Shape::size() - 1>(make_array(Strides{}));
    return tensor<const T, apply_permutation_t<Shape, IndexPermutation, 1>,
                  apply_permutation_t<Strides, IndexPermutation, last_stride>, ErrorChecking, ownership_type::reference,
                  MemorySpace>(this->data());
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::transpose(
    const std::vector<std::size_t> &index_permutation)
    requires dynamic_shape<Shape>
{
    if constexpr (ErrorChecking == error_checking::enabled) {
        if (index_permutation.size() <= shape_.size()) {
            throw std::invalid_argument(
                "Index permutation must have at least the same number of elements as the shape");
        }
        // if (!static_cast<bool>(std::unique(index_permutation.begin(), index_permutation.end()) ==
        //                        index_permutation.end())) {
        //     throw std::invalid_argument("Index permutation must not contain duplicates");
        // }
        if (!all_less_than(index_permutation, shape_.size())) {
            throw std::invalid_argument("Index permutation must be less than the number of dimensions");
        }
    }
    constexpr std::size_t last_stride = this->strides_[shape_.size() - 1];
    return tensor<T, std::vector<size_t>, std::vector<size_t>, ErrorChecking, ownership_type::reference, MemorySpace>(
        this->data(), apply_permutation_vector(shape_, index_permutation, 1),
        apply_permutation_vector(strides_, index_permutation, last_stride));
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::transpose(
    const std::vector<std::size_t> &index_permutation) const
    requires dynamic_shape<Shape>
{
    if constexpr (ErrorChecking == error_checking::enabled) {
        if (index_permutation.size() <= shape_.size()) {
            throw std::invalid_argument(
                "Index permutation must have at least the same number of elements as the shape");
        }
        // if (!static_cast<bool>(std::unique(index_permutation.begin(), index_permutation.end()) ==
        //                        index_permutation.end())) {
        //     throw std::invalid_argument("Index permutation must not contain duplicates");
        // }
        if (!all_less_than(index_permutation, shape_.size())) {
            throw std::invalid_argument("Index permutation must be less than the number of dimensions");
        }
    }
    constexpr std::size_t last_stride = this->strides_[shape_.size() - 1];
    return tensor<const T, std::vector<size_t>, std::vector<size_t>, ErrorChecking, ownership_type::reference,
                  MemorySpace>(this->data(), apply_permutation_vector(shape_, index_permutation, 1),
                               apply_permutation_vector(strides_, index_permutation, last_stride));
}

// Convenience method to apply transpose to a 1D or 2D tensor
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::transpose() {
    if constexpr (fixed_shape<Shape>) {
        if constexpr (Shape::size() == 1 || Shape::size() == 2) {
            return this->transpose<std::index_sequence<1, 0>>();
        } else {
            throw std::invalid_argument(
                "You must provide an index permutation for tensors with more than 2 dimensions");
        }
    } else {
        if (shape_.size() == 1 || shape_.size() == 2) {
            return transpose(std::vector<size_t>{1, 0});
        } else {
            throw std::invalid_argument(
                "You must provide an index permutation for tensors with more than 2 dimensions");
        }
    }
}

template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::transpose() const {
    if constexpr (fixed_shape<Shape>) {
        if constexpr (Shape::size() == 1 || Shape::size() == 2) {
            return this->transpose<std::index_sequence<1, 0>>();
        } else {
            throw std::invalid_argument(
                "You must provide an index permutation for tensors with more than 2 dimensions");
        }
    } else {
        if (shape_.size() == 1 || shape_.size() == 2) {
            return transpose(std::vector<size_t>{1, 0});
        } else {
            throw std::invalid_argument(
                "You must provide an index permutation for tensors with more than 2 dimensions");
        }
    }
}

} // namespace squint

#endif // SQUINT_TENSOR_TENSOR_SHAPE_MANIPULATIONS_HPP