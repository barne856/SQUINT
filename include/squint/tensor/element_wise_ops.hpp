/**
 * @file element_wise_ops.hpp
 * @brief Element-wise operations for tensor objects.
 *
 * This file contains implementations of element-wise operations on tensors,
 * including addition, subtraction, equality comparison, and negation.
 */
#ifndef SQUINT_TENSOR_ELEMENT_WISE_OPS_HPP
#define SQUINT_TENSOR_ELEMENT_WISE_OPS_HPP

#include "squint/core/concepts.hpp"
#include "squint/core/error_checking.hpp"
#include "squint/core/memory.hpp"
#include "squint/tensor/tensor.hpp"
#include "squint/tensor/tensor_op_compatibility.hpp"

#include <cstddef>
#include <functional>
#include <type_traits>
#include <vector>

#ifdef SQUINT_USE_CUDA
#include "squint/tensor/cuda/element_wise.hpp"
#include <cuda_runtime.h>
#endif

namespace squint {

// Element-wise addition assignment
/**
 * @brief Element-wise addition assignment operator.
 * @param other The tensor to add to this tensor.
 * @return Reference to the modified tensor.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <typename U, typename OtherShape, typename OtherStrides, enum error_checking OtherErrorChecking,
          enum ownership_type OtherOwnershipType>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::operator+=(
    const tensor<U, OtherShape, OtherStrides, OtherErrorChecking, OtherOwnershipType, MemorySpace> &other) -> tensor & {
    element_wise_compatible(*this, other);
    if constexpr (MemorySpace == memory_space::host) {
        std::transform(begin(), end(), other.begin(), begin(), std::plus{});
    } else {
#ifdef SQUINT_USE_CUDA
        // NOLINTBEGIN
        using blas_type = std::common_type_t<blas_type_t<T>, blas_type_t<U>>;
        if constexpr (std::is_same_v<blas_type, float>) {
            element_wise_addition(reinterpret_cast<float *>(data()), reinterpret_cast<const float *>(other.data()),
                                  reinterpret_cast<const float *>(data()), device_shape(), device_strides(),
                                  other.device_strides(), device_strides(), shape().size(), size());
        } else if constexpr (std::is_same_v<blas_type, double>) {
            element_wise_addition(reinterpret_cast<double *>(data()), reinterpret_cast<const double *>(other.data()),
                                  reinterpret_cast<const double *>(data()), device_shape(), device_strides(),
                                  other.device_strides(), device_strides(), shape().size(), size());
        }
        // NOLINTEND
#endif
    }
    return *this;
}

// Element-wise subtraction assignment
/**
 * @brief Element-wise subtraction assignment operator.
 * @param other The tensor to subtract from this tensor.
 * @return Reference to the modified tensor.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <typename U, typename OtherShape, typename OtherStrides, enum error_checking OtherErrorChecking,
          enum ownership_type OtherOwnershipType>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::operator-=(
    const tensor<U, OtherShape, OtherStrides, OtherErrorChecking, OtherOwnershipType, MemorySpace> &other) -> tensor & {
    element_wise_compatible(*this, other);
    std::transform(begin(), end(), other.begin(), begin(), std::minus{});
    return *this;
}

// Element-wise equality comparison
/**
 * @brief Element-wise equality comparison operator.
 * @param other The tensor to compare with this tensor.
 * @return True if all elements are equal, false otherwise.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <typename U, typename OtherShape, typename OtherStrides, enum error_checking OtherErrorChecking,
          enum ownership_type OtherOwnershipType>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::operator==(
    const tensor<U, OtherShape, OtherStrides, OtherErrorChecking, OtherOwnershipType, MemorySpace> &other) const
    -> bool {
    element_wise_compatible(*this, other);
    return std::equal(begin(), end(), other.begin());
}

// Element-wise inequality comparison
/**
 * @brief Element-wise inequality comparison operator.
 * @param other The tensor to compare with this tensor.
 * @return True if any elements are not equal, false otherwise.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
template <typename U, typename OtherShape, typename OtherStrides, enum error_checking OtherErrorChecking,
          enum ownership_type OtherOwnershipType>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::operator!=(
    const tensor<U, OtherShape, OtherStrides, OtherErrorChecking, OtherOwnershipType, MemorySpace> &other) const
    -> bool {
    element_wise_compatible(*this, other);
    return !std::equal(begin(), end(), other.begin());
}

// Unary negation
/**
 * @brief Unary negation operator.
 * @return A new tensor with all elements negated.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace>
auto tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace>::operator-() const -> tensor {
    tensor result(*this);
    std::transform(result.begin(), result.end(), result.begin(), std::negate{});
    return result;
}

// Element-wise addition
/**
 * @brief Element-wise addition operator.
 * @param lhs The left-hand side tensor.
 * @param rhs The right-hand side tensor.
 * @return A new tensor containing the element-wise sum.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace, typename U, typename OtherShape, typename OtherStrides,
          enum error_checking OtherErrorChecking, enum ownership_type OtherOwnershipType>
auto operator+(const tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace> &lhs,
               const tensor<U, OtherShape, OtherStrides, OtherErrorChecking, OtherOwnershipType, MemorySpace> &rhs)
    -> tensor<decltype(std::declval<T>() + std::declval<U>()),
              std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Shape, std::vector<std::size_t>>,
              std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Strides, std::vector<std::size_t>>,
              resulting_error_checking<ErrorChecking, OtherErrorChecking>::value, ownership_type::owner, MemorySpace> {
    element_wise_compatible(lhs, rhs);
    tensor<decltype(std::declval<T>() + std::declval<U>()),
           std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Shape, std::vector<std::size_t>>,
           std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Strides, std::vector<std::size_t>>,
           resulting_error_checking<ErrorChecking, OtherErrorChecking>::value, ownership_type::owner, MemorySpace>
        result(lhs);
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), result.begin(), std::plus{});
    return result;
}

// Element-wise subtraction
/**
 * @brief Element-wise subtraction operator.
 * @param lhs The left-hand side tensor.
 * @param rhs The right-hand side tensor.
 * @return A new tensor containing the element-wise difference.
 */
template <typename T, typename Shape, typename Strides, error_checking ErrorChecking, ownership_type OwnershipType,
          memory_space MemorySpace, typename U, typename OtherShape, typename OtherStrides,
          enum error_checking OtherErrorChecking, enum ownership_type OtherOwnershipType>
auto operator-(const tensor<T, Shape, Strides, ErrorChecking, OwnershipType, MemorySpace> &lhs,
               const tensor<U, OtherShape, OtherStrides, OtherErrorChecking, OtherOwnershipType, MemorySpace> &rhs)
    -> tensor<decltype(std::declval<T>() - std::declval<U>()),
              std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Shape, std::vector<std::size_t>>,
              std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Strides, std::vector<std::size_t>>,
              resulting_error_checking<ErrorChecking, OtherErrorChecking>::value, ownership_type::owner, MemorySpace> {
    element_wise_compatible(lhs, rhs);
    tensor<decltype(std::declval<T>() - std::declval<U>()),
           std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Shape, std::vector<std::size_t>>,
           std::conditional_t<fixed_shape<Shape> && fixed_shape<OtherShape>, Strides, std::vector<std::size_t>>,
           resulting_error_checking<ErrorChecking, OtherErrorChecking>::value, ownership_type::owner, MemorySpace>
        result(lhs);
    std::transform(lhs.begin(), lhs.end(), rhs.begin(), result.begin(), std::minus{});
    return result;
}

} // namespace squint

#endif // SQUINT_TENSOR_ELEMENT_WISE_OPS_HPP