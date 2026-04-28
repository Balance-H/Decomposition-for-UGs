//#define _CRTDBG_MAP_ALLOC //用于内存泄露检测
//#include <crtdbg.h>

#include <igraph.h>
#include <stdio.h>

/**
 * This function implements the maximum cardinality search plus algorithm.
 * It computes minimal chordal completions of graphs.
 *
 * </para><para>
 * References:
 *
 * </para><para>
 * Anne Berry, Jean R. S. Blair, Pinar Heggernes and Barry W. Peyton:
 * Maximum Cardinality Search for Computing Minimal Triangulations of Graphs.
 * Algorithmica 39, 287–298 (2004)
 * https://doi.org/10.1007/s00453-004-1084-3
 * 
 * </para><para>
 * Anne Berry, Romain Pogorelcnik and Geneviève Simonet:
 * An Introduction to Clique Minimal Separator Decomposition.
 * Algorithms 2010, 3(2), 197-215
 * https://doi.org/10.3390/a3020197
 *
 * \param g The input graph. Edge directions will be ignored.
 * \param alpha A minimal elimination ordering on the vertex set.
 * \param minimal_chordal A minimal chordal completion of graph.
 * * \param min_sep_gen The set of vertices which generate a minimal separator of minimal_chordal.
 *
 */

void mcs_m_plus(const igraph_t *g,
                igraph_vector_int_t *alpha,
                igraph_vector_int_t *min_sep_gen,
                igraph_t *minimal_chordal) {

    const igraph_integer_t n = igraph_vcount(g);

    // 初始化 minimal_chordal 和 alpha / min_sep_gen
    igraph_copy(minimal_chordal, g);
    igraph_vector_int_resize(alpha, n);
    igraph_vector_int_clear(min_sep_gen);

    // 存储填充边
    igraph_vector_int_t F;
    igraph_vector_int_init(&F, 0);

    igraph_integer_t s = -1;

    // 顶点标签，初始化为0
    igraph_vector_int_t label;
    igraph_vector_int_init(&label, n);

    igraph_integer_t x, y, z, min_label, size_Y, size_Z, index;

    // 提前初始化 Y、Z、reached
    igraph_vector_int_t Y, Z, reached;
    igraph_vector_int_init(&Y, 0);
    igraph_vector_int_init(&Z, 0);
    igraph_vector_int_init(&reached, n);

    // 提前初始化 reach list
    igraph_vector_int_list_t reach;
    igraph_vector_int_list_init(&reach, n);
    for (igraph_integer_t j = 0; j < n; j++)
        igraph_vector_int_init(igraph_vector_int_list_get_ptr(&reach, j), 0);

    igraph_bool_t x_y_adj;

    for (igraph_integer_t i = n - 1; i >= 0; i--) {
        // 选择最大 label 顶点 x
        igraph_vector_int_which_minmax(&label, &min_label, &x);

        // Y <- N_g(x) 并剔除已选顶点
        igraph_vector_int_clear(&Y);
        igraph_neighbors(g, &Y, x, IGRAPH_ALL);
        size_Y = igraph_vector_int_size(&Y);
        index = 0;
        for (igraph_integer_t j = 0; j < size_Y; j++) {
            if (VECTOR(label)[VECTOR(Y)[j]] > -1) {
                VECTOR(Y)[index++] = VECTOR(Y)[j];
            }
        }
        igraph_vector_int_resize(&Y, index);

        // 更新最小分离子集合
        if (VECTOR(label)[x] <= s)
            igraph_vector_int_push_back(min_sep_gen, x);

        s = VECTOR(label)[x];

        // 重置 reached
        for (igraph_integer_t j = 0; j < n; j++)
            VECTOR(reached)[j] = 0;
        VECTOR(reached)[x] = 1;

        // 重置 reach list
        for (igraph_integer_t j = 0; j < n; j++)
            igraph_vector_int_clear(igraph_vector_int_list_get_ptr(&reach, j));

        // 标记 x 的邻居
        size_Y = igraph_vector_int_size(&Y);
        for (igraph_integer_t j = 0; j < size_Y; j++) {
            y = VECTOR(Y)[j];
            VECTOR(reached)[y] = 1;
            igraph_vector_int_push_back(igraph_vector_int_list_get_ptr(&reach, VECTOR(label)[y]), y);
        }

        // BFS 遍历
        for (igraph_integer_t j = 0; j < n; j++) {
            while (igraph_vector_int_size(igraph_vector_int_list_get_ptr(&reach, j)) > 0) {
                y = igraph_vector_int_pop_back(igraph_vector_int_list_get_ptr(&reach, j));

                igraph_vector_int_clear(&Z);
                igraph_neighbors(g, &Z, y, IGRAPH_ALL);
                size_Z = igraph_vector_int_size(&Z);

                for (igraph_integer_t k = 0; k < size_Z; k++) {
                    z = VECTOR(Z)[k];
                    if (VECTOR(label)[z] > -1 && VECTOR(reached)[z] == 0) {
                        VECTOR(reached)[z] = 1;
                        if (VECTOR(label)[z] > j) {
                            igraph_vector_int_push_back(&Y, z);
                            igraph_vector_int_push_back(igraph_vector_int_list_get_ptr(&reach, VECTOR(label)[z]), z);
                        } else {
                            igraph_vector_int_push_back(igraph_vector_int_list_get_ptr(&reach, j), z);
                        }
                    }
                }
            }
        }

        // 填充边和 label 更新
        size_Y = igraph_vector_int_size(&Y);
        for (igraph_integer_t j = 0; j < size_Y; j++) {
            y = VECTOR(Y)[j];
            igraph_are_adjacent(g, x, y, &x_y_adj);
            if (!x_y_adj) {
                igraph_vector_int_push_back(&F, x);
                igraph_vector_int_push_back(&F, y);
            }
            VECTOR(label)[y]++;
        }

        VECTOR(*alpha)[i] = x;
        VECTOR(label)[x] = -1;
    }

    igraph_add_edges(minimal_chordal, &F, NULL);

    // 统一释放
    igraph_vector_int_destroy(&Y);
    igraph_vector_int_destroy(&Z);
    igraph_vector_int_destroy(&reached);
    for (igraph_integer_t j = 0; j < n; j++)
        igraph_vector_int_destroy(igraph_vector_int_list_get_ptr(&reach, j));
    igraph_vector_int_list_destroy(&reach);
    igraph_vector_int_destroy(&F);
    igraph_vector_int_destroy(&label);
}


/**
 * This function implements Atoms algorithm.
 * It computes the atoms and minimal separators of graphs.
 *
 * </para><para>
 * References:
 *
 * </para><para>
 * Anne Berry, Romain Pogorelcnik and Geneviève Simonet:
 * An Introduction to Clique Minimal Separator Decomposition.
 * Algorithms 2010, 3(2), 197-215
 * https://doi.org/10.3390/a3020197
 *
 * \param g The input graph. Edge directions will be ignored.
 * \param A The set of atoms of g.
 * \param S_C The set of cliques minimal seaparators of g. 
 * */

void atoms(const igraph_t *g,
           igraph_vector_int_list_t *A,
           igraph_vector_int_list_t *S_C) {

    const igraph_integer_t n = igraph_vcount(g);

    igraph_t minimal_chordal;
    igraph_vector_int_t alpha, min_sep_gen;

    igraph_vector_int_init(&alpha, n);
    igraph_vector_int_init(&min_sep_gen, 0);
    igraph_vector_int_list_init(A, 0);
    igraph_vector_int_list_init(S_C, 0);

    // minimal chordal completion
    mcs_m_plus(g, &alpha, &min_sep_gen, &minimal_chordal);

    igraph_vector_int_t label;
    igraph_vector_int_init(&label, n);

    igraph_t g1;
    igraph_copy(&g1, g);

    igraph_vector_int_t g1_v;
    igraph_vector_int_init_range(&g1_v, 0, n);

    // 临时向量统一初始化，循环中 clear
    igraph_vector_int_t S, S_copy, C, g1_minus_S_v, g1_minus_C_v, map, invmap;
    igraph_vector_int_init(&S, 0);
    igraph_vector_int_init(&S_copy, 0);
    igraph_vector_int_init(&C, 0);
    igraph_vector_int_init(&g1_minus_S_v, 0);
    igraph_vector_int_init(&g1_minus_C_v, 0);
    igraph_vector_int_init(&map, 0);
    igraph_vector_int_init(&invmap, 0);

    igraph_integer_t x, size_S, size_C, index;
    igraph_bool_t S_is_clique;
    igraph_vs_t S_vids, g1_minus_S_vids, g1_minus_C_vids;
    igraph_t g1_minus_S, g2;

    for (igraph_integer_t i = 0; i < n; i++) {
        x = VECTOR(alpha)[i];

        if (igraph_vector_int_contains(&min_sep_gen, x)) {

            igraph_vector_int_clear(&S);
            igraph_neighbors(&minimal_chordal, &S, x, IGRAPH_ALL);

            // 过滤已标记顶点
            size_S = igraph_vector_int_size(&S);
            index = 0;
            for (igraph_integer_t j = 0; j < size_S; j++) {
                if (VECTOR(label)[VECTOR(S)[j]] == 0) {
                    VECTOR(S)[index++] = VECTOR(S)[j];
                }
            }
            igraph_vector_int_resize(&S, index);

            igraph_vector_int_clear(&S_copy);
            igraph_vector_int_append(&S_copy, &S);


            igraph_vs_vector(&S_vids, &S_copy);
            igraph_is_clique(g, S_vids, 0, &S_is_clique);
            igraph_vs_destroy(&S_vids);

            if (S_is_clique) {

                if (igraph_vector_int_size(&S_copy) > 0) {
                    igraph_vector_int_t tmp_S;
                    igraph_vector_int_init_copy(&tmp_S, &S_copy);
                    igraph_vector_int_list_push_back(S_C, &tmp_S);
                }

                // g1_minus_S 子图
                igraph_vector_int_clear(&g1_minus_S_v);
                igraph_vector_int_difference_sorted(&g1_v, &S, &g1_minus_S_v);

                igraph_vs_vector(&g1_minus_S_vids, &g1_minus_S_v);
                igraph_vector_int_clear(&map);
                igraph_vector_int_clear(&invmap);
                igraph_induced_subgraph_map(g, &g1_minus_S, g1_minus_S_vids,
                                            IGRAPH_SUBGRAPH_AUTO, &map, &invmap);
                igraph_vs_destroy(&g1_minus_S_vids);

                igraph_vector_int_clear(&C);
                igraph_subcomponent(&g1_minus_S, &C, VECTOR(map)[x] - 1, IGRAPH_ALL);

                size_C = igraph_vector_int_size(&C);
                for (igraph_integer_t k = 0; k < size_C; k++)
                    VECTOR(C)[k] = VECTOR(invmap)[VECTOR(C)[k]];
                igraph_vector_int_sort(&C);

                igraph_vector_int_append(&S, &C);

                // push_back deep copy
                igraph_vector_int_t tmp_A;
                igraph_vector_int_init_copy(&tmp_A, &S);
                igraph_vector_int_list_push_back(A, &tmp_A);

                // g1_minus_C 子图
                igraph_vector_int_clear(&g1_minus_C_v);
                igraph_vector_int_difference_sorted(&g1_v, &C, &g1_minus_C_v);

                igraph_vs_vector(&g1_minus_C_vids, &g1_minus_C_v);
                igraph_induced_subgraph_map(g, &g2, g1_minus_C_vids,
                                            IGRAPH_SUBGRAPH_AUTO, &map, &invmap);
                igraph_vs_destroy(&g1_minus_C_vids);

                // 释放上一个 g1
                igraph_destroy(&g1);
                g1 = g2;  // g2 成为新的 g1
                // g2 不再单独 destroy，因为已转移给 g1

                igraph_vector_int_clear(&g1_v);
                igraph_vector_int_append(&g1_v, &g1_minus_C_v);


                // 释放 g1_minus_S 子图
                igraph_destroy(&g1_minus_S);
            }
        }

        VECTOR(label)[x] = 1;
    }

    // 最后把剩余 g1_v 放入 A
    igraph_vector_int_t tmp_g1v;
    igraph_vector_int_init_copy(&tmp_g1v, &g1_v);
    igraph_vector_int_list_push_back(A, &tmp_g1v);

    // 销毁临时向量和图
    igraph_vector_int_destroy(&S);
    igraph_vector_int_destroy(&S_copy);
    igraph_vector_int_destroy(&C);
    igraph_vector_int_destroy(&g1_minus_S_v);
    igraph_vector_int_destroy(&g1_minus_C_v);
    igraph_vector_int_destroy(&map);
    igraph_vector_int_destroy(&invmap);

    igraph_vector_int_destroy(&g1_v);
    igraph_vector_int_destroy(&label);
    igraph_vector_int_destroy(&alpha);
    igraph_vector_int_destroy(&min_sep_gen);
    igraph_destroy(&minimal_chordal);
    igraph_destroy(&g1);

    // 注意：A 和 S_C 的内存由外层调用者负责销毁
}
