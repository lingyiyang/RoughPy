#ifndef ROUGHPY_ALGEBRA_ALGEBRA_FWD_H_
#define ROUGHPY_ALGEBRA_ALGEBRA_FWD_H_

#include "roughpy_algebra_export.h"
#include <roughpy/core/implementation_types.h>



namespace rpy {
namespace algebra {


enum class ImplementationType {
    Owned,
    Borrowed
};

enum class VectorType {
    Dense,
    Sparse
};

/**
 * @brief Different algebra types required by RoughPy
 */
enum class AlgebraType : uint32_t {
    FreeTensor,
    ShuffleTensor,
    Lie
};


template <typename Interface>
struct algebra_access;


template <typename Wrapper, typename Algebra>
struct algebra_info;

template <typename Algebra>
class AlgebraIterator;

template <typename PrimaryInterface>
class Basis;


class FreeTensor;
class Lie;
class ShuffleTensor;


}
}

#endif // ROUGHPY_ALGEBRA_ALGEBRA_FWD_H_
