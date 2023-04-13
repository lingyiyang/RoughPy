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

#ifndef ROUGHPY_SCALARS_SCALAR_MATRIX_H_
#define ROUGHPY_SCALARS_SCALAR_MATRIX_H_

#include "scalars_fwd.h"
#include "scalar_array.h"

namespace rpy { namespace scalars {

enum class MatrixStorage {
    FullMatrix,
    UpperTriangular,
    LowerTriangular,
    Diagonal
};

enum class MatrixLayout {
    CStype,
    FStype
};


class ROUGHPY_SCALARS_EXPORT ScalarMatrix
    : public scalars::ScalarArray {

    MatrixStorage m_storage;
    MatrixLayout m_layout;
    deg_t m_nrows;
    deg_t m_ncols;


public:

    ScalarMatrix(deg_t rows,
                 deg_t cols,
                 ScalarArray&& array,
                 MatrixStorage storage=MatrixStorage::FullMatrix,
                 MatrixLayout layout=MatrixLayout::CStype);

    ScalarMatrix row(deg_t i);
    ScalarMatrix row(deg_t i) const;
    ScalarMatrix col(deg_t i);
    ScalarMatrix col(deg_t i) const;

    ScalarPointer data() const;
    ScalarPointer data();


};





}}



#endif // ROUGHPY_SCALARS_SCALAR_MATRIX_H_