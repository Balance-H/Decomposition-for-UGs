#ifndef DECOM_MATRIX_H
#define DECOM_MATRIX_H

#include <igraph.h>

#ifdef __cplusplus
extern "C" {
#endif

// minimal triangulation，返回边列表
igraph_error_t minimal_triangulation_fmt(const igraph_t* G, igraph_vector_int_t* edges_out);

// clique-minimal separators，返回 igraph_vector_ptr_t，每个元素是 igraph_vector_int_t* 表示一个 separator
igraph_error_t clique_minimal_separators(const igraph_t* G, igraph_vector_ptr_t* clique_separators);

#ifdef __cplusplus
}
#endif

#endif // DECOM_MATRIX_H
