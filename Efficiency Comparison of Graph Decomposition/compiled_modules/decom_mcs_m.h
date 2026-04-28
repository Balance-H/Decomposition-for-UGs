#ifndef DECOM_MCS_M
#define DECOM_MCS_M

#include <igraph.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute minimal chordal completion using Maximum Cardinality Search Plus (MCS-M+)
 * 
 * @param g Input graph (const)
 * @param alpha Output vector storing vertex ordering
 * @param min_sep_gen Output vector storing minimal separators
 * @param minimal_chordal Output graph after adding fill-in edges
 */
void mcs_m_plus(const igraph_t *g,
                igraph_vector_int_t *alpha,
                igraph_vector_int_t *min_sep_gen,
                igraph_t *minimal_chordal);


/**
 * @brief Compute the atoms (decomposition components) of a graph
 * 
 * @param g Input graph (const)
 * @param A Output list of atoms (vector_int_list)
 * @param S_C Output list of clique separators (vector_int_list)
 */
void atoms(const igraph_t *g,
           igraph_vector_int_list_t *A,
           igraph_vector_int_list_t *S_C);

#ifdef __cplusplus
}
#endif

#endif // DECOM_MCS_M
