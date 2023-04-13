// Copyright (c) 2023 Datasig Group. All rights reserved.
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

#ifndef ROUGHPY_CORE_POINTER_HELPERS_H_
#define ROUGHPY_CORE_POINTER_HELPERS_H_

#include "implementation_types.h"
#include "traits.h"
#include "macros.h"

#include <cstring>
#include <iterator>
#include <vector>

namespace rpy {


/**
 * @brief Cast the bit value from a value of type From to a value
 * of type To.
 *
 * We're using the same conditions on From and To as the Abseil library
 * if we have to define our own version using memcpy.
 */
#if defined(__cpp_lib_bit_cast) && __cpp_Lib_bit_cast >= 201806L
using std::bit_cast;
#else
template <typename To, typename From>
traits::enable_if_t<
    sizeof(To) == sizeof(From)
    && traits::is_trivially_copyable<From>::value
    && traits::is_trivially_copyable<To>::value
    && traits::is_default_constructible<To>::value
    , To>
bit_cast(From from) {
    To to;
    memcpy(static_cast<void*>(std::addressof(to)),
           static_cast<const void*>(std::addressof(from)),
           sizeof(To));
    return to;
}
#endif



/**
 * @brief
 * @tparam T
 */
template <typename T>
class MaybeOwned {
    enum State {
        IsOwned,
        IsBorrowed
    };

    T* p_data;
    State m_state;

public:

    constexpr MaybeOwned(nullptr_t) : p_data(nullptr), m_state(IsOwned) {}
    constexpr MaybeOwned(T* ptr) : p_data(ptr), m_state(IsBorrowed) {}

    ~MaybeOwned() {
        if (m_state == IsOwned) {
            delete[] p_data;
        }
    }

    constexpr MaybeOwned& operator=(T* ptr) {
        p_data = ptr;
        m_state = IsOwned;
        return *this;
    }

    operator T* () const noexcept { return p_data; }

    operator bool() const noexcept { return p_data != nullptr; }

};









}

#endif // ROUGHPY_CORE_POINTER_HELPERS_H_