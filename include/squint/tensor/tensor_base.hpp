#ifndef SQUINT_TENSOR_TENSOR_BASE_HPP
#define SQUINT_TENSOR_TENSOR_BASE_HPP

#include "squint/core/concepts.hpp"
#include "squint/core/error_checking.hpp"
#include "squint/core/layout.hpp"
#include "squint/core/types.hpp"
#include "squint/util/array_utils.hpp"

#include <array>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace squint {

template <typename Derived, typename T, typename Shape, layout Layout, error_checking ErrorChecking> class tensor_base {
  public:
    using value_type = T;
    using shape_type = Shape;

    static constexpr layout layout() { return Layout; }
    static constexpr error_checking error_checking() { return ErrorChecking; }

    constexpr std::size_t rank() const {
        if constexpr (fixed_shape<Derived>) {
            return Shape::size();
        } else {
            return static_cast<const Derived *>(this)->shape().size();
        }
    }

    constexpr std::size_t size() const {
        if constexpr (fixed_shape<Derived>) {
            return product(Shape{});
        } else {
            return static_cast<const Derived *>(this)->shape().size();
        }
    }

    auto shape() const {
        if constexpr (fixed_shape<Derived>) {
            constexpr auto shape_arr = make_array(Shape{});
            return std::vector<std::size_t>(shape_arr.begin(), shape_arr.end());
        } else {
            return static_cast<Derived *>(this)->shape();
        }
    }

    const T *data() const { return static_cast<const Derived *>(this)->data(); }
    T *data() { return static_cast<Derived *>(this)->data(); }

  protected:
    void check_subscript_bounds(const std::vector<std::size_t> &indices) const {
        if (indices.size() != rank()) {
            throw std::invalid_argument("Incorrect number of indices");
        }
        const auto s = shape();
        for (std::size_t i = 0; i < rank(); ++i) {
            if (indices[i] >= s[i]) {
                throw std::out_of_range("Index out of bounds");
            }
        }
    }
    void check_subview_bounds(const std::vector<std::size_t> &shape, const std::vector<std::size_t> &start) const {
        if (shape.size() != start.size() || shape.size() != this->shape().size()) {
            throw std::out_of_range("Invalid number of sizes or offsets");
        }
        for (size_t i = 0; i < shape.size(); ++i) {
            if (start[i] + shape[i] > this->shape()[i]) {
                throw std::out_of_range("Subview out of bounds");
            }
        }
    }
};

} // namespace squint

#endif // SQUINT_TENSOR_TENSOR_BASE_HPP