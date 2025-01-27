#ifndef SKYLARK_ACCELERATED_LINEARL2_REGRESSION_SOLVER_ELEMENTAL_HPP
#define SKYLARK_ACCELERATED_LINEARL2_REGRESSION_SOLVER_ELEMENTAL_HPP

#include <elemental.hpp>

#include "regression_problem.hpp"

namespace skylark {
namespace algorithms {

namespace flinl2_internal {

extern "C" {

void BLAS(dtrcon)(const char* norm, const char* uplo, const char* diag,
    const int* n, const double* A, const int *lda, double *rcond,
    double *work, int *iwork, int *info);

void BLAS(strcon)(const char* norm, const char* uplo, const char* diag,
    const int* n, const float* A, const int *lda, float *rcond,
    float *work, int *iwork, int *info);

}

inline double utcondest(const elem::Matrix<double>& A) {
    int n = A.Height(), ld = A.LDim(), info;
    double *work = new double[3 * n];
    int *iwork = new int[n];
    double rcond;
    BLAS(dtrcon)("1", "U", "N", &n, A.LockedBuffer(), &ld, &rcond, work,
        iwork, &info); // TODO check info
    delete[] work;
    delete[] iwork;
    return 1/rcond;
}

inline double utcondest(const elem::Matrix<float>& A) {
    int n = A.Height(), ld = A.LDim(), info;
    float *work  = new float[3 * n];
    int *iwork = new int[n];
    float rcond;
    BLAS(strcon)("1", "U", "N", &n, A.LockedBuffer(), &ld, &rcond, work,
        iwork, &info);  // TODO check info
    delete[] work;
    delete[] iwork;
    return 1/rcond;
}

template<typename T>
inline double utcondest(const elem::DistMatrix<T, elem::STAR, elem::STAR>& A) {
    return utcondest(A.LockedMatrix());
}

template<typename SolType, typename SketchType, typename PrecondType>
double build_precond(SketchType& SA,
    PrecondType& R, algorithms::inplace_precond_t<SolType> *&P, qr_precond_tag) {
    elem::qr::Explicit(SA.Matrix(), R.Matrix()); // TODO
    P =
        new algorithms::inplace_tri_inverse_precond_t<SolType, PrecondType,
                                       elem::UPPER, elem::NON_UNIT>(R);
    return utcondest(R);
}

template<typename SolType, typename SketchType, typename PrecondType>
double build_precond(SketchType& SA,
    PrecondType& V, algorithms::inplace_precond_t<SolType> *&P, svd_precond_tag) {

    int n = SA.Width();
    PrecondType s(SA);
    s.Resize(n, 1);
    elem::SVD(SA.Matrix(), s.Matrix(), V.Matrix()); // TODO
    for(int i = 0; i < n; i++)
        s.Set(i, 0, 1 / s.Get(i, 0));
    base::DiagonalScale(elem::RIGHT, elem::NORMAL, s, V);
    P =
        new algorithms::inplace_mat_precond_t<SolType, PrecondType>(V);
    return s.Get(0,0) / s.Get(n-1, 0);
}

}  // namespace flinl2_internal

/// Specialization for simplified Blendenpik algorithm
template <typename ValueType, elem::Distribution VD,
          template <typename, typename> class TransformType,
          typename PrecondTag>
class accelerated_regression_solver_t<
    regression_problem_t<elem::DistMatrix<ValueType, VD, elem::STAR>,
                         linear_tag, l2_tag, no_reg_tag>,
    elem::DistMatrix<ValueType, VD, elem::STAR>,
    elem::DistMatrix<ValueType, elem::STAR, elem::STAR>,
    simplified_blendenpik_tag<TransformType, PrecondTag> > {

public:

    typedef ValueType value_type;

    typedef elem::DistMatrix<ValueType, VD, elem::STAR> matrix_type;
    typedef elem::DistMatrix<ValueType, VD, elem::STAR> rhs_type;
    typedef elem::DistMatrix<ValueType, elem::STAR, elem::STAR> sol_type;

    typedef regression_problem_t<matrix_type,
                                 linear_tag, l2_tag, no_reg_tag> problem_type;

private:

    typedef elem::DistMatrix<ValueType, elem::STAR, elem::STAR> precond_type;
    typedef precond_type sketch_type;
    // The assumption is that the sketch is not much bigger than the
    // preconditioner, so we should use the same matrix distribution.

    const int _m;
    const int _n;
    const matrix_type &_A;
    precond_type _R;
    algorithms::inplace_precond_t<sol_type> *_precond_R;

public:
    /**
     * Prepares the regressor to quickly solve given a right-hand side.
     *
     * @param problem Problem to solve given right-hand side.
     */
    accelerated_regression_solver_t(const problem_type& problem, 
        base::context_t& context) :
        _m(problem.m), _n(problem.n), _A(problem.input_matrix),
        _R(_n, _n, problem.input_matrix.Grid()) {
        // TODO n < m ???

        int t = 4 * _n;    // TODO parameter.

        TransformType<matrix_type, sketch_type> S(_m, t, context);
        sketch_type SA(t, _n);
        S.apply(_A, SA, sketch::columnwise_tag());

        flinl2_internal::build_precond(SA, _R, _precond_R, PrecondTag());
    }

    ~accelerated_regression_solver_t() {
        delete _precond_R;
    }

    int solve(const rhs_type& b, sol_type& x) {
        return LSQR(_A, b, x, algorithms::krylov_iter_params_t(), *_precond_R);
    }
};

/// Specialization for Blendenpik algorithm
template <typename ValueType, elem::Distribution VD,
          typename PrecondTag>
class accelerated_regression_solver_t<
    regression_problem_t<elem::DistMatrix<ValueType, VD, elem::STAR>,
                         linear_tag, l2_tag, no_reg_tag>,
    elem::DistMatrix<ValueType, VD, elem::STAR>,
    elem::DistMatrix<ValueType, elem::STAR, elem::STAR>,
    blendenpik_tag<PrecondTag> > {

public:

    typedef ValueType value_type;

    typedef elem::DistMatrix<ValueType, VD, elem::STAR> matrix_type;
    typedef elem::DistMatrix<ValueType, VD, elem::STAR> rhs_type;
    typedef elem::DistMatrix<ValueType, elem::STAR, elem::STAR> sol_type;

    typedef regression_problem_t<matrix_type,
                                 linear_tag, l2_tag, no_reg_tag> problem_type;

private:

    typedef elem::DistMatrix<ValueType, elem::STAR, elem::STAR> precond_type;
    typedef precond_type sketch_type; 
    // The assumption is that the sketch is not much bigger than the
    // preconditioner, so we should use the same matrix distribution.

    const int _m;
    const int _n;
    const matrix_type &_A;
    precond_type _R;
    algorithms::inplace_precond_t<sol_type> *_precond_R;

    regression_solver_t<problem_type, rhs_type, sol_type, svd_l2_solver_tag> 
    *_alt_solver;

public:
    /**
     * Prepares the regressor to quickly solve given a right-hand side.
     *
     * @param problem Problem to solve given right-hand side.
     */
    accelerated_regression_solver_t(const problem_type& problem, base::context_t& context) :
        _m(problem.m), _n(problem.n), _A(problem.input_matrix),
        _R(_n, _n, problem.input_matrix.Grid()) {
        // TODO n < m ???

        int t = 4 * _n;    // TODO parameter.
        double scale = std::sqrt((double)_m / (double)t);

        elem::DistMatrix<ValueType, elem::STAR, VD> Ar(_A.Grid());
        elem::DistMatrix<ValueType, elem::STAR, VD> dist_SA(t, _n, _A.Grid());
        sketch_type SA(t, _n, _A.Grid());
        boost::random::uniform_int_distribution<int> distribution(0, _m- 1);

        Ar = _A;
        double condest = 0;
        int attempts = 0;
        do {
            sketch::RFUT_t<elem::DistMatrix<ValueType, elem::STAR, VD>,
                           sketch::fft_futs<double>::DCT_t,
                           utility::rademacher_distribution_t<value_type> > 
                F(_m, context);
            F.apply(Ar, Ar, sketch::columnwise_tag());

            std::vector<int> samples =
                context.generate_random_samples_array(t, distribution);
            for (int j = 0; j < Ar.LocalWidth(); j++)
                for (int i = 0; i < t; i++) {
                    int row = samples[i];
                    dist_SA.Matrix().Set(i, j, scale * Ar.Matrix().Get(row, j));
                }

            SA = dist_SA;
            condest = flinl2_internal::build_precond(SA, _R, _precond_R, PrecondTag());
            attempts++;
        } while (condest > 1e14 && attempts < 3); // TODO parameters

        if (condest <= 1e14)
            _alt_solver = nullptr;
        else {
            _alt_solver =
                new regression_solver_t<problem_type,
                                      rhs_type,
                                      sol_type, svd_l2_solver_tag>(problem);
            delete _precond_R;
            _precond_R = nullptr;
        }
    }

    ~accelerated_regression_solver_t() {
        if (_precond_R != nullptr)
            delete _precond_R;
        if (_alt_solver != nullptr)
            delete _alt_solver;
    }

    int solve(const rhs_type& b, sol_type& x) {
        return LSQR(_A, b, x, algorithms::krylov_iter_params_t(), *_precond_R);
    }
};

/// Specialization for LSRN algorithm.
template <typename ValueType, elem::Distribution VD,
          typename PrecondTag>
class accelerated_regression_solver_t<
    regression_problem_t<elem::DistMatrix<ValueType, VD, elem::STAR>,
                         linear_tag, l2_tag, no_reg_tag>,
    elem::DistMatrix<ValueType, VD, elem::STAR>,
    elem::DistMatrix<ValueType, elem::STAR, elem::STAR>,
    lsrn_tag<PrecondTag> > {

public:

    typedef ValueType value_type;

    typedef elem::DistMatrix<ValueType, VD, elem::STAR> matrix_type;
    typedef elem::DistMatrix<ValueType, VD, elem::STAR> rhs_type;
    typedef elem::DistMatrix<ValueType, elem::STAR, elem::STAR> sol_type;

    typedef regression_problem_t<matrix_type,
                                 linear_tag, l2_tag, no_reg_tag> problem_type;

private:

    typedef elem::DistMatrix<ValueType, elem::STAR, elem::STAR> precond_type;
    typedef precond_type sketch_type;
    // The assumption is that the sketch is not much bigger than the
    // preconditioner, so we should use the same matrix distribution.

    const int _m;
    const int _n;
    const matrix_type &_A;
    bool _use_lsqr;
    double _sigma_U, _sigma_L;
    precond_type _R;
    algorithms::inplace_precond_t<sol_type> *_precond_R;
    algorithms::krylov_iter_params_t _params;

public:
    /**
     * Prepares the regressor to quickly solve given a right-hand side.
     *
     * @param problem Problem to solve given right-hand side.
     */
    accelerated_regression_solver_t(const problem_type& problem, base::context_t& context) :
        _m(problem.m), _n(problem.n), _A(problem.input_matrix),
        _R(_n, _n, problem.input_matrix.Grid()) {
        // TODO n < m ???

        int t = 4 * _n;    // TODO parameter.
        double epsilon = 1e-14;  // TODO parameter
        double delta = 1e-6; // TODO parameter

        sketch::JLT_t<matrix_type, sketch_type> S(_m, t, context);
        sketch_type SA(t, _n);
        S.apply(_A, SA, sketch::columnwise_tag());
        flinl2_internal::build_precond(SA, _R, _precond_R, PrecondTag());

        // Select alpha so that probability of failure is delta.
        // If alpha is too big, we need to use LSQR (although ill-conditioning
        // might be very severe so to prevent convergence).
        double alpha = std::sqrt(2 * std::log(2.0 / delta) / t);
        if (alpha >= (1 - std::sqrt(_n / t)))
            _use_lsqr = true;
        else {
            _use_lsqr = false;
            _sigma_U = std::sqrt(t) / ((1 - alpha) * std::sqrt(t)
                - std::sqrt(_n));
            _sigma_L = std::sqrt(t) / ((1 + alpha) * std::sqrt(t)
                + std::sqrt(_n));
        }
    }

    ~accelerated_regression_solver_t() {
        delete _precond_R;
    }

    int solve(const rhs_type& b, sol_type& x) {
        int ret;
        if (_use_lsqr)
            ret = LSQR(_A, b, x, _params, *_precond_R);
        else {
            ChebyshevLS(_A, b, x,  _sigma_L, _sigma_U,
                _params, *_precond_R);
            ret = -6; // TODO! - check!
        }
        return ret; // TODO!
    }
};



} } /** namespace skylark::algorithms */

#endif // SKYLARK_ACCELERATED_LINEARL2_REGRESSION_SOLVER_ELEMENTAL_HPP
