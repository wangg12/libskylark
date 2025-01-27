#ifndef SKYLARK_HASH_TRANSFORM_HPP
#define SKYLARK_HASH_TRANSFORM_HPP

#ifndef SKYLARK_SKETCH_HPP
#error "Include top-level sketch.hpp instead of including individuals headers"
#else
    #include "transforms.hpp"
    #include "hash_transform_data.hpp"
    #include "sketch_transform_data.hpp"
    #include "sketch_transform.hpp"
#endif

#include "../utility/distributions.hpp"

namespace skylark { namespace sketch {

template < typename InputMatrixType,
           typename OutputMatrixType,
           template <typename> class IdxDistributionType,
           template <typename> class ValueDistribution
        >
struct hash_transform_t :
        public hash_transform_data_t<IdxDistributionType,
                                     ValueDistribution> {
    // To be specialized and derived. Just some guards here.
    typedef InputMatrixType matrix_type;
    typedef OutputMatrixType output_matrix_type;
    typedef hash_transform_data_t<IdxDistributionType, ValueDistribution>
    data_type;

    hash_transform_t(int N, int S, base::context_t& context)
        : data_type(N, S, context) {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for HashTransform"));
    }

    hash_transform_t(const data_type& other_data)
        : data_type(other_data) {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for HashTransform"));
    }

    hash_transform_t(const boost::property_tree::ptree &pt)
        : data_type(pt) {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for HashTransform"));
    }

    void apply (const matrix_type& A,
                output_matrix_type& sketch_of_A,
                columnwise_tag dimension) const {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for HashTransform"));
    }

    void apply (const matrix_type& A,
                output_matrix_type& sketch_of_A,
                rowwise_tag dimension) const {
        SKYLARK_THROW_EXCEPTION (
          base::sketch_exception()
              << base::error_msg(
                 "This combination has not yet been implemented for HashTransform"));
    }
};

} } /** namespace skylark::sketch */

#include "hash_transform_Elemental.hpp"

#if SKYLARK_HAVE_COMBBLAS
#include "hash_transform_Mixed.hpp"
#include "hash_transform_CombBLAS.hpp"
#endif

#include "hash_transform_local_sparse.hpp"

#endif // SKYLARK_HASH_TRANSFORM_HPP
