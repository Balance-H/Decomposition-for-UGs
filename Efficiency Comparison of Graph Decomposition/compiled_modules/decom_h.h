#ifndef DECOM_H_H
#define DECOM_H_H


#include <igraph.h>

#ifdef __cplusplus
extern "C" {
#endif

igraph_error_t components_forbidden(
    const igraph_t *graph,
    igraph_vector_ptr_t *components,
    igraph_vector_ptr_t *boundaries,
    const igraph_vector_int_t *forbidden_vertices
);

// 修改后的 igraph_subcomponent_forbidden 函数声明
igraph_error_t close_separator(
    const igraph_t *graph,
    igraph_integer_t vertex,
    const igraph_vector_int_t *forbidden_vertices,
    igraph_vector_int_t *bound_b  // 用于存储遇到的禁忌节点
);

igraph_error_t find_convex_hull(
    const igraph_t *graph,
    const igraph_vector_int_t *r_nodes,
    igraph_vector_int_t *H_out,
    const char *method  // 新增参数
);

igraph_error_t recursive_decom(
    const igraph_t *g,
    const char *method,
    igraph_vector_ptr_t *atoms, // 存储 Atoms (A)
    igraph_vector_ptr_t *separators // 存储 Separators (C)
);

igraph_error_t SAHR(
    igraph_t *g, 
    int *r, 
    int r_size, 
    int **local2global, 
    int *result_size
);




#ifdef __cplusplus
}
#endif

#endif // decom_h_H
