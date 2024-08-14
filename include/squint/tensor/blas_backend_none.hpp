// NOLINTBEGIN
#ifndef SQUINT_TENSOR_BLAS_BACKEND_NONE_HPP
#define SQUINT_TENSOR_BLAS_BACKEND_NONE_HPP

#include <cmath>
#include <stdexcept>
#include <vector>

namespace squint {
// BLAS and LAPACK constants
constexpr int LAPACK_ROW_MAJOR = 101;
constexpr int LAPACK_COL_MAJOR = 102;

enum class CBLAS_ORDER { CblasRowMajor = 101, CblasColMajor = 102 };
enum class CBLAS_TRANSPOSE { CblasNoTrans = 111, CblasTrans = 112, CblasConjTrans = 113 };

// Helper functions
template <typename T> void swap_row(T *matrix, int row1, int row2, int n, int lda) {
    for (int j = 0; j < n; ++j) {
        std::swap(matrix[row1 * lda + j], matrix[row2 * lda + j]);
    }
}

template <typename T> auto matrix_element(T *matrix, int i, int j, int lda, int matrix_layout) -> T & {
    return (matrix_layout == LAPACK_ROW_MAJOR) ? matrix[i * lda + j] : matrix[j * lda + i];
}

// Main LAPACK fallback implementations
template <typename T>
void gemm(CBLAS_ORDER order, CBLAS_TRANSPOSE trans_a, CBLAS_TRANSPOSE trans_b, int m, int n, int k, T alpha, const T *a,
          int lda, const T *b, int ldb, T beta, T *c, int ldc) {
    const bool row_major = (order == CBLAS_ORDER::CblasRowMajor);
    const bool trans_a_bool = (trans_a != CBLAS_TRANSPOSE::CblasNoTrans);
    const bool trans_b_bool = (trans_b != CBLAS_TRANSPOSE::CblasNoTrans);

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            T sum = 0;
            for (int l = 0; l < k; ++l) {
                const int a_idx =
                    trans_a_bool ? (row_major ? l * lda + i : i * lda + l) : (row_major ? i * lda + l : l * lda + i);
                const int b_idx =
                    trans_b_bool ? (row_major ? j * ldb + l : l * ldb + j) : (row_major ? l * ldb + j : j * ldb + l);
                sum += a[a_idx] * b[b_idx];
            }
            const int c_idx = row_major ? i * ldc + j : j * ldc + i;
            c[c_idx] = alpha * sum + beta * c[c_idx];
        }
    }
}

template <typename T> auto getrf(int matrix_layout, int m, int n, T *a, int lda, int *ipiv) -> int {
    if (matrix_layout != LAPACK_ROW_MAJOR && matrix_layout != LAPACK_COL_MAJOR) {
        throw std::invalid_argument("Invalid matrix layout");
    }

    const int min_mn = std::min(m, n);

    for (int i = 0; i < min_mn; ++i) {
        // Find pivot
        int pivot = i;
        T max_val = std::abs(matrix_element(a, i, i, lda, matrix_layout));

        for (int j = i + 1; j < m; ++j) {
            T val = std::abs(matrix_element(a, j, i, lda, matrix_layout));
            if (val > max_val) {
                max_val = val;
                pivot = j;
            }
        }

        ipiv[i] = pivot + 1; // LAPACK uses 1-based indexing for ipiv

        // Swap rows if necessary
        if (pivot != i) {
            if (matrix_layout == LAPACK_ROW_MAJOR) {
                swap_row(a, i, pivot, n, lda);
            } else {
                for (int k = 0; k < m; ++k) {
                    std::swap(a[k * lda + i], a[k * lda + pivot]);
                }
            }
        }

        // Gaussian elimination
        if (matrix_element(a, i, i, lda, matrix_layout) != T(0)) {
            for (int j = i + 1; j < m; ++j) {
                T factor = matrix_element(a, j, i, lda, matrix_layout) / matrix_element(a, i, i, lda, matrix_layout);
                matrix_element(a, j, i, lda, matrix_layout) = factor;

                for (int k = i + 1; k < n; ++k) {
                    matrix_element(a, j, k, lda, matrix_layout) -= factor * matrix_element(a, i, k, lda, matrix_layout);
                }
            }
        } else if (i == min_mn - 1) {
            return i + 1; // Matrix is singular
        }
    }

    return 0; // Success
}

template <typename T> int getri(int matrix_layout, int n, T *a, int lda, const int *ipiv) {
    if (matrix_layout != LAPACK_ROW_MAJOR && matrix_layout != LAPACK_COL_MAJOR) {
        throw std::invalid_argument("Invalid matrix layout");
    }

    // Create identity matrix
    std::vector<T> work(n * n, T(0));
    for (int i = 0; i < n; ++i) {
        work[i * n + i] = T(1);
    }

    // Apply permutations to identity matrix
    for (int i = n - 1; i >= 0; --i) {
        int pivot = ipiv[i] - 1; // Convert back to 0-based indexing
        if (pivot != i) {
            swap_row(work.data(), i, pivot, n, n);
        }
    }

    // Solve the system LY = P
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                work[j * n + k] -= matrix_element(a, j, i, lda, matrix_layout) * work[i * n + k];
            }
        }
    }

    // Solve the system UX = Y
    for (int i = n - 1; i >= 0; --i) {
        for (int j = 0; j < n; ++j) {
            work[i * n + j] /= matrix_element(a, i, i, lda, matrix_layout);
        }

        for (int j = 0; j < i; ++j) {
            for (int k = 0; k < n; ++k) {
                work[j * n + k] -= matrix_element(a, j, i, lda, matrix_layout) * work[i * n + k];
            }
        }
    }

    // Copy result back to a
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix_element(a, i, j, lda, matrix_layout) = work[i * n + j];
        }
    }

    return 0; // Success
}

template <typename T> auto gesv(int matrix_layout, int n, int nrhs, T *a, int lda, int *ipiv, T *b, int ldb) -> int {
    // Perform LU decomposition
    int info = getrf(matrix_layout, n, n, a, lda, ipiv);
    if (info != 0) {
        return info;
    }

    const bool row_major = (matrix_layout == LAPACK_ROW_MAJOR);

    // Solve the system using forward and backward substitution
    for (int k = 0; k < nrhs; ++k) {
        // Forward substitution
        for (int i = 0; i < n; ++i) {
            int pivot = ipiv[i] - 1; // LAPACK uses 1-based indexing for ipiv
            if (pivot != i) {
                int idx_i = row_major ? i * ldb + k : k * ldb + i;
                int idx_pivot = row_major ? pivot * ldb + k : k * ldb + pivot;
                std::swap(b[idx_i], b[idx_pivot]);
            }
            for (int j = i + 1; j < n; ++j) {
                int idx_j = row_major ? j * ldb + k : k * ldb + j;
                int idx_i = row_major ? i * ldb + k : k * ldb + i;
                int idx_ji = row_major ? j * lda + i : i * lda + j;
                b[idx_j] -= a[idx_ji] * b[idx_i];
            }
        }

        // Backward substitution
        for (int i = n - 1; i >= 0; --i) {
            int idx_i = row_major ? i * ldb + k : k * ldb + i;
            T sum = b[idx_i];
            for (int j = i + 1; j < n; ++j) {
                int idx_j = row_major ? j * ldb + k : k * ldb + j;
                int idx_ij = row_major ? i * lda + j : j * lda + i;
                sum -= a[idx_ij] * b[idx_j];
            }
            int idx_ii = row_major ? i * lda + i : i * lda + i;
            b[idx_i] = sum / a[idx_ii];
        }
    }

    return 0;
}

template <typename T>
auto gels(int matrix_layout, char trans, int m, int n, int nrhs, T *a, int lda, T *b, int ldb) -> int {
    bool is_row_major = (matrix_layout == LAPACK_ROW_MAJOR);
    bool is_transposed = (trans == 'T' || trans == 't');

    if (is_transposed) {
        std::swap(m, n);
    }

    int min_mn = std::min(m, n);
    int max_mn = std::max(m, n);

    // Create copies of A and B to work with
    std::vector<T> a_copy(m * n);
    std::vector<T> b_copy(max_mn * nrhs, T(0));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            if (is_transposed) {
                a_copy[j * m + i] = is_row_major ? a[i * lda + j] : a[j * lda + i];
            } else {
                a_copy[i * n + j] = is_row_major ? a[i * lda + j] : a[j * lda + i];
            }
        }
        for (int j = 0; j < nrhs; ++j) {
            b_copy[i * nrhs + j] = is_row_major ? b[i * ldb + j] : b[j * ldb + i];
        }
    }

    if (m >= n) {
        // Overdetermined or square system
        // Compute A^T * A and A^T * b
        std::vector<T> ata(n * n, T(0));
        std::vector<T> atb(n * nrhs, T(0));

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int k = 0; k < m; ++k) {
                    ata[i * n + j] += a_copy[k * n + i] * a_copy[k * n + j];
                }
            }
            for (int j = 0; j < nrhs; ++j) {
                for (int k = 0; k < m; ++k) {
                    atb[i * nrhs + j] += a_copy[k * n + i] * b_copy[k * nrhs + j];
                }
            }
        }

        // Solve (A^T * A) * x = A^T * b using Gaussian elimination with partial pivoting
        for (int k = 0; k < n; ++k) {
            // Find pivot
            int pivot = k;
            T max_val = std::abs(ata[k * n + k]);
            for (int i = k + 1; i < n; ++i) {
                if (std::abs(ata[i * n + k]) > max_val) {
                    max_val = std::abs(ata[i * n + k]);
                    pivot = i;
                }
            }

            // Swap rows if necessary
            if (pivot != k) {
                for (int j = k; j < n; ++j) {
                    std::swap(ata[k * n + j], ata[pivot * n + j]);
                }
                for (int j = 0; j < nrhs; ++j) {
                    std::swap(atb[k * nrhs + j], atb[pivot * nrhs + j]);
                }
            }

            // Gaussian elimination
            for (int i = k + 1; i < n; ++i) {
                T factor = ata[i * n + k] / ata[k * n + k];
                for (int j = k + 1; j < n; ++j) {
                    ata[i * n + j] -= factor * ata[k * n + j];
                }
                for (int j = 0; j < nrhs; ++j) {
                    atb[i * nrhs + j] -= factor * atb[k * nrhs + j];
                }
                ata[i * n + k] = 0;
            }
        }

        // Back-substitution
        for (int j = 0; j < nrhs; ++j) {
            for (int i = n - 1; i >= 0; --i) {
                T sum = atb[i * nrhs + j];
                for (int k = i + 1; k < n; ++k) {
                    sum -= ata[i * n + k] * b_copy[k * nrhs + j];
                }
                b_copy[i * nrhs + j] = sum / ata[i * n + i];
            }
        }
    } else {
        // Underdetermined system
        // Compute A * A^T
        std::vector<T> aat(m * m, T(0));
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < m; ++j) {
                for (int k = 0; k < n; ++k) {
                    aat[i * m + j] += a_copy[i * n + k] * a_copy[j * n + k];
                }
            }
        }

        // Solve (A * A^T) * y = b using Gaussian elimination with partial pivoting
        std::vector<T> y = b_copy;
        for (int k = 0; k < m; ++k) {
            // Find pivot
            int pivot = k;
            T max_val = std::abs(aat[k * m + k]);
            for (int i = k + 1; i < m; ++i) {
                if (std::abs(aat[i * m + k]) > max_val) {
                    max_val = std::abs(aat[i * m + k]);
                    pivot = i;
                }
            }

            // Swap rows if necessary
            if (pivot != k) {
                for (int j = k; j < m; ++j) {
                    std::swap(aat[k * m + j], aat[pivot * m + j]);
                }
                for (int j = 0; j < nrhs; ++j) {
                    std::swap(y[k * nrhs + j], y[pivot * nrhs + j]);
                }
            }

            // Gaussian elimination
            for (int i = k + 1; i < m; ++i) {
                T factor = aat[i * m + k] / aat[k * m + k];
                for (int j = k + 1; j < m; ++j) {
                    aat[i * m + j] -= factor * aat[k * m + j];
                }
                for (int j = 0; j < nrhs; ++j) {
                    y[i * nrhs + j] -= factor * y[k * nrhs + j];
                }
                aat[i * m + k] = 0;
            }
        }

        // Back-substitution
        for (int j = 0; j < nrhs; ++j) {
            for (int i = m - 1; i >= 0; --i) {
                T sum = y[i * nrhs + j];
                for (int k = i + 1; k < m; ++k) {
                    sum -= aat[i * m + k] * y[k * nrhs + j];
                }
                y[i * nrhs + j] = sum / aat[i * m + i];
            }
        }

        // Compute x = A^T * y
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < nrhs; ++j) {
                b_copy[i * nrhs + j] = 0;
                for (int k = 0; k < m; ++k) {
                    b_copy[i * nrhs + j] += a_copy[k * n + i] * y[k * nrhs + j];
                }
            }
        }
    }

    // Copy solution back to b
    for (int i = 0; i < max_mn; ++i) {
        for (int j = 0; j < nrhs; ++j) {
            if (is_row_major) {
                b[i * ldb + j] = b_copy[i * nrhs + j];
            } else {
                b[j * ldb + i] = b_copy[i * nrhs + j];
            }
        }
    }

    return 0;
}
} // namespace squint

#endif // SQUINT_TENSOR_BLAS_BACKEND_NONE_HPP
       // NOLINTEND