//#define _CRTDBG_MAP_ALLOC //用于内存泄露检测
//#include <crtdbg.h>

#include <igraph_types.h>
#include <igraph.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <cblas.h> 如果使用openblas就取消注释
#include <time.h>  


/**
 * @brief Compute the number of non-edges in an undirected graph.
 *
 * @param H Pointer to the igraph_t graph object (should be undirected).
 * @return igraph_integer_t Number of non-edges in the graph.
 *
 * @note For an undirected graph with n vertices, the total possible unordered
 *       vertex pairs is n*(n-1)/2. The number of non-edges equals total pairs minus actual edges.
 */
igraph_integer_t num_nonedges(const igraph_t *H) {
    igraph_integer_t n = igraph_vcount(H);             // number of vertices
    igraph_integer_t m = igraph_ecount(H);             // number of edges
    igraph_integer_t total_pairs = n * (n - 1) / 2;    // total possible unordered vertex pairs
    return total_pairs - m;                            // non-edges = total pairs - existing edges
}


igraph_error_t components_forbidden(
    const igraph_t *graph,
    igraph_vector_ptr_t *components,
    igraph_vector_ptr_t *boundaries,
    const igraph_vector_int_t *forbidden_vertices)
{
    igraph_integer_t n = igraph_vcount(graph);

    // 访问标记数组：0未访问，1已访问, 2是禁忌节点
    char *visited = (char*)calloc(n, sizeof(char));
    if (!visited) {
        return IGRAPH_ENOMEM;
    }

    // 标记禁忌节点为已访问，防止遍历
    if (forbidden_vertices) {
        for (int i = 0; i < igraph_vector_int_size(forbidden_vertices); i++) {
            igraph_integer_t v = VECTOR(*forbidden_vertices)[i];
            visited[v] = 2;
        }
    }

     // 边界节点辅助标记，避免重复加入
    char *boundary_marked = (char*)calloc(n, sizeof(char));
    if (!boundary_marked) {
        free(visited);
        return IGRAPH_ENOMEM;
    }

    // 用于 BFS 的队列（动态数组）
    igraph_integer_t *queue = (igraph_integer_t*)malloc(n * sizeof(igraph_integer_t));
     if (!queue) {
        free(visited);
        free(boundary_marked);
        return IGRAPH_ENOMEM;
    }

    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    for (igraph_integer_t v = 0; v < n; v++) {
        if (visited[v]) continue;  // 已访问或禁忌跳过

        // 新连通分量和边界初始化
        igraph_vector_int_t *comp = (igraph_vector_int_t*)malloc(sizeof(igraph_vector_int_t));
        igraph_vector_int_t *bound = (igraph_vector_int_t*)malloc(sizeof(igraph_vector_int_t));
        if (!comp || !bound) {
            free(visited);
            free(boundary_marked);
            free(queue);
            igraph_vector_int_destroy(&neighbors);
            if (comp) free(comp);
            if (bound) free(bound);
            return IGRAPH_ENOMEM;
        }
        
        igraph_vector_int_init(comp, 0);
        igraph_vector_int_init(bound, 0);

        // BFS 初始化
        int head = 0, tail = 0;
        queue[tail++] = v;
        visited[v] = 1;

        while (head < tail) {
            igraph_integer_t cur = queue[head++];

            igraph_vector_int_push_back(comp, cur);

            IGRAPH_CHECK(igraph_neighbors(graph, &neighbors, cur, IGRAPH_ALL));
            igraph_integer_t nei_count = igraph_vector_int_size(&neighbors);

            for (igraph_integer_t i = 0; i < nei_count; i++) {
                igraph_integer_t w = VECTOR(neighbors)[i];
                if (visited[w] == 2) {
                    // 禁忌节点，加入边界（避免重复）
                    if (!boundary_marked[w]) {
                        igraph_vector_int_push_back(bound, w);
                        boundary_marked[w] = 1;
                    }
                    continue; // 不加入队列
                }

                if (!visited[w]) {
                    visited[w] = 1;
                    queue[tail++] = w;
                }
            }
        }

        igraph_vector_ptr_push_back(components, comp);
        igraph_vector_ptr_push_back(boundaries, bound);

        // 清空边界辅助标记，为下一个连通分量做准备
        igraph_integer_t bsize = igraph_vector_int_size(bound);
        for (igraph_integer_t i = 0; i < bsize; i++) {
            boundary_marked[VECTOR(*bound)[i]] = 0;
        }

        
    }

    free(visited);
    free(boundary_marked);
    free(queue);
    igraph_vector_int_destroy(&neighbors);

    return IGRAPH_SUCCESS;
}

// 工具函数：unique + sort, 去重
int vector_int_unique(igraph_vector_int_t *v) {
    if (igraph_vector_int_size(v) == 0) return IGRAPH_SUCCESS;
    igraph_vector_int_sort(v);
    long int write_pos = 1;
    for (long int i = 1; i < igraph_vector_int_size(v); i++) {
        if (VECTOR(*v)[i] != VECTOR(*v)[i - 1]) {
            VECTOR(*v)[write_pos++] = VECTOR(*v)[i];
        }
    }
    igraph_vector_int_resize(v, write_pos);
    return IGRAPH_SUCCESS;
}



/**
 * @brief Find a full component whose boundary matches P in a graph g.
 *
 * This function calls components_forbidden() to generate components and their
 * boundaries, and returns a copy of the first component whose boundary equals P.
 *
 * @param g Pointer to the igraph_t graph object.
 * @param U Pointer to an igraph_vector_int_t representing the vertex set U (used internally).
 * @param P Pointer to an igraph_vector_int_t representing the boundary P.
 * @return igraph_vector_int_t* Pointer to a new vector containing the component if found, or NULL otherwise.
 */
igraph_vector_int_t* full_components(const igraph_t *g,
                                     const igraph_vector_int_t *U,
                                     const igraph_vector_int_t *P) {
    igraph_vector_ptr_t components, boundaries;
    igraph_vector_ptr_init(&components, 0);
    igraph_vector_ptr_init(&boundaries, 0);

    if (components_forbidden(g, &components, &boundaries, P) != IGRAPH_SUCCESS) {
        igraph_vector_ptr_destroy(&components);
        igraph_vector_ptr_destroy(&boundaries);
        return NULL;
    }

    igraph_vector_int_t *result = NULL;

    long n_boundaries = igraph_vector_ptr_size(&boundaries);
    for (long i = 0; i < n_boundaries; i++) {
        igraph_vector_int_t *bound = (igraph_vector_int_t *)VECTOR(boundaries)[i];
        if (igraph_vector_int_size(bound) == igraph_vector_int_size(P)) {
            igraph_vector_int_t *orig = (igraph_vector_int_t *)VECTOR(components)[i];
            result = malloc(sizeof(*result));
            igraph_vector_int_init(result, igraph_vector_int_size(orig));
            igraph_vector_int_update(result, orig);  // copy component
            break;  // 找到第一个匹配就返回
        }
    }

    // Cleanup
    for (long i = 0; i < n_boundaries; i++) {
        igraph_vector_int_t *bound = (igraph_vector_int_t *)VECTOR(boundaries)[i];
        igraph_vector_int_destroy(bound);
        free(bound);
        igraph_vector_int_t *comp = (igraph_vector_int_t *)VECTOR(components)[i];
        igraph_vector_int_destroy(comp);
        free(comp);
    }
    igraph_vector_ptr_destroy(&components);
    igraph_vector_ptr_destroy(&boundaries);

    return result;
}





/**
 * @brief Compute the closed neighborhood of a vertex set S in graph g.
 *
 * @param g    Pointer to the igraph_t graph object.
 * @param S    Pointer to an igraph_vector_int_t containing vertex IDs (the set S).
 * @param out  Pointer to an igraph_vector_int_t to store the result; will be cleared first.
 * @return int IGRAPH_SUCCESS on success, IGRAPH_ENOMEM if memory allocation fails.
 *
 * @note The result contains no duplicates.
 */
int closed_neigh(const igraph_adjlist_t *adjlist,
                 const igraph_vector_int_t *S,
                 igraph_vector_int_t *out) {
    igraph_vector_int_clear(out);                              // 清空输出向量
    long n = igraph_adjlist_size(adjlist);                     // 顶点总数
    char *seen = calloc(n, sizeof(char));                      // 访问标记
    if (!seen) return IGRAPH_ENOMEM;

    // 先把 S 自身放入 out
    for (long i = 0; i < igraph_vector_int_size(S); i++) {
        igraph_integer_t v = VECTOR(*S)[i];
        if (!seen[v]) {
            igraph_vector_int_push_back(out, v);
            seen[v] = 1;
        }
    }

    // 遍历 S，利用邻接表取邻居
    for (long i = 0; i < igraph_vector_int_size(S); i++) {
        igraph_integer_t v = VECTOR(*S)[i];
        const igraph_vector_int_t *neis = igraph_adjlist_get(adjlist, v);
        for (long j = 0; j < igraph_vector_int_size(neis); j++) {
            igraph_integer_t w = VECTOR(*neis)[j];
            if (!seen[w]) {
                igraph_vector_int_push_back(out, w);
                seen[w] = 1;
            }
        }
    }

    free(seen);                                                // 清理标记
    return IGRAPH_SUCCESS;
}



#define UNMARKED 0
#define S_VERTEX 1
#define C_VERTEX 2
#define P_VERTEX 3

/**
 * @brief Algorithm to partition a graph into components and identify a special vertex set A.
 * 
 * This function implements a two-phase partitioning strategy for an undirected simple graph.
 * Phase I: Define S-vertices, P-vertices, and initial components using incremental complement-degree updates.
 * Phase II: Determine the set A according to four possible cases involving full components, 
 *         non-edges with s-vertices, nonadjacent p-vertices with cross adjacency, or defaulting to P.
 * 
 * @param g       Pointer to an igraph graph object (const igraph_t*)
 * @param A_out   Pointer to an igraph_vector_int_t to store the resulting vertex set A
 * 
 * @return IGRAPH_SUCCESS on success, IGRAPH_ENOMEM on memory allocation failure.
 */
igraph_error_t algorithm_partition(const igraph_t *g, igraph_vector_int_t *A_out) {
    igraph_integer_t n = igraph_vcount(g);

    // ------------------------ Initialize ------------------------
    igraph_vector_int_t U;
    igraph_vector_int_init_range(&U, 0, n);  // 一步生成 0,1,...,n-1

    igraph_integer_t Ebar_H = num_nonedges(g);                    // Number of non-edges in graph
    double threshold = (2.0 / 5.0) * (double)Ebar_H;             // Threshold for S-vertex selection

    // Vertex markers: UNMARKED / S_VERTEX / C_VERTEX / P_VERTEX
    char *mark = calloc(n, sizeof(char));
    int *assoc_comp = calloc(n, sizeof(int));                     // P-vertex -> component ID

    // ------------------------ Component storage ------------------------//heng
    int comp_count = 0;
    igraph_vector_int_list_t comp_members;
    igraph_vector_int_list_init(&comp_members, n);
    for (int i = 0; i < (int)n; i++) {
        igraph_vector_int_init(igraph_vector_int_list_get_ptr(&comp_members, i), 0);
    }

    // ------------------------ Adjacency list ------------------------
    igraph_adjlist_t adjlist;
    IGRAPH_CHECK(igraph_adjlist_init(g, &adjlist, IGRAPH_ALL, IGRAPH_NO_LOOPS, IGRAPH_NO_MULTIPLE));

    // ------------------------ Precompute complement degrees ------------------------ //heng
    igraph_vector_int_t comp_deg_list;
    igraph_vector_int_init(&comp_deg_list, n);
    igraph_degree(g, &comp_deg_list, igraph_vss_all(), IGRAPH_ALL, IGRAPH_LOOPS);
    for (igraph_integer_t v = 0; v < n; v++) {
        VECTOR(comp_deg_list)[v] = (igraph_integer_t)(n - 1 - VECTOR(comp_deg_list)[v]);
    }



    // ======================== Part I: Define P and components ========================
    while (1) {
        // Find the next unmarked vertex u
        igraph_integer_t u = -1;
        for (igraph_integer_t i = 0; i < n; i++) {
            if (mark[i] == UNMARKED) { u = i; break; }
        }
        if (u == -1) break;

        // ------------------------ Build U \ N_H[u] ------------------------
        int *in_UminusNC = calloc(n, sizeof(int)); // Boolean array for vertices in current U \ N_H[u]
        int *in_Nclosed = calloc(n, sizeof(int));  // Temporary closed neighborhood marker

        in_Nclosed[u] = 1;
        const igraph_vector_int_t *neis_u = igraph_adjlist_get(&adjlist, u);
        for (long kk = 0; kk < igraph_vector_int_size(neis_u); kk++) {
            igraph_integer_t w = VECTOR(*neis_u)[kk];
            in_Nclosed[w] = 1;
        }

        igraph_vector_int_t UminusNC_vec;
        igraph_vector_int_init(&UminusNC_vec, 0);
        igraph_integer_t current_sum = 0;
        for (igraph_integer_t v = 0; v < n; v++) {
            if (!in_Nclosed[v]) {
                in_UminusNC[v] = 1;
                igraph_vector_int_push_back(&UminusNC_vec, v);
                current_sum += VECTOR(comp_deg_list)[v];
            } else {
                in_UminusNC[v] = 0;
            }
        }
        free(in_Nclosed);

        // S-vertex condition
        if (current_sum < threshold) {
            mark[u] = S_VERTEX;
            igraph_vector_int_destroy(&UminusNC_vec);
            free(in_UminusNC);
            continue;
        }

        // ------------------------ Start new component C_k ------------------------
        comp_count++;
        igraph_vector_int_push_back(igraph_vector_int_list_get_ptr(&comp_members, comp_count - 1),u);

        mark[u] = C_VERTEX;

        // Candidate set initialization
        int *in_cand = calloc(n, sizeof(int));
        igraph_vector_int_t cand;
        igraph_vector_int_init(&cand, 0);
        igraph_vector_int_t *Ck = igraph_vector_int_list_get_ptr(&comp_members, comp_count - 1);

        for (long i = 0; i < igraph_vector_int_size(Ck); i++) {
            igraph_integer_t vtx = VECTOR(*Ck)[i];
            const igraph_vector_int_t *neis = igraph_adjlist_get(&adjlist, vtx);
            for (long j = 0; j < igraph_vector_int_size(neis); j++) {
                igraph_integer_t w = VECTOR(*neis)[j];
                if ((mark[w] == UNMARKED || mark[w] == S_VERTEX) && !in_cand[w]) {
                    igraph_vector_int_push_back(&cand, w);
                    in_cand[w] = 1;
                }
            }
        }
        //free(in_cand);


        // ------------------------ Incrementally expand component ------------------------
        while (igraph_vector_int_size(&cand) > 0) {
            igraph_integer_t v = igraph_vector_int_pop_back(&cand);

            // Compute sum_after
            igraph_integer_t remove_sum = 0;
            if (in_UminusNC[v]) remove_sum += VECTOR(comp_deg_list)[v];
            const igraph_vector_int_t *neis_v = igraph_adjlist_get(&adjlist, v);
            for (long k = 0; k < igraph_vector_int_size(neis_v); k++) {
                igraph_integer_t w = VECTOR(*neis_v)[k];
                if (in_UminusNC[w]) remove_sum += VECTOR(comp_deg_list)[w];
            }
            igraph_integer_t sum_after = current_sum - remove_sum;

            if (sum_after >= threshold) {
                // Add vertex v to component
                igraph_vector_int_push_back(Ck, v);
                mark[v] = C_VERTEX;

                // Update UminusNC and current sum
                current_sum = sum_after;
                if (in_UminusNC[v]) in_UminusNC[v] = 0;
                for (long k = 0; k < igraph_vector_int_size(neis_v); k++) {
                    igraph_integer_t w = VECTOR(*neis_v)[k];
                    if (in_UminusNC[w]) in_UminusNC[w] = 0;
                }

                // Extend candidate set with neighbors of newly added vertex
                for (long k = 0; k < igraph_vector_int_size(neis_v); k++) {
                    igraph_integer_t w = VECTOR(*neis_v)[k];
                    if ((mark[w] == UNMARKED || mark[w] == S_VERTEX) &&
                        !igraph_vector_int_contains(Ck, w) &&  
                        !in_cand[w]) {                         
                        igraph_vector_int_push_back(&cand, w);
                        in_cand[w] = 1;                  
                    }
                }

            } else {
                // Mark as P-vertex
                mark[v] = P_VERTEX;
                assoc_comp[v] = comp_count;
            }
        }
        free(in_cand);

        // Free per-iteration resources
        igraph_vector_int_destroy(&cand);
        igraph_vector_int_destroy(&UminusNC_vec);
        free(in_UminusNC);
    }

    // ======================== Part II: Determine A ========================
    igraph_vector_int_t s_vertices, p_vertices, P;
    igraph_vector_int_init(&s_vertices, 0);
    igraph_vector_int_init(&p_vertices, 0);
    igraph_vector_int_init(&P, 0);

    for (igraph_integer_t i = 0; i < n; i++) {
        if (mark[i] == S_VERTEX) { igraph_vector_int_push_back(&s_vertices, i); igraph_vector_int_push_back(&P, i); }
        else if (mark[i] == P_VERTEX) { igraph_vector_int_push_back(&p_vertices, i); igraph_vector_int_push_back(&P, i); }
    }

    // ---------------- Case 1: Full component ----------------
    igraph_vector_int_t *C = full_components(g, &U, &P);
    if (C != NULL) {
        igraph_vector_int_clear(A_out);
        for (long i = 0; i < igraph_vector_int_size(&P); i++) igraph_vector_int_push_back(A_out, VECTOR(P)[i]);
        for (long i = 0; i < igraph_vector_int_size(C); i++) {
            igraph_integer_t v = VECTOR(*C)[i];
            igraph_vector_int_push_back(A_out, v); //heng
        }
        igraph_vector_int_destroy(C); free(C);
        igraph_vector_int_destroy(&s_vertices); igraph_vector_int_destroy(&p_vertices); igraph_vector_int_destroy(&P);
        goto cleanup;
    }

    // ---------------- Case 2: Non-edge with S-vertex ----------------
    for (long i = 0; i < igraph_vector_int_size(&s_vertices); i++) {
        igraph_integer_t u = VECTOR(s_vertices)[i];
        for (long j = 0; j < igraph_vector_int_size(&P); j++) {
            igraph_integer_t v = VECTOR(P)[j];
            if (u == v) continue;  // skip self
            igraph_bool_t are_adj = 0;
            IGRAPH_CHECK(igraph_are_adjacent(g, u, v, &are_adj));
            if (!are_adj) {
                // A = closed neighborhood of u
                igraph_vector_int_clear(A_out);
                const igraph_vector_int_t *neis = igraph_adjlist_get(&adjlist, u);
                for (long k = 0; k < igraph_vector_int_size(neis); k++)
                    igraph_vector_int_push_back(A_out, VECTOR(*neis)[k]);
                igraph_vector_int_push_back(A_out, u);

                // Free temporary vectors
                igraph_vector_int_destroy(&s_vertices);
                igraph_vector_int_destroy(&p_vertices);
                igraph_vector_int_destroy(&P);

                goto cleanup;
            }
        }
    }
    // ---------------- Case 3: Nonadjacent p-vertices with cross adjacency ----------------
    // Case 3: nonadjacent p-vertices with cross adjacency (incremental closed neighborhood)
    for (long i = 0; i < igraph_vector_int_size(&p_vertices); i++) {
        igraph_integer_t u = VECTOR(p_vertices)[i];
        for (long j = i + 1; j < igraph_vector_int_size(&p_vertices); j++) {
            igraph_integer_t v = VECTOR(p_vertices)[j];
            igraph_bool_t are_adj = 0; 
            IGRAPH_CHECK(igraph_are_adjacent(g, u, v, &are_adj));
            if (!are_adj) {
                int ci = assoc_comp[u]; 
                int cj = assoc_comp[v];
                igraph_vector_int_t *Ci = igraph_vector_int_list_get_ptr(&comp_members, ci - 1);
                igraph_vector_int_t *Cj = igraph_vector_int_list_get_ptr(&comp_members, cj - 1);


                // Compute closed neighborhoods of components
                igraph_vector_int_t n_h_Cj, n_h_Ci;
                igraph_vector_int_init(&n_h_Cj, 0); 
                igraph_vector_int_init(&n_h_Ci, 0);
                IGRAPH_CHECK(closed_neigh(&adjlist, Cj, &n_h_Cj));
                IGRAPH_CHECK(closed_neigh(&adjlist, Ci, &n_h_Ci));

                // Check cross adjacency condition
                if (!igraph_vector_int_contains(&n_h_Cj, u) && !igraph_vector_int_contains(&n_h_Ci, v)) {
                    // Incrementally update Ci's closed neighborhood with u
                    igraph_vector_int_push_back(&n_h_Ci, u);  // add u itself
                    const igraph_vector_int_t *neis_u = igraph_adjlist_get(&adjlist, u);
                    for (long k = 0; k < igraph_vector_int_size(neis_u); k++) {
                        igraph_integer_t w = VECTOR(*neis_u)[k];
                        if (!igraph_vector_int_contains(&n_h_Ci, w)) {
                            igraph_vector_int_push_back(&n_h_Ci, w); // add neighbors
                        }
                    }

                    // Set A_out to updated closed neighborhood
                    igraph_vector_int_clear(A_out);                 // 先清空目标向量
                    for (long i = 0; i < igraph_vector_int_size(&n_h_Ci); i++) {
                        igraph_vector_int_push_back(A_out, VECTOR(n_h_Ci)[i]);  // 逐个添加元素
                    }


                    // Clean up
                    igraph_vector_int_destroy(&n_h_Cj); 
                    igraph_vector_int_destroy(&n_h_Ci);
                    igraph_vector_int_destroy(&s_vertices); 
                    igraph_vector_int_destroy(&p_vertices); 
                    igraph_vector_int_destroy(&P);
                    goto cleanup;
                }

                igraph_vector_int_destroy(&n_h_Cj); 
                igraph_vector_int_destroy(&n_h_Ci);
            }
        }
    }


    // ---------------- Case 4: Default ----------------
    igraph_vector_int_clear(A_out);                 // 先清空目标向量
    for (long i = 0; i < igraph_vector_int_size(&P); i++) {
        igraph_vector_int_push_back(A_out, VECTOR(P)[i]);  // 逐个添加元素
    }

    igraph_vector_int_destroy(&s_vertices); igraph_vector_int_destroy(&p_vertices); igraph_vector_int_destroy(&P);

cleanup:
    // Free all resources
    igraph_vector_int_list_destroy(&comp_members);
     free(mark); free(assoc_comp); 
    igraph_vector_int_destroy(&comp_deg_list);
    igraph_vector_int_destroy(&U);
    igraph_adjlist_destroy(&adjlist);
    return IGRAPH_SUCCESS;
}





igraph_error_t minimal_triangulation_fmt(const igraph_t* G, igraph_vector_int_t* edges_out) {
    igraph_t Gprime;
    igraph_vector_int_clear(edges_out);
    IGRAPH_CHECK(igraph_copy(&Gprime, G));                 
    const igraph_integer_t n = igraph_vcount(&Gprime);


    // ------------------ Queue initialization ------------------
    igraph_vector_ptr_t Q1, Q2, Q3;
    igraph_vector_ptr_init(&Q1, 0);
    igraph_vector_ptr_init(&Q2, 0);
    igraph_vector_ptr_init(&Q3, 0);

    //clock_t total_blas_time = 0;  // 用于累计矩阵乘法阶段的耗时

    // 初始化 Q1 为全图节点集（0,1,...,n-1）
    igraph_vector_int_t* Vfull = (igraph_vector_int_t*) malloc(sizeof(igraph_vector_int_t));
    igraph_vector_int_init_range(Vfull, 0, n);
    igraph_vector_ptr_push_back(&Q1, Vfull);

    igraph_vector_int_t map, invmap;
    

    // ------------------ Sparse matrix initialization ------------------
    long maxcols = 1024;
    long rows_cap = n * maxcols;
    long* rows   = (long*)   malloc(sizeof(long)   * rows_cap);
    long* cols   = (long*)   malloc(sizeof(long)   * rows_cap);
    double* data = (double*) malloc(sizeof(double) * rows_cap);
    long col_id;


    // ------------------ 主循环 ------------------
    while (igraph_vector_ptr_size(&Q1) > 0) {
        col_id = 0;
        long nnz = 0;


        while (igraph_vector_ptr_size(&Q1) > 0) {

            igraph_vector_int_t* H_nodes = (igraph_vector_int_t*) igraph_vector_ptr_pop_back(&Q1);

            igraph_t H_sub; 
            igraph_vector_int_init(&map, 0);
            igraph_vector_int_init(&invmap, 0);

            // 创建子图
            igraph_vs_t vs;
            igraph_vs_vector(&vs, H_nodes); // H_nodes 是 igraph_vector_int_t*
            IGRAPH_CHECK(igraph_induced_subgraph_map(&Gprime, &H_sub, vs,
                                                    IGRAPH_SUBGRAPH_AUTO, &map, &invmap));
            igraph_vs_destroy(&vs);

            // ------------------ Partitioning ------------------
            igraph_vector_int_t A_local;
            igraph_vector_int_init(&A_local, 0);
            IGRAPH_CHECK(algorithm_partition(&H_sub, &A_local));

            igraph_vector_int_t* A_global = (igraph_vector_int_t*) malloc(sizeof(igraph_vector_int_t));
            igraph_vector_int_init(A_global, igraph_vector_int_size(&A_local));
            long sz = igraph_vector_int_size(&A_local);
            for (long i = 0; i < sz; i++) {
                long local_idx = VECTOR(A_local)[i];
                VECTOR(*A_global)[i] = VECTOR(invmap)[local_idx];
            }

            igraph_vector_ptr_push_back(&Q3, A_global);

            // ------------------ Components forbidden ------------------

            igraph_vector_ptr_t components, boundaries;
            igraph_vector_ptr_init(&components, 0);
            igraph_vector_ptr_init(&boundaries, 0);
            IGRAPH_CHECK(components_forbidden(&H_sub, &components, &boundaries, &A_local));
            igraph_vector_int_destroy(&A_local);


            for (long c = 0; c < (long) igraph_vector_ptr_size(&components); c++) {
                igraph_vector_int_t* C = (igraph_vector_int_t*) VECTOR(components)[c];
                igraph_vector_int_t* N_H_C = (igraph_vector_int_t*) VECTOR(boundaries)[c];

                // ------------------ Construct NClosed ------------------
                igraph_vector_int_t NClosed;
                igraph_vector_int_init(&NClosed, 0);
                for (long ii = 0; ii < igraph_vector_int_size(C); ii++)
                    igraph_vector_int_push_back(&NClosed, VECTOR(*C)[ii]);
                for (long ii = 0; ii < igraph_vector_int_size(N_H_C); ii++)
                    igraph_vector_int_push_back(&NClosed, VECTOR(*N_H_C)[ii]);

                // ------------------ Update triplet matrix ------------------
                for (long ii = 0; ii < igraph_vector_int_size(N_H_C); ii++) {
                    if (nnz >= rows_cap) {
                        rows_cap *= 2;
                        rows = (long*)   realloc(rows, sizeof(long)   * rows_cap);
                        cols = (long*)   realloc(cols, sizeof(long)   * rows_cap);
                        data = (double*) realloc(data, sizeof(double) * rows_cap);
                    }
                    rows[nnz] = VECTOR(invmap)[ VECTOR(*N_H_C)[ii]];
                    cols[nnz] = col_id;
                    data[nnz] = 1.0;
                    nnz++;
                }
                col_id++;

                // ------------------ Check non-edges ------------------
                if (igraph_vector_int_size(&NClosed) >= 2) {
                    int found_nonedge = 0;
                    igraph_vs_t vs;
                    igraph_bool_t is_clique;
                    igraph_vs_vector(&vs, C);
                    igraph_is_clique(&H_sub, vs, 0, &is_clique);
                    igraph_vs_destroy(&vs);

                    if (!is_clique) found_nonedge = 1;
                    for (long ii = 0; ii < igraph_vector_int_size(C) && !found_nonedge; ++ii) {
                        igraph_integer_t u = VECTOR(*C)[ii];
                        for (long jj = 0; jj < igraph_vector_int_size(N_H_C); ++jj) {
                            igraph_integer_t v = VECTOR(*N_H_C)[jj];
                            igraph_bool_t are_adj = 0;
                            IGRAPH_CHECK(igraph_are_adjacent(&H_sub, u, v, &are_adj));
                            if (!are_adj) { found_nonedge = 1; break; }
                        }
                    }

                    if (found_nonedge) {
                        

                        long sz = igraph_vector_int_size(&NClosed);
                        igraph_vector_int_t* NClosed_global = (igraph_vector_int_t*) malloc(sizeof(igraph_vector_int_t));
                        igraph_vector_int_init(NClosed_global, sz);

                        for (long i = 0; i < sz; i++) {
                            long local_idx = VECTOR(NClosed)[i];      // H_sub 的局部编号
                            VECTOR(*NClosed_global)[i] = VECTOR(invmap)[local_idx]; // 映射到 Gprime 的全局编号
                        }

                        igraph_vector_ptr_push_back(&Q2, NClosed_global);
                    }
                }

                igraph_vector_int_destroy(&NClosed);
            }

            // ------------------ Clean up components ------------------
            for (long ii = 0; ii < (long) igraph_vector_ptr_size(&components); ii++) {
                igraph_vector_int_t *comp = (igraph_vector_int_t*) VECTOR(components)[ii];
                if (comp) {
                    igraph_vector_int_destroy(comp);
                    free(comp);
                }
            }
            for (long ii = 0; ii < (long) igraph_vector_ptr_size(&boundaries); ii++) {
                igraph_vector_int_t *bound = (igraph_vector_int_t*) VECTOR(boundaries)[ii];
                if (bound) {
                    igraph_vector_int_destroy(bound);
                    free(bound);
                }
            }
            igraph_vector_ptr_destroy(&components);
            igraph_vector_ptr_destroy(&boundaries);

            igraph_vector_int_destroy(&map);
            igraph_vector_int_destroy(&invmap);
            igraph_destroy(&H_sub);
            igraph_vector_int_destroy(H_nodes);
            free(H_nodes);
        }
        // ------------------ BLAS triplet ------------------
        if (col_id > 0) {
            //clock_t t1 = clock();  // 计时开始
            float* M = (float*) calloc((size_t)n * (size_t)col_id, sizeof(float));
            for (long ii = 0; ii < nnz; ii++) {
                M[(size_t)rows[ii]*(size_t)col_id + (size_t)cols[ii]] = (float)data[ii];
            }

            float* MMt = (float*) calloc((size_t)n * (size_t)n, sizeof(float));

            // 稀疏列组合法计算 MMt = M * M^T
            for (long k = 0; k < col_id; k++) {
                long start_idx = -1, end_idx = -1;
                for (long idx = 0; idx < nnz; idx++) {
                    if (cols[idx] == k) {
                        if (start_idx == -1) start_idx = idx;
                        end_idx = idx;
                    }
                }
                if (start_idx == -1) continue;
                for (long a = start_idx; a <= end_idx; a++) {
                    long ii = rows[a];
                    float val_i = (float) data[a];
                    for (long b = a; b <= end_idx; b++) {
                        long jj = rows[b];
                        float val_j = (float) data[b];
                        MMt[(size_t)ii * (size_t)n + (size_t)jj] += val_i * val_j;
                        if (ii != jj)
                            MMt[(size_t)jj * (size_t)n + (size_t)ii] = MMt[(size_t)ii * (size_t)n + (size_t)jj];
                    }
                }
            }
            //clock_t t2 = clock();  // 计时结束
            //total_blas_time += (t2 - t1);  // 累计矩阵运算时间

            igraph_vector_int_t new_edges;
            igraph_vector_int_init(&new_edges, 0);

            for (long ii = 0; ii < n; ii++) {
                for (long jj = ii + 1; jj < n; jj++) {
                    if (MMt[(size_t)ii * (size_t)n + (size_t)jj] > 0) {
                        igraph_bool_t adjacent = 0;
                        IGRAPH_CHECK(igraph_are_adjacent(&Gprime, (igraph_integer_t)ii, (igraph_integer_t)jj, &adjacent));
                        if (!adjacent) {
                            igraph_vector_int_push_back(&new_edges, (igraph_integer_t)ii);
                            igraph_vector_int_push_back(&new_edges, (igraph_integer_t)jj);
                        }
                    }
                }
            }

            if (igraph_vector_int_size(&new_edges) > 0) {
                IGRAPH_CHECK(igraph_add_edges(&Gprime, &new_edges, NULL));
            }

            igraph_vector_int_append(edges_out, &new_edges);
            igraph_vector_int_destroy(&new_edges);

            free(M);
            free(MMt);

            
        }

        // ------------------ Process Q3 ------------------
        while (igraph_vector_ptr_size(&Q3) > 0) {
            igraph_vector_int_t* A_nodes = (igraph_vector_int_t*) igraph_vector_ptr_pop_back(&Q3);
            long sz = igraph_vector_int_size(A_nodes);

            if (sz >= 2) {
                igraph_vs_t vs;
                igraph_bool_t is_clique;
                igraph_vs_vector(&vs, A_nodes);
                igraph_is_clique(&Gprime, vs, 0, &is_clique);
                igraph_vs_destroy(&vs);

                if (!is_clique) {

                    igraph_vector_int_t* A_copy = (igraph_vector_int_t*) malloc(sizeof(igraph_vector_int_t));
                    igraph_vector_int_init_copy(A_copy, A_nodes);
                    igraph_vector_ptr_push_back(&Q2, A_copy);
                }
            }
            igraph_vector_int_destroy(A_nodes);
            free(A_nodes);
        }


        // ------------------ Swap Q1/Q2 ------------------
        {
            igraph_vector_ptr_t tmp = Q1;
            Q1 = Q2;
            Q2 = tmp;
            igraph_vector_ptr_destroy(&Q3);
            igraph_vector_ptr_init(&Q3, 0);
        }
    }

    // ------------------ Cleanup ------------------
    igraph_destroy(&Gprime);
    free(rows);
    free(cols);
    free(data);

    // 遍历 Q1/Q2/Q3，释放剩余堆对象
    for (long i = 0; i < igraph_vector_ptr_size(&Q1); i++) {
        igraph_vector_int_t* v = (igraph_vector_int_t*) VECTOR(Q1)[i];
        igraph_vector_int_destroy(v); free(v);
    }
    for (long i = 0; i < igraph_vector_ptr_size(&Q2); i++) {
        igraph_vector_int_t* v = (igraph_vector_int_t*) VECTOR(Q2)[i];
        igraph_vector_int_destroy(v); free(v);
    }
    for (long i = 0; i < igraph_vector_ptr_size(&Q3); i++) {
        igraph_vector_int_t* v = (igraph_vector_int_t*) VECTOR(Q3)[i];
        igraph_vector_int_destroy(v); free(v);
    }

    igraph_vector_ptr_destroy(&Q1);
    igraph_vector_ptr_destroy(&Q2);
    igraph_vector_ptr_destroy(&Q3);



    //double elapsed_sec = (double)total_blas_time / CLOCKS_PER_SEC;
    //printf("[FMT] Total matrix multiplication time: %.4f seconds\n", elapsed_sec);

    return IGRAPH_SUCCESS;
}





//注意我们允许分离子重复
igraph_error_t MCS_Minseps(const igraph_t* g, igraph_vector_ptr_t* separators) {
    igraph_integer_t no_of_nodes, i, j, s;
    igraph_integer_t v, x, k, len;
    igraph_integer_t w, ws, nw, pw;
    igraph_vector_int_t size, head, next, prev;
    igraph_adjlist_t adjlist;
    igraph_vector_int_t *neis;

    no_of_nodes = igraph_vcount(g);
    if (no_of_nodes == 0) {
        igraph_vector_ptr_clear(separators);
        return IGRAPH_SUCCESS;
    }

    IGRAPH_VECTOR_INT_INIT_FINALLY(&size, no_of_nodes);
    IGRAPH_VECTOR_INT_INIT_FINALLY(&head, no_of_nodes);
    IGRAPH_VECTOR_INT_INIT_FINALLY(&next, no_of_nodes);
    IGRAPH_VECTOR_INT_INIT_FINALLY(&prev, no_of_nodes);

    IGRAPH_CHECK(igraph_adjlist_init(g, &adjlist, IGRAPH_ALL, IGRAPH_NO_LOOPS, IGRAPH_NO_MULTIPLE));
    IGRAPH_FINALLY(igraph_adjlist_destroy, &adjlist);

    VECTOR(head)[0] = 1;
    for (v = 0; v < no_of_nodes; v++) {
        VECTOR(next)[v] = v + 2;
        VECTOR(prev)[v] = v;
        VECTOR(size)[v] = 0;
    }
    VECTOR(next)[no_of_nodes - 1] = 0;

    i = no_of_nodes;
    j = 0;
    s = -1;

    while (i >= 1) {
        v = VECTOR(head)[j] - 1;
        x = VECTOR(next)[v];
        VECTOR(head)[j] = x;
        if (x != 0) VECTOR(prev)[x - 1] = 0;

        VECTOR(size)[v] = -1;

        if (j <= s) {
            igraph_vector_int_t* sep = (igraph_vector_int_t*) malloc(sizeof(igraph_vector_int_t));
            igraph_vector_int_init(sep, 0);

            neis = igraph_adjlist_get(&adjlist, v);
            len = igraph_vector_int_size(neis);
            for (k = 0; k < len; k++) {
                w = VECTOR(*neis)[k];
                if (VECTOR(size)[w] == -1)
                    igraph_vector_int_push_back(sep, w);
            }
            igraph_vector_ptr_push_back(separators, sep);
        }

        neis = igraph_adjlist_get(&adjlist, v);
        len = igraph_vector_int_size(neis);
        for (k = 0; k < len; k++) {
            w = VECTOR(*neis)[k];
            ws = VECTOR(size)[w];
            if (ws >= 0) {
                nw = VECTOR(next)[w];
                pw = VECTOR(prev)[w];
                if (nw != 0) VECTOR(prev)[nw - 1] = pw;
                if (pw != 0) VECTOR(next)[pw - 1] = nw;
                else VECTOR(head)[ws] = nw;

                VECTOR(size)[w] += 1;

                ws = VECTOR(size)[w];
                nw = VECTOR(head)[ws];
                VECTOR(next)[w] = nw;
                VECTOR(prev)[w] = 0;
                if (nw != 0) VECTOR(prev)[nw - 1] = w + 1;
                VECTOR(head)[ws] = w + 1;
            }
        }

        i -= 1;
        j += 1;
        s = j - 1;

        if (j < no_of_nodes) while (j >= 0 && VECTOR(head)[j] == 0) j--;
    }

    igraph_adjlist_destroy(&adjlist);
    igraph_vector_int_destroy(&prev);
    igraph_vector_int_destroy(&next);
    igraph_vector_int_destroy(&head);
    igraph_vector_int_destroy(&size);
    IGRAPH_FINALLY_CLEAN(5);

    return IGRAPH_SUCCESS;
}


igraph_error_t clique_minimal_separators(const igraph_t* G,
                                         igraph_vector_ptr_t* clique_separators) {
    long n = igraph_vcount(G);

    // -------- Step 1: 最小三角化 --------
    //clock_t t1 = clock();
    igraph_vector_int_t fill_edges;
    igraph_vector_int_init(&fill_edges, 0);
    IGRAPH_CHECK(minimal_triangulation_fmt(G, &fill_edges));
    //clock_t t2 = clock();
    //printf("[TIMING] Step 1 (minimal triangulation): %.4f sec\n",(double)(t2 - t1) / CLOCKS_PER_SEC);


    // 构造 G_tri
    //t1 = clock();
    igraph_t G_tri;
    IGRAPH_CHECK(igraph_copy(&G_tri, G));
    IGRAPH_CHECK(igraph_add_edges(&G_tri, &fill_edges, 0));
    igraph_vector_int_destroy(&fill_edges);

    igraph_vector_ptr_t separators;
    igraph_vector_ptr_init(&separators, 0);
    IGRAPH_CHECK(MCS_Minseps(&G_tri, &separators));
    long l = igraph_vector_ptr_size(&separators);
    //t2 = clock();
    //printf("[TIMING] Step 2 (minimal separators): %.4f sec\n",(double)(t2 - t1) / CLOCKS_PER_SEC);

    // -------- Step 3: 构建原图邻接矩阵 A_G --------
    //t1 = clock();
    //float* A_G = calloc(n * n, sizeof(float));
    //igraph_es_t es;
    //igraph_eit_t eit;
    //IGRAPH_CHECK(igraph_es_all(&es, IGRAPH_EDGEORDER_ID));
    //IGRAPH_CHECK(igraph_eit_create(G, es, &eit));
    //igraph_es_destroy(&es);
    //t2 = clock();
    //printf("[TIMING] Step 3 (build adjacency A_G): %.4f sec\n",(double)(t2 - t1) / CLOCKS_PER_SEC);

    // -------- Step 4: 构建 B_H (n x l) --------
    // Step 4: 构建 B_H 矩阵（单精度）
    //t1 = clock();
    float* B_H = calloc(n * l, sizeof(float));
    for (long j = 0; j < l; j++) {
        igraph_vector_int_t* sep = (igraph_vector_int_t*)VECTOR(separators)[j];
        for (long k = 0; k < igraph_vector_int_size(sep); k++) {
            long v = VECTOR(*sep)[k];
            B_H[v * l + j] = 1.0f;
        }
    }
    //t2 = clock();
    //printf("[TIMING] Step 4a (build B_H): %.4f sec\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    // ---------------- Step 4b: Build C ----------------
    //t1 = clock();

    // Step 4b: C = A_G * B_H using sparse column approach
    float* C = calloc((size_t)n * (size_t)l, sizeof(float));

    // 遍历 B_H 每列非零元素
    for (long j=0; j<l; j++){
        igraph_vector_int_t* sep = (igraph_vector_int_t*)VECTOR(separators)[j];
        for (long k=0; k<igraph_vector_int_size(sep); k++){
            long v = VECTOR(*sep)[k];
            igraph_vector_int_t neighbors;
            igraph_vector_int_init(&neighbors,0);
            IGRAPH_CHECK(igraph_neighbors(G,&neighbors,v,IGRAPH_ALL));
            for(long ni=0;ni<igraph_vector_int_size(&neighbors);ni++){
                long u = VECTOR(neighbors)[ni];
                C[u*l + j] += 1.0f;
            }
            igraph_vector_int_destroy(&neighbors);
        }
    }


    //t2 = clock();
    //printf("[TIMING] Step 4b (build C sparse with B_H): %.4f sec\n", (double)(t2 - t1) / CLOCKS_PER_SEC);

    // -------- Step 6: 判断 clique-minimal --------
    for (long j = 0; j < l; j++) {
        igraph_vector_int_t* sep = (igraph_vector_int_t*)VECTOR(separators)[j];
        long k;
        for (k = 0; k < igraph_vector_int_size(sep); k++) {
            long v = VECTOR(*sep)[k];
            if (C[v * l + j] != igraph_vector_int_size(sep) - 1) break;
        }
        if (k == igraph_vector_int_size(sep)) {
            // 满足 clique-minimal 条件，加入结果
            igraph_vector_int_t* sep_copy = malloc(sizeof(igraph_vector_int_t));
            igraph_vector_int_init_copy(sep_copy, sep);
            igraph_vector_ptr_push_back(clique_separators, sep_copy);
        }
    }

    // -------- Step 7: 清理 --------
    igraph_destroy(&G_tri);

    // 先释放 separators 中每个 sep
    long nsep = igraph_vector_ptr_size(&separators);
    for (long i = 0; i < nsep; i++) {
        igraph_vector_int_t* sep = (igraph_vector_int_t*) VECTOR(separators)[i];
        igraph_vector_int_destroy(sep);
        free(sep);
    }
    igraph_vector_ptr_destroy(&separators);

    //free(A_G);
    free(B_H);
    free(C);


    return IGRAPH_SUCCESS;
}

