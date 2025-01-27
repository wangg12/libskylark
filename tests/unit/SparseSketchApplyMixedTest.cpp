/**
 *  This test ensures that the sketch application (for mixed: CombBLAS ->
 *  Elemental matrices) is done correctly (on-the-fly matrix multiplication in
 *  the code is compared to true matrix multiplication).
 *  This test builds on the following assumptions:
 *
 *      - CombBLAS PSpGEMM returns the correct result, and
 *      - the random numbers in row_idx and row_value (see
 *        hash_transform_data_t) are drawn from the promised distributions.
 */


#include <vector>

#include <boost/mpi.hpp>
#include <boost/test/minimal.hpp>

//FIXME: ugly
#define SKYLARK_SKETCH_HPP 1

#include "../../utility/distributions.hpp"
#include "../../base/context.hpp"
#include "../../sketch/hash_transform.hpp"

#include "../../base/sparse_matrix.hpp"

#include "../../utility/external/combblas_comm_grid.hpp"

typedef FullyDistVec<size_t, double> mpi_vector_t;
typedef SpDCCols<size_t, double> col_t;
typedef SpParMat<size_t, double, col_t> DistMatrixType;
typedef PlusTimesSRing<double, double> PTDD;
typedef skylark::base::sparse_matrix_t<double> LocalMatrixType;


template < typename InputMatrixType,
           typename OutputMatrixType = InputMatrixType >
struct Dummy_t : public skylark::sketch::hash_transform_t<
    InputMatrixType, OutputMatrixType,
    boost::random::uniform_int_distribution,
    skylark::utility::rademacher_distribution_t > {

    typedef skylark::sketch::hash_transform_t<
        InputMatrixType, OutputMatrixType,
        boost::random::uniform_int_distribution,
        skylark::utility::rademacher_distribution_t >
            hash_t;

    Dummy_t(int N, int S, skylark::base::context_t& context)
        : skylark::sketch::hash_transform_t<InputMatrixType, OutputMatrixType,
          boost::random::uniform_int_distribution,
          skylark::utility::rademacher_distribution_t>(N, S, context)
    {}

    std::vector<size_t> getRowIdx() { return hash_t::row_idx; }
    std::vector<double> getRowValues() { return hash_t::row_value; }
};


template<typename sketch_t>
void compute_sketch_matrix(sketch_t sketch, const DistMatrixType &A,
                           DistMatrixType &result) {

    std::vector<size_t> row_idx = sketch.getRowIdx();
    std::vector<double> row_val = sketch.getRowValues();

    // PI generated by random number gen
    size_t sketch_size = row_val.size();
    mpi_vector_t cols(sketch_size);
    mpi_vector_t rows(sketch_size);
    mpi_vector_t vals(sketch_size);

    for(size_t i = 0; i < sketch_size; ++i) {
        cols.SetElement(i, i);
        rows.SetElement(i, row_idx[i]);
        vals.SetElement(i, row_val[i]);
    }

    result = DistMatrixType(result.getnrow(), result.getncol(),
                            rows, cols, vals);
}

void compare_result(size_t rank, DistMatrixType &expected_A,
                    elem::DistMatrix<double, elem::STAR, elem::STAR> &result) {

    col_t &data = expected_A.seq();

    const size_t my_row_offset = skylark::utility::cb_my_row_offset(expected_A);
    const size_t my_col_offset = skylark::utility::cb_my_col_offset(expected_A);

    for(typename col_t::SpColIter col = data.begcol();
        col != data.endcol(); col++) {
        for(typename col_t::SpColIter::NzIter nz = data.begnz(col);
            nz != data.endnz(col); nz++) {

            const size_t rowid = nz.rowid()  + my_row_offset;
            const size_t colid = col.colid() + my_col_offset;
            const double value = nz.value();

            if(value != result.GetLocal(rowid, colid)) {
                std::ostringstream os;
                os << rank << ": " << rowid << ", " << colid << ": "
                   << value << " != "
                   << result.GetLocal(rowid, colid) << std::endl;
                std::cout << os.str() << std::flush;
                BOOST_FAIL("Result application not as expected");
            }
        }
    }
}

int test_main(int argc, char *argv[]) {

    //////////////////////////////////////////////////////////////////////////
    //[> Parameters <]

    //FIXME: use random sizes?
    const size_t n   = 100;
    const size_t m   = 50;
    const size_t n_s = 60;
    const size_t m_s = 30;

    //////////////////////////////////////////////////////////////////////////
    //[> Setup test <]
    namespace mpi = boost::mpi;

#ifdef SKYLARK_HAVE_OPENMP
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
#endif

    mpi::environment env(argc, argv);
    mpi::communicator world;
    const size_t rank = world.rank();

    elem::Initialize(argc, argv);
    MPI_Comm mpi_world(world);
    elem::Grid grid(mpi_world);

    skylark::base::context_t context (0);

    double count = 1.0;

    const size_t matrix_full = n * m;
    mpi_vector_t colsf(matrix_full);
    mpi_vector_t rowsf(matrix_full);
    mpi_vector_t valsf(matrix_full);

    for(size_t i = 0; i < matrix_full; ++i) {
        colsf.SetElement(i, i % m);
        rowsf.SetElement(i, i / m);
        valsf.SetElement(i, count);
        count++;
    }

    DistMatrixType A(n, m, rowsf, colsf, valsf);
    mpi_vector_t zero;

    count = 1.0;
    elem::Matrix<double> local_A(n, m);
    for( size_t j = 0; j < local_A.Height(); j++ )
        for( size_t i = 0; i < local_A.Width(); i++ )
            local_A.Set(j, i, count++);


    elem::DistMatrix<double, elem::STAR, elem::STAR> result;

    // columnwise application
    DistMatrixType expected_A;
    DistMatrixType pi_sketch(n_s, n, zero, zero, zero);

    // rowwise application
    DistMatrixType expected_AR;
    DistMatrixType pi_sketch_r(m_s, m, zero, zero, zero);

    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> DistMatrix[MC/MR] <]

    typedef elem::DistMatrix<double> mcmr_target_t;

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, mcmr_target_t> Sparse(n, n_s, context);

    //[> 2. Create space for the sketched matrix <]
    mcmr_target_t sketch_A(n_s, m, grid);
    elem::Zero(sketch_A);

    //[> 3. Apply the transform <]
    Sparse.apply(A, sketch_A, skylark::sketch::columnwise_tag());

    //[> 4. Build structure to compare <]
    // easier to check if all processors own result
    result = sketch_A;
    elem::Display(result);

    compute_sketch_matrix(Sparse, A, pi_sketch);
    expected_A = Mult_AnXBn_Synch<PTDD, double, col_t>(
            pi_sketch, A, false, false);

    compare_result(rank, expected_A, result);

    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> DistMatrix[VC/*] <]

    typedef elem::DistMatrix<double, elem::VC, elem::STAR> vcs_target_t;

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, vcs_target_t> SparseVC(n, n_s, context);

    //[> 2. Create space for the sketched matrix <]
    vcs_target_t sketch_A_vcs(n_s, m, grid);
    elem::Zero(sketch_A_vcs);

    //[> 3. Apply the transform <]
    SparseVC.apply(A, sketch_A_vcs, skylark::sketch::columnwise_tag());

    //[> 4. Build structure to compare <]
    // easier to check if all processors own result
    result = sketch_A_vcs;

    compute_sketch_matrix(SparseVC, A, pi_sketch);
    expected_A = Mult_AnXBn_Synch<PTDD, double, col_t>(
            pi_sketch, A, false, false);

    compare_result(rank, expected_A, result);

    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> DistMatrix[*/VR] <]

    typedef elem::DistMatrix<double, elem::STAR, elem::VR> svr_target_t;

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, svr_target_t> SparseVR(n, n_s, context);

    //[> 2. Create space for the sketched matrix <]
    svr_target_t sketch_A_svr(n_s, m, grid);
    elem::Zero(sketch_A_svr);

    //[> 3. Apply the transform <]
    SparseVR.apply(A, sketch_A_svr, skylark::sketch::columnwise_tag());

    //[> 4. Build structure to compare <]
    // easier to check if all processors own result
    result = sketch_A_svr;

    compute_sketch_matrix(SparseVR, A, pi_sketch);
    expected_A = Mult_AnXBn_Synch<PTDD, double, col_t>(
            pi_sketch, A, false, false);

    compare_result(rank, expected_A, result);

    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> DistMatrix[*/*] <]

    typedef elem::DistMatrix<double, elem::STAR, elem::STAR> st_target_t;

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, st_target_t> SparseST(n, n_s, context);

    //[> 2. Create space for the sketched matrix <]
    st_target_t sketch_A_st(n_s, m, grid);
    elem::Zero(sketch_A_st);

    //[> 3. Apply the transform <]
    SparseST.apply(A, sketch_A_st, skylark::sketch::columnwise_tag());

    //[> 4. Compare <]
    compute_sketch_matrix(SparseST, A, pi_sketch);
    expected_A = Mult_AnXBn_Synch<PTDD, double, col_t>(
            pi_sketch, A, false, false);

    compare_result(rank, expected_A, sketch_A_st);

    //////////////////////////////////////////////////////////////////////////
    //[> Column wise application DistSparseMatrix -> LocalDenseMatrix <]

    Dummy_t<DistMatrixType, elem::Matrix<double>> LocalSparse(n, n_s, context);
    elem::Matrix<double> local_sketch_A(n_s, m);
    elem::Zero(local_sketch_A);
    LocalSparse.apply(A, local_sketch_A, skylark::sketch::columnwise_tag());

    elem::Matrix<double> pi_sketch_l(n_s, n);
    elem::Zero(pi_sketch_l);
    elem::Matrix<double> expected_A_l(n_s, m);
    elem::Zero(expected_A_l);

    if(rank == 0) {
        // PI generated by random number gen
        std::vector<size_t> row_idx = LocalSparse.getRowIdx();
        std::vector<double> row_val = LocalSparse.getRowValues();

        int sketch_size = row_val.size();
        typename LocalMatrixType::coords_t coords;

        for(int i = 0; i < sketch_size; ++i)
            pi_sketch_l.Set(row_idx[i], i, row_val[i]);

        elem::Gemm(elem::NORMAL, elem::NORMAL, 1.0, pi_sketch_l, local_A,
                   0.0, expected_A_l);

        for(int col = 0; col < expected_A_l.Width(); col++) {
            for(int row = 0; row < expected_A_l.Height(); row++) {
                if(local_sketch_A.Get(row, col) !=
                        expected_A_l.Get(row, col))
                    BOOST_FAIL("Result of local colwise application not as expected");
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    //[> Row wise application DistSparseMatrix -> DistMatrix[MC/MR] <]

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, mcmr_target_t> Sparse_r(m, m_s, context);

    //[> 2. Create space for the sketched matrix <]
    mcmr_target_t sketch_A_r(n, m_s, grid);
    elem::Zero(sketch_A_r);

    //[> 3. Apply the transform <]
    Sparse_r.apply(A, sketch_A_r, skylark::sketch::rowwise_tag());

    //[> 4. Build structure to compare <]
    // easier to check if all processors own result
    result = sketch_A_r;

    compute_sketch_matrix(Sparse_r, A, pi_sketch_r);
    pi_sketch_r.Transpose();
    expected_AR = Mult_AnXBn_Synch<PTDD, double, col_t>(
            A, pi_sketch_r, false, false);

    compare_result(rank, expected_AR, result);

    //////////////////////////////////////////////////////////////////////////
    //[> Row wise application DistSparseMatrix -> DistMatrix[VC/*] <]

    //[> 1. Create the sketching matrix <]
    Dummy_t<DistMatrixType, vcs_target_t> Sparse_r_vcs(m, m_s, context);

    //[> 2. Create space for the sketched matrix <]
    vcs_target_t sketch_A_r_vcs(n, m_s, grid);
    elem::Zero(sketch_A_r_vcs);

    //[> 3. Apply the transform <]
    Sparse_r_vcs.apply(A, sketch_A_r_vcs, skylark::sketch::rowwise_tag());

    //[> 4. Build structure to compare <]
    // easier to check if all processors own result
    result = sketch_A_r_vcs;

    pi_sketch_r.Transpose();
    compute_sketch_matrix(Sparse_r_vcs, A, pi_sketch_r);
    pi_sketch_r.Transpose();
    expected_AR = Mult_AnXBn_Synch<PTDD, double, col_t>(
            A, pi_sketch_r, false, false);

    compare_result(rank, expected_AR, result);

    //////////////////////////////////////////////////////////////////////////
    //[> Row wise application DistSparseMatrix -> LocalDenseMatrix <]

    Dummy_t<DistMatrixType, elem::Matrix<double>> LocalSparse_r(m, m_s, context);
    elem::Matrix<double> local_sketch_A_r(n, m_s);
    elem::Zero(local_sketch_A_r);
    LocalSparse_r.apply(A, local_sketch_A_r, skylark::sketch::rowwise_tag());

    elem::Matrix<double> local_pi_sketch_r(m_s, m);
    elem::Zero(local_pi_sketch_r);
    elem::Matrix<double> expected_A_r(n, m_s);
    elem::Zero(expected_A_r);

    if(rank == 0) {
        // PI generated by random number gen
        std::vector<size_t> row_idx = LocalSparse_r.getRowIdx();
        std::vector<double> row_val = LocalSparse_r.getRowValues();

        int sketch_size = row_val.size();
        typename LocalMatrixType::coords_t coords;

        for(int i = 0; i < sketch_size; ++i)
            local_pi_sketch_r.Set(row_idx[i], i, row_val[i]);

        elem::Gemm(elem::NORMAL, elem::TRANSPOSE, 1.0, local_A, local_pi_sketch_r,
                   0.0, expected_A_r);

        for(int col = 0; col < expected_A_r.Width(); col++) {
            for(int row = 0; row < expected_A_r.Height(); row++) {
                if(local_sketch_A_r.Get(row, col) !=
                        expected_A_r.Get(row, col))
                    BOOST_FAIL("Result of local rowwise application not as expected");
            }
        }
    }

    return 0;
}
