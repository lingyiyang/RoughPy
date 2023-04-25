// Copyright (c) 2023 RoughPy Developers. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be used to endorse or promote products derived from this software without
// specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//
// Created by sam on 13/03/23.
//

#include "RationalType.h"

#include <algorithm>
#include <ostream>
#include <utility>

#include "float_type_import.h"

using namespace rpy;
using namespace rpy::scalars;

std::unique_ptr<RandomGenerator> RationalType::get_mt19937_generator(const ScalarType* type, Slice<uint64_t> seed) {
    return nullptr;
}
std::unique_ptr<RandomGenerator> RationalType::get_pcg_generator(const ScalarType* type, Slice<uint64_t> seed) {
    return std::unique_ptr<RandomGenerator>();
}

RationalType::RationalType()
    : ScalarType({}) {}
ScalarPointer RationalType::allocate(std::size_t count) const {
    if (count == 1) {
        return ScalarPointer(this, new rational_scalar_type, flags::IsMutable | flags::OwnedPointer);
    } else {
        return ScalarPointer(this, new rational_scalar_type[count], flags::IsMutable | flags::OwnedPointer);
    }
}
void RationalType::free(ScalarPointer pointer, std::size_t count) const {
    if (!pointer.is_null()) {
        if (count == 1) {
            delete pointer.template raw_cast<rational_scalar_type>();
        } else {
            delete[] pointer.template raw_cast<rational_scalar_type>();
        }
    }
}

RationalType::scalar_type RationalType::try_convert(ScalarPointer other) const {
    if (other.is_null()) {
        return scalar_type(0);
    }
    if (other.type() == this) {
        return *other.template raw_cast<const scalar_type>();
    }

    const ScalarType *type = other.type();
    if (type == nullptr) {
        throw std::runtime_error("null type for non-zero value");
    }

    auto cv = get_conversion(type->id(), this->id());
    if (cv) {
        scalar_type result;
        ScalarPointer result_ptr{this, &result};
        cv(result_ptr, other, 1);
        return result;
    }

    throw std::runtime_error("could not convert " + type->info().name + " to scalar type " + info().name);
}

void RationalType::convert_copy(ScalarPointer dst, ScalarPointer src, dimn_t count) const {
    if (src.type() == nullptr) {
        throw std::invalid_argument("source type cannot be null");
    }
    convert_copy(dst, src.ptr(), count, src.type()->id());
}


template<typename F>
static inline void convert_copy_ext(ScalarPointer& out, const void* in, std::size_t count) {
    const auto *iptr = static_cast<const F *>(in);
    auto *optr = static_cast<rational_scalar_type *>(out.ptr());

    for (dimn_t i = 0; i < count; ++i, ++iptr, ++optr) {
        ::new (optr) rational_scalar_type(static_cast<float>(*iptr));
    }
}


void RationalType::convert_copy(void *out, const void *in, std::size_t count, BasicScalarInfo info) const {

    ScalarPointer optr(this, out);
    switch (info.code) {
        case ScalarTypeCode::Int:
            switch (info.bits) {
                case 8:
                    convert_copy_basic<int8_t>(optr, in, info.lanes * count);
                    break;
                case 16:
                    convert_copy_basic<int16_t>(optr, in, info.lanes * count);
                    break;
                case 32:
                    convert_copy_basic<int32_t>(optr, in, info.lanes * count);
                    break;
                case 64:
                    convert_copy_basic<int64_t>(optr, in, info.lanes * count);
                    break;
                case 128:
                default:
                    throw std::runtime_error("invalid bit configuration for integer type");
            }
            break;
        case ScalarTypeCode::UInt:
            switch (info.bits) {
                case 8:
                    convert_copy_basic<uint8_t>(optr, in, info.lanes * count);
                    break;
                case 16:
                    convert_copy_basic<uint16_t>(optr, in, info.lanes * count);
                    break;
                case 32:
                    convert_copy_basic<uint32_t>(optr, in, info.lanes * count);
                    break;
                case 64:
                    convert_copy_basic<uint64_t>(optr, in, info.lanes * count);
                    break;
                case 128:
                default:
                    throw std::runtime_error("invalid bit configuration for integer type");
            }
            break;
        case ScalarTypeCode::Float:
            switch (info.bits) {
                case 16:
                    convert_copy_ext<half>(optr, in, info.lanes * count);
                    break;
                case 32:
                    convert_copy_basic<float>(optr, in, info.lanes * count);
                    break;
                case 64:
                    convert_copy_basic<double>(optr, in, info.lanes * count);
                    break;
                default:
                    throw std::runtime_error("invalid bit configuration for float type");
            }
            break;
        case ScalarTypeCode::BFloat:
            switch (info.bits) {
                case 16:
                    convert_copy_ext<bfloat16>(optr, in, info.lanes * count);
                    break;
                default:
                    throw std::runtime_error("invalid bit configuration for bfloat type");
            }
            break;
        case ScalarTypeCode::Bool:
        case ScalarTypeCode::OpaqueHandle: break;
        case ScalarTypeCode::Complex:
        default:
            throw std::runtime_error("unsupported scalar type");
    }



}
void RationalType::convert_copy(void *out, ScalarPointer in, std::size_t count) const {
    assert(out != nullptr);
    assert(!in.is_null());
    const auto *type = in.type();

    if (type == nullptr) {
        throw std::runtime_error("null type for non-zero value");
    }

    if (type == this) {
        const auto *in_begin = in.template raw_cast<const scalar_type>();
        const auto *in_end = in_begin + count;
        std::copy(in_begin, in_end, static_cast<scalar_type *>(out));
    } else {
        auto cv = get_conversion(type->id(), this->id());
        ScalarPointer out_ptr{this, out};

        cv(out_ptr, in, count);
    }
}

void RationalType::convert_copy(ScalarPointer out, const void *in, std::size_t count, const string &type_id) const {
    if (type_id == "f64") {
        return convert_copy_basic<double>(out, in, count);
    } else if (type_id == "f32") {
        return convert_copy_basic<float>(out, in, count);
    } else if (type_id == "i32") {
        return convert_copy_basic<int>(out, in, count);
    } else if (type_id == "u32") {
        return convert_copy_basic<unsigned int>(out, in, count);
    } else if (type_id == "i64") {
        return convert_copy_basic<long long>(out, in, count);
    } else if (type_id == "u64") {
        return convert_copy_basic<unsigned long long>(out, in, count);
    } else if (type_id == "isize") {
        return convert_copy_basic<std::ptrdiff_t>(out, in, count);
    } else if (type_id == "usize") {
        return convert_copy_basic<std::size_t>(out, in, count);
    } else if (type_id == "i16") {
        return convert_copy_basic<short>(out, in, count);
    } else if (type_id == "u16") {
        return convert_copy_basic<unsigned short>(out, in, count);
    } else if (type_id == "i8") {
        return convert_copy_basic<char>(out, in, count);
    } else if (type_id == "u8") {
        return convert_copy_basic<unsigned char>(out, in, count);
    }

    // If we're here, then it is a non-standard type
    const auto &conversion = get_conversion(type_id, this->id());
    conversion(out, {nullptr, in}, count);
}
scalar_t RationalType::to_scalar_t(ScalarPointer arg) const {
    return static_cast<scalar_t>(*arg.raw_cast<const scalar_type *>());
}
void RationalType::assign(ScalarPointer target, long long int numerator, long long int denominator) const {
    *target.raw_cast<scalar_type *>() = scalar_type(numerator) / denominator;
}
Scalar RationalType::uminus(ScalarPointer arg) const {
    return Scalar(this, -try_convert(arg));
}
void RationalType::add_inplace(ScalarPointer lhs, ScalarPointer rhs) const {
    assert(lhs);
    auto *ptr = lhs.raw_cast<scalar_type *>();
    *ptr += try_convert(rhs);
}
void RationalType::sub_inplace(ScalarPointer lhs, ScalarPointer rhs) const {
    assert(lhs);
    auto *ptr = lhs.raw_cast<scalar_type *>();
    *ptr -= try_convert(rhs);
}
void RationalType::mul_inplace(ScalarPointer lhs, ScalarPointer rhs) const {
    assert(lhs);
    auto *ptr = lhs.raw_cast<scalar_type *>();
    *ptr *= try_convert(rhs);
}
void RationalType::div_inplace(ScalarPointer lhs, ScalarPointer rhs) const {
    assert(lhs);
    auto *ptr = lhs.raw_cast<scalar_type *>();
    if (rhs.is_null()) {
        throw std::runtime_error("division by zero");
    }

    auto crhs = try_convert(rhs);

    if (crhs == scalar_type(0)) {
        throw std::runtime_error("division by zero");
    }

    *ptr /= crhs;
}
bool RationalType::are_equal(ScalarPointer lhs, ScalarPointer rhs) const noexcept {
    return *lhs.raw_cast<const scalar_type *>() == try_convert(rhs);
}
Scalar RationalType::from(long long int numerator, long long int denominator) const {
    return Scalar(this, scalar_type(numerator) / denominator);
}
void RationalType::convert_fill(ScalarPointer out, ScalarPointer in, dimn_t count, const string &id) const {
    ScalarType::convert_fill(out, in, count, id);
}
Scalar RationalType::one() const {
    return Scalar(this, scalar_type(1));
}
Scalar RationalType::mone() const {
    return Scalar(this, scalar_type(-1));
}
Scalar RationalType::zero() const {
    return Scalar(this, scalar_type(0));
}
Scalar RationalType::copy(ScalarPointer arg) const {
    return Scalar(this, try_convert(arg));
}
Scalar RationalType::add(ScalarPointer lhs, ScalarPointer rhs) const {
    if (!lhs) {
        return copy(rhs);
    }
    return Scalar(this, *lhs.raw_cast<const scalar_type *>() + try_convert(rhs));
}
Scalar RationalType::sub(ScalarPointer lhs, ScalarPointer rhs) const {
    if (!lhs) {
        return uminus(rhs);
    }
    return Scalar(this, *lhs.raw_cast<const scalar_type *>() - try_convert(rhs));
}
Scalar RationalType::mul(ScalarPointer lhs, ScalarPointer rhs) const {
    if (!lhs) {
        return zero();
    }
    return Scalar(this, *lhs.raw_cast<const scalar_type *>() * try_convert(rhs));
}
Scalar RationalType::div(ScalarPointer lhs, ScalarPointer rhs) const {
    if (!lhs) {
        return zero();
    }
    if (rhs.is_null()) {
        throw std::runtime_error("division by zero");
    }

    auto crhs = try_convert(rhs);

    if (crhs == scalar_type(0)) {
        throw std::runtime_error("division by zero");
    }

    return Scalar(this, static_cast<scalar_type>(*lhs.raw_cast<const scalar_type *>() / crhs));
}
bool RationalType::is_zero(ScalarPointer arg) const {
    return !static_cast<bool>(arg) || *arg.raw_cast<const scalar_type *>() == scalar_type(0);
}
void RationalType::print(ScalarPointer arg, std::ostream &os) const {
    if (!arg) {
        os << 0.0;
    } else {
        os << *arg.raw_cast<const scalar_type *>();
    }
}
std::unique_ptr<RandomGenerator> RationalType::get_rng(const string &bit_generator, Slice<uint64_t> seed) const {
    ScalarType::get_rng(bit_generator, seed);
    RPY_UNREACHABLE();
}
void RationalType::swap(ScalarPointer lhs, ScalarPointer rhs) const {

    if (lhs.is_null() ^ rhs.is_null()) {
        throw std::runtime_error("one of the pointers is null");
    }

    if (lhs.type() != rhs.type()) {
        throw std::runtime_error("cannot swap scalars of different types");
    }

    if (lhs.type() != this && lhs.type() != nullptr) {
        return lhs.type()->swap(lhs, rhs);
    }

    if (lhs.is_const() || rhs.is_const()) {
        throw std::runtime_error("one or both of the scalars is const");
    }

    std::swap(
        *lhs.raw_cast<scalar_type *>(),
        *rhs.raw_cast<scalar_type *>());
}
