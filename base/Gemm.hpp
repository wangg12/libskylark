#ifndef SKYLARK_GEMM_HPP
#define SKYLARK_GEMM_HPP

#include <boost/mpi.hpp>
#include "exception.hpp"
#include "sparse_matrix.hpp"
#include "computed_matrix.hpp"
#include "../utility/typer.hpp"

#include "Gemm_detail.hpp"

// Defines a generic Gemm function that receives both dense and sparse matrices.

namespace skylark { namespace base {

/**
 * Rename the elemental Gemm function, so that we have unified access.
 */

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::Matrix<T>& A, const elem::Matrix<T>& B,
    T beta, elem::Matrix<T>& C) {
    elem::Gemm(oA, oB, alpha, A, B, beta, C);
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::Matrix<T>& A, const elem::Matrix<T>& B,
    elem::Matrix<T>& C) {
    elem::Gemm(oA, oB, alpha, A, B, C);
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::STAR, elem::STAR>& A,
    const elem::DistMatrix<T, elem::STAR, elem::STAR>& B,
    T beta, elem::DistMatrix<T, elem::STAR, elem::STAR>& C) {
    elem::Gemm(oA, oB, alpha, A.LockedMatrix(), B.LockedMatrix(), beta, C.Matrix());
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::STAR, elem::STAR>& A,
    const elem::DistMatrix<T, elem::STAR, elem::STAR>& B,
    elem::DistMatrix<T, elem::STAR, elem::STAR>& C) {
    elem::Gemm(oA, oB, alpha, A.LockedMatrix(), B.LockedMatrix(), C.Matrix());
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T>& A, const elem::DistMatrix<T>& B,
    T beta, elem::DistMatrix<T>& C) {
    elem::Gemm(oA, oB, alpha, A, B, beta, C);
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T>& A, const elem::DistMatrix<T>& B,
    elem::DistMatrix<T>& C) {
    elem::Gemm(oA, oB, alpha, A, B, C);
}

/**
 * The following combinations is not offered by Elemental, but is useful for us.
 * We implement it partially.
 */

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VC, elem::STAR>& A,
    const elem::DistMatrix<T, elem::VC, elem::STAR>& B,
    T beta, elem::DistMatrix<T, elem::STAR, elem::STAR>& C) {
    // TODO verify sizes etc.

    if ((oA == elem::TRANSPOSE || oA == elem::ADJOINT) && oB == elem::NORMAL) {
        boost::mpi::communicator comm(C.Grid().Comm(), boost::mpi::comm_attach);
        elem::Matrix<T> Clocal(C.Matrix());
        elem::Gemm(oA, elem::NORMAL,
            alpha, A.LockedMatrix(), B.LockedMatrix(),
            beta / T(comm.size()), Clocal);
        boost::mpi::all_reduce(comm,
            Clocal.Buffer(), Clocal.MemorySize(), C.Matrix().Buffer(),
            std::plus<T>());
    } else {
        SKYLARK_THROW_EXCEPTION(base::unsupported_base_operation());
    }
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VC, elem::STAR>& A,
    const elem::DistMatrix<T, elem::VC, elem::STAR>& B,
    elem::DistMatrix<T, elem::STAR, elem::STAR>& C) {

    int C_height = (oA == elem::NORMAL ? A.Height() : A.Width());
    int C_width = (oB == elem::NORMAL ? B.Width() : B.Height());
    elem::Zeros(C, C_height, C_width);
    base::Gemm(oA, oB, alpha, A, B, T(0), C);
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VC, elem::STAR>& A,
    const elem::DistMatrix<T, elem::STAR, elem::STAR>& B,
    T beta, elem::DistMatrix<T, elem::VC, elem::STAR>& C) {
    // TODO verify sizes etc.

    if (oA == elem::NORMAL && oB == elem::NORMAL) {
        elem::Gemm(elem::NORMAL, elem::NORMAL,
            alpha, A.LockedMatrix(), B.LockedMatrix(),
            beta, C.Matrix());
    } else {
        SKYLARK_THROW_EXCEPTION(base::unsupported_base_operation());
    }
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VC, elem::STAR>& A,
    const elem::DistMatrix<T, elem::STAR, elem::STAR>& B,
    elem::DistMatrix<T, elem::VC, elem::STAR>& C) {

    int C_height = (oA == elem::NORMAL ? A.Height() : A.Width());
    int C_width = (oB == elem::NORMAL ? B.Width() : B.Height());
    elem::Zeros(C, C_height, C_width);
    base::Gemm(oA, oB, alpha, A, B, T(0), C);
}


template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VR, elem::STAR>& A,
    const elem::DistMatrix<T, elem::VR, elem::STAR>& B,
    T beta, elem::DistMatrix<T, elem::STAR, elem::STAR>& C) {
    // TODO verify sizes etc.

    if ((oA == elem::TRANSPOSE || oA == elem::ADJOINT) && oB == elem::NORMAL) {
        boost::mpi::communicator comm(C.Grid().Comm(), boost::mpi::comm_attach);
        elem::Matrix<T> Clocal(C.Matrix());
        elem::Gemm(oA, elem::NORMAL,
            alpha, A.LockedMatrix(), B.LockedMatrix(),
            beta / T(comm.size()), Clocal);
        boost::mpi::all_reduce(comm,
            Clocal.Buffer(), Clocal.MemorySize(), C.Matrix().Buffer(),
            std::plus<T>());
    } else {
        SKYLARK_THROW_EXCEPTION(base::unsupported_base_operation());
    }
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VR, elem::STAR>& A,
    const elem::DistMatrix<T, elem::VR, elem::STAR>& B,
    elem::DistMatrix<T, elem::STAR, elem::STAR>& C) {

    int C_height = (oA == elem::NORMAL ? A.Height() : A.Width());
    int C_width = (oB == elem::NORMAL ? B.Width() : B.Height());
    elem::Zeros(C, C_height, C_width);
    base::Gemm(oA, oB, alpha, A, B, T(0), C);
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VR, elem::STAR>& A,
    const elem::DistMatrix<T, elem::STAR, elem::STAR>& B,
    T beta, elem::DistMatrix<T, elem::VR, elem::STAR>& C) {
    // TODO verify sizes etc.

    if (oA == elem::NORMAL && oB == elem::NORMAL) {
        elem::Gemm(elem::NORMAL, elem::NORMAL,
            alpha, A.LockedMatrix(), B.LockedMatrix(),
            beta, C.Matrix());
    } else {
        SKYLARK_THROW_EXCEPTION(base::unsupported_base_operation());
    }
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::DistMatrix<T, elem::VR, elem::STAR>& A,
    const elem::DistMatrix<T, elem::STAR, elem::STAR>& B,
    elem::DistMatrix<T, elem::VR, elem::STAR>& C) {

    int C_height = (oA == elem::NORMAL ? A.Height() : A.Width());
    int C_width = (oB == elem::NORMAL ? B.Width() : B.Height());
    elem::Zeros(C, C_height, C_width);
    base::Gemm(oA, oB, alpha, A, B, T(0), C);
}

/**
 * Gemm between mixed elemental, sparse input. Output is dense elemental.
 */

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const elem::Matrix<T>& A, const sparse_matrix_t<T>& B,
    T beta, elem::Matrix<T>& C) {
    // TODO verify sizes etc.

    const int* indptr = B.indptr();
    const int* indices = B.indices();
    const T *values = B.locked_values();

    int k = A.Width();
    int n = B.width();
    int m = A.Height();

    if (oA == elem::ADJOINT && std::is_same<T, elem::Base<T> >::value)
        oA = elem::TRANSPOSE;

    if (oB == elem::ADJOINT && std::is_same<T, elem::Base<T> >::value)
        oB = elem::TRANSPOSE;

    if (oA == elem::ADJOINT || oB == elem::ADJOINT)
        SKYLARK_THROW_EXCEPTION(base::unsupported_base_operation());

    // NN
    if (oA == elem::NORMAL && oB == elem::NORMAL) {

        elem::Scal(beta, C);

        elem::Matrix<T> Ac;
        elem::Matrix<T> Cc;

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for private(Cc, Ac)
#       endif
        for(int col = 0; col < n; col++) {
            elem::View(Cc, C, 0, col, m, 1);
            for (int j = indptr[col]; j < indptr[col + 1]; j++) {
                int row = indices[j];
                T val = values[j];
                elem::LockedView(Ac, A, 0, row, m, 1);
                elem::Axpy(alpha * val, Ac, Cc);
            }
        }
    }

    // NT
    if (oA == elem::NORMAL && oB == elem::TRANSPOSE) {

        elem::Scal(beta, C);

        elem::Matrix<T> Ac;
        elem::Matrix<T> Cc;

        // Now, we simply think of B has being in CSR mode...
        int row = 0;
        for(int row = 0; row < n; row++) {
            elem::LockedView(Ac, A, 0, row, m, 1);
#           if SKYLARK_HAVE_OPENMP
#           pragma omp parallel for private(Cc)
#           endif
            for (int j = indptr[row]; j < indptr[row + 1]; j++) {
                int col = indices[j];
                T val = values[j];
                elem::View(Cc, C, 0, col, m, 1);
                elem::Axpy(alpha * val, Ac, Cc);
            }
        }
    }


    // TN - TODO: Not tested!
    if (oA == elem::TRANSPOSE && oB == elem::NORMAL) {
        double *c = C.Buffer();
        int ldc = C.LDim();

        const double *a = A.LockedBuffer();
        int lda = A.LDim();

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for collapse(2)
#       endif
        for (int j = 0; j < n; j++)
            for(int row = 0; row < k; row++) {
                c[j * ldc + row] *= beta;
                 for (int l = indptr[j]; l < indptr[j + 1]; l++) {
                     int rr = indices[l];
                     T val = values[l];
                     c[j * ldc + row] += val * a[j * lda + rr];
                 }
            }
    }

    // TT - TODO: Not tested!
    if (oA == elem::TRANSPOSE && oB == elem::TRANSPOSE) {
        elem::Scal(beta, C);

        double *c = C.Buffer();
        int ldc = C.LDim();

        const double *a = A.LockedBuffer();
        int lda = A.LDim();

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for
#       endif
        for(int row = 0; row < k; row++)
            for(int rb = 0; rb < n; rb++)
                for (int l = indptr[rb]; l < indptr[rb + 1]; l++) {
                    int col = indices[l];
                    c[col * ldc + row] += values[l] * a[row * lda + rb];
                }
    }
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const sparse_matrix_t<T>& A, const elem::Matrix<T>& B,
    T beta, elem::Matrix<T>& C) {
    // TODO verify sizes etc.

    const int* indptr = A.indptr();
    const int* indices = A.indices();
    const double *values = A.locked_values();

    int k = A.width();
    int n = B.Width();
    int m = B.Height();

    if (oA == elem::ADJOINT && std::is_same<T, elem::Base<T> >::value)
        oA = elem::TRANSPOSE;

    if (oB == elem::ADJOINT && std::is_same<T, elem::Base<T> >::value)
        oB = elem::TRANSPOSE;

    // NN
    if (oA == elem::NORMAL && oB == elem::NORMAL) {

        elem::Scal(beta, C);

        double *c = C.Buffer();
        int ldc = C.LDim();

        const double *b = B.LockedBuffer();
        int ldb = B.LDim();

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for
#       endif
        for(int i = 0; i < n; i++)
            for(int col = 0; col < k; col++)
                 for (int j = indptr[col]; j < indptr[col + 1]; j++) {
                     int row = indices[j];
                     T val = values[j];
                     c[i * ldc + row] += alpha * val * b[i * ldb + col];
                 }
    }

    // NT
    if (oA == elem::NORMAL && (oB == elem::TRANSPOSE || oB == elem::ADJOINT)) {

        elem::Scal(beta, C);

        elem::Matrix<T> Bc;
        elem::Matrix<T> BTr;
        elem::Matrix<T> Cr;

        for(int col = 0; col < k; col++) {
            elem::LockedView(Bc, B, 0, col, m, 1);
            elem::Transpose(Bc, BTr, oB == elem::ADJOINT);
#           if SKYLARK_HAVE_OPENMP
#           pragma omp parallel for private(Cr)
#           endif
            for (int j = indptr[col]; j < indptr[col + 1]; j++) {
                int row = indices[j];
                T val = values[j];
                elem::View(Cr, C, row, 0, 1, m);
                elem::Axpy(alpha * val, BTr, Cr);
            }
        }
    }

    // TN - TODO: Not tested!
    if (oA == elem::TRANSPOSE && oB == elem::NORMAL) {
        double *c = C.Buffer();
        int ldc = C.LDim();

        const double *b = B.LockedBuffer();
        int ldb = B.LDim();

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for collapse(2)
#       endif
        for (int j = 0; j < n; j++)
            for(int row = 0; row < k; row++) {
                c[j * ldc + row] *= beta;
                 for (int l = indptr[row]; l < indptr[row + 1]; l++) {
                     int col = indices[l];
                     T val = values[l];
                     c[j * ldc + row] += val * b[j * ldb + col];
                 }
            }
    }

    // AN - TODO: Not tested!
    if (oA == elem::ADJOINT && oB == elem::NORMAL) {
        double *c = C.Buffer();
        int ldc = C.LDim();

        const double *b = B.LockedBuffer();
        int ldb = B.LDim();

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for collapse(2)
#       endif
        for (int j = 0; j < n; j++)
            for(int row = 0; row < k; row++) {
                c[j * ldc + row] *= beta;
                 for (int l = indptr[row]; l < indptr[row + 1]; l++) {
                     int col = indices[l];
                     T val = elem::Conj(values[l]);
                     c[j * ldc + row] += val * b[j * ldb + col];
                 }
            }
    }


    // TT - TODO: Not tested!
    if (oA == elem::TRANSPOSE && (oB == elem::TRANSPOSE || oB == elem::ADJOINT)) {

        elem::Scal(beta, C);

        elem::Matrix<T> Bc;
        elem::Matrix<T> BTr;
        elem::Matrix<T> Cr;

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for private(Cr, Bc, BTr)
#       endif
        for(int row = 0; row < k; row++) {
            elem::View(Cr, C, row, 0, 1, m);
            for (int l = indptr[row]; l < indptr[row + 1]; l++) {
                int col = indices[l];
                T val = values[l];
                elem::LockedView(Bc, B, 0, col, m, 1);
                elem::Transpose(Bc, BTr, oB == elem::ADJOINT);
                elem::Axpy(alpha * val, BTr, Cr);
            }
        }
    }

    // AT - TODO: Not tested!
    if (oA == elem::ADJOINT && (oB == elem::TRANSPOSE || oB == elem::ADJOINT)) {

        elem::Scal(beta, C);

        elem::Matrix<T> Bc;
        elem::Matrix<T> BTr;
        elem::Matrix<T> Cr;

#       if SKYLARK_HAVE_OPENMP
#       pragma omp parallel for private(Cr, Bc, BTr)
#       endif
        for(int row = 0; row < k; row++) {
            elem::View(Cr, C, row, 0, 1, m);
            for (int l = indptr[row]; l < indptr[row + 1]; l++) {
                int col = indices[l];
                T val = elem::Conj(values[l]);
                elem::LockedView(Bc, B, 0, col, m, 1);
                elem::Transpose(Bc, BTr, oB == elem::ADJOINT);
                elem::Axpy(alpha * val, BTr, Cr);
            }
        }
    }
}

template<typename T>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    T alpha, const sparse_matrix_t<T>& A, const elem::Matrix<T>& B,
    elem::Matrix<T>& C) {
    int C_height = (oA == elem::NORMAL ? A.height() : A.width());
    int C_width = (oB == elem::NORMAL ? B.Width() : B.Height());
    elem::Zeros(C, C_height, C_width);
    base::Gemm(oA, oB, alpha, A, B, T(0), C);
}

#if SKYLARK_HAVE_COMBBLAS
/**
 * Mixed GEMM for Elemental and CombBLAS matrices. For a distributed Elemental
 * input matrix, the output has the same distribution.
 */

/// Gemm for distCombBLAS x distElemental(* / *) -> distElemental (SOMETHING / *)
template<typename index_type, typename value_type, elem::Distribution col_d>
void Gemm(elem::Orientation oA, elem::Orientation oB, double alpha,
          const SpParMat<index_type, value_type, SpDCCols<index_type, value_type> > &A,
          const elem::DistMatrix<value_type, elem::STAR, elem::STAR> &B,
          double beta,
          elem::DistMatrix<value_type, col_d, elem::STAR> &C) {

    if(oA == elem::NORMAL && oB == elem::NORMAL) {

        if(A.getnol() != B.Height())
            SKYLARK_THROW_EXCEPTION (
                base::combblas_exception()
                    << base::error_msg("Gemm: Dimensions do not agree"));

        if(A.getnrow() != C.Height())
            SKYLARK_THROW_EXCEPTION (
                base::combblas_exception()
                    << base::error_msg("Gemm: Dimensions do not agree"));

        if(B.Width() != C.Width())
            SKYLARK_THROW_EXCEPTION (
                base::combblas_exception()
                    << base::error_msg("Gemm: Dimensions do not agree"));

        //XXX: simple heuristic to decide what to communicate (improve!)
        //     or just if A.getncol() < B.Width..
        if(A.getnnz() < B.Height() * B.Width())
            detail::outer_panel_mixed_gemm_impl_nn(alpha, A, B, beta, C);
        else
            detail::inner_panel_mixed_gemm_impl_nn(alpha, A, B, beta, C);
    }
}

/// Gemm for distCombBLAS x distElemental(SOMETHING / *) -> distElemental (* / *)
template<typename index_type, typename value_type, elem::Distribution col_d>
void Gemm(elem::Orientation oA, elem::Orientation oB, double alpha,
          const SpParMat<index_type, value_type, SpDCCols<index_type, value_type> > &A,
          const elem::DistMatrix<value_type, col_d, elem::STAR> &B,
          double beta,
          elem::DistMatrix<value_type, elem::STAR, elem::STAR> &C) {

    if(oA == elem::TRANSPOSE && oB == elem::NORMAL) {

        if(A.getrow() != B.Height())
            SKYLARK_THROW_EXCEPTION (
                base::combblas_exception()
                    << base::error_msg("Gemm: Dimensions do not agree"));

        if(A.getncol() != C.Height())
            SKYLARK_THROW_EXCEPTION (
                    base::combblas_exception()
                    << base::error_msg("Gemm: Dimensions do not agree"));

        if(B.Width() != C.Width())
            SKYLARK_THROW_EXCEPTION (
                base::combblas_exception()
                    << base::error_msg("Gemm: Dimensions do not agree"));

        detail::outer_panel_mixed_gemm_impl_tn(alpha, A, B, beta, C);
    }

}

#endif // SKYLARK_HAVE_COMBBLAS

/* All combinations with computed matrix */

template<typename CT, typename RT, typename OT>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    typename utility::typer_t<OT>::value_type alpha, const computed_matrix_t<CT>& A,
    const RT& B, typename utility::typer_t<OT>::value_type beta, OT& C) {
    base::Gemm(oA, oB, alpha, A.materialize(), B, beta, C);
}

template<typename CT, typename RT, typename OT>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    typename utility::typer_t<OT>::value_type alpha, const computed_matrix_t<CT>& A,
    const RT& B, OT& C) {
    base::Gemm(oA, oB, alpha, A.materialize(), B, C);
}

template<typename CT, typename RT, typename OT>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    typename utility::typer_t<OT>::value_type alpha, const RT& A,
    const computed_matrix_t<CT>& B,
    typename utility::typer_t<OT>::value_type beta, OT& C) {
    base::Gemm(oA, oB, alpha, A, B.materialize(), beta, C);
}

template<typename CT, typename RT, typename OT>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    typename utility::typer_t<OT>::value_type alpha, const RT& A,
    const computed_matrix_t<CT>& B, OT& C) {
    base::Gemm(oA, oB, alpha, A, B.materialize(), C);
}

template<typename CT1, typename CT2, typename OT>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    typename utility::typer_t<OT>::value_type alpha, const computed_matrix_t<CT1>& A,
    const computed_matrix_t<CT2>& B, typename utility::typer_t<OT>::value_type beta,
    OT& C) {
    base::Gemm(oA, oB, alpha, A.materialize(), B.materialize(), beta, C);
}

template<typename CT1, typename CT2, typename OT>
inline void Gemm(elem::Orientation oA, elem::Orientation oB,
    typename utility::typer_t<OT>::value_type alpha, const computed_matrix_t<CT1>& A,
    const computed_matrix_t<CT2>& B, OT& C) {
    base::Gemm(oA, oB, alpha, A.materialize(), B.materialize(), C);
}


} } // namespace skylark::base
#endif // SKYLARK_GEMM_HPP
