#ifndef SKYLARK_RLT_HPP
#define SKYLARK_RLT_HPP

#ifndef SKYLARK_SKETCH_HPP
#error "Include top-level sketch.hpp instead of including individuals headers"
#endif

namespace skylark { namespace sketch {

namespace bstrand = boost::random;

/**
 * Random Laplace Transform
 *
 * Sketch transform into Eucledian space of fuctions in an RKHS
 * implicitly defined by a vector and a semigroup kernel.
 *
 * See:
 *
 * Random Laplace Feature Maps for Semigroup Kernels on Histograms
 *
 */
template < typename InputMatrixType,
           typename OutputMatrixType,
           template <typename> class KernelDistribution>
class RLT_t {
    // To be specilized and derived.
    typedef InputMatrixType matrix_type;
    typedef OutputMatrixType output_matrix_type;
    typedef RLT_data_t<KernelDistribution> data_type;

    RLT_t(int N, int S, base::context_t& context)
        : data_type(N, S, context) {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for RLT"));
    }

    RLT_t(const data_type& other_data)
        : data_type(other_data) {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for RLT"));
    }

    RLT_t(const boost::property_tree::ptree &pt)
        : data_type(pt) {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for RLT"));
    }

    void apply (const matrix_type& A,
                output_matrix_type& sketch_of_A,
                columnwise_tag dimension) const {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for RLT"));
    }

    void apply (const matrix_type& A,
                output_matrix_type& sketch_of_A,
                rowwise_tag dimension) const {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for RLT"));
    }
};

/**
 * Random Features for Exponential Semigroup
 */
template< typename InputMatrixType,
          typename OutputMatrixType>
struct ExpSemigroupRLT_t :
    public ExpSemigroupRLT_data_t,
    virtual public sketch_transform_t<InputMatrixType, OutputMatrixType > {

    // We use composition to defer calls to RLT_t
    typedef RLT_t<InputMatrixType, OutputMatrixType,
                  utility::standard_levy_distribution_t > transform_t;

    typedef ExpSemigroupRLT_data_t data_type;
    typedef data_type::params_t params_t;

    ExpSemigroupRLT_t(int N, int S, double beta,
        base::context_t& context)
        : data_type(N, S, beta, context), _transform(*this) {

    }

    ExpSemigroupRLT_t(int N, int S, const params_t& params,
        base::context_t& context)
        : data_type(N, S, params, context), _transform(*this) {

    }

    ExpSemigroupRLT_t(const boost::property_tree::ptree &pt)
        : data_type(pt), _transform(*this) {

    }

    template <typename OtherInputMatrixType,
              typename OtherOutputMatrixType>
    ExpSemigroupRLT_t(
        const ExpSemigroupRLT_t<OtherInputMatrixType, OtherOutputMatrixType>& other)
        : data_type(other), _transform(*this) {

    }

    ExpSemigroupRLT_t (const data_type& other)
        : data_type(other), _transform(*this) {

    }

    /**
     * Apply columnwise the sketching transform that is described by the
     * the transform with output sketch_of_A.
     */
    void apply (const typename transform_t::matrix_type& A,
                typename transform_t::output_matrix_type& sketch_of_A,
                columnwise_tag dimension) const {
        _transform.apply(A, sketch_of_A, dimension);
    }

    /**
     * Apply rowwise the sketching transform that is described by the
     * the transform with output sketch_of_A.
     */
    void apply (const typename transform_t::matrix_type& A,
                typename transform_t::output_matrix_type& sketch_of_A,
                rowwise_tag dimension) const {
        _transform.apply(A, sketch_of_A, dimension);
    }

    int get_N() const { return this->_N; } /**< Get input dimesion. */
    int get_S() const { return this->_S; } /**< Get output dimesion. */

    const sketch_transform_data_t* get_data() const { return this; }

private:
    transform_t _transform;

};

} } /** namespace skylark::sketch */


#include "RLT_Elemental.hpp"

#endif // SKYLARK_RLT_HPP
