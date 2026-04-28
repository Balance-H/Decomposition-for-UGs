//#define _CRTDBG_MAP_ALLOC //用于内存泄露检测
//#include <crtdbg.h>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "igraph.h"
#include "decom_matrix.h"

namespace py = pybind11;

// ⚠ 封装函数
std::vector<std::pair<int,int>> minimal_triangulation_py(py::object graph_obj) {
    // 获取 Python igraph 对象的 capsule
    py::capsule graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t* g = graph_capsule.get_pointer<igraph_t>();
    if (!g) throw std::runtime_error("Invalid igraph capsule");

    // ⚠ 这里用 igraph_vector_int_t
    igraph_vector_int_t edges;
    igraph_vector_int_init(&edges, 0);

    igraph_error_t err = minimal_triangulation_fmt(g, &edges);
    if (err != IGRAPH_SUCCESS) {
        igraph_vector_int_destroy(&edges);
        throw std::runtime_error("minimal_triangulation_fmt failed");
    }

    std::vector<std::pair<int,int>> result;
    for (long i = 0; i < igraph_vector_int_size(&edges); i += 2) {
        result.emplace_back((int)VECTOR(edges)[i], (int)VECTOR(edges)[i+1]);
    }

    igraph_vector_int_destroy(&edges);
    return result;
}

// ⚠ minimal_triangulation_py 已有

// -------- 新封装：clique_minimal_separators --------
std::vector<std::vector<int>> clique_minimal_separators_py(py::object graph_obj) {
    // 获取 Python igraph 对象 capsule
    py::capsule graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t* g = graph_capsule.get_pointer<igraph_t>();
    if (!g) throw std::runtime_error("Invalid igraph capsule");

    // 创建一个 vector_ptr 来存放 clique-minimal separators
    igraph_vector_ptr_t clique_seps;
    igraph_vector_ptr_init(&clique_seps, 0);

    igraph_error_t err = clique_minimal_separators(g, &clique_seps);
    if (err != IGRAPH_SUCCESS) {
        igraph_vector_ptr_destroy(&clique_seps);
        throw std::runtime_error("clique_minimal_separators failed");
    }

    // 转换为 std::vector<std::vector<int>> 返回给 Python
    std::vector<std::vector<int>> result;
    for (long i = 0; i < igraph_vector_ptr_size(&clique_seps); i++) {
        igraph_vector_int_t* sep = (igraph_vector_int_t*)VECTOR(clique_seps)[i];
        std::vector<int> sep_vec(igraph_vector_int_size(sep));
        for (long j = 0; j < igraph_vector_int_size(sep); j++) {
            sep_vec[j] = (int)VECTOR(*sep)[j];
        }
        result.push_back(sep_vec);

        // 释放 sep 内存
        igraph_vector_int_destroy(sep);
        free(sep);
    }

    igraph_vector_ptr_destroy(&clique_seps);
    return result;
}


PYBIND11_MODULE(decom_matrix, m) {
    m.doc() = "pybind11 bindings for decom_matrix minimal_triangulation_fmt";
    m.def("minimal_triangulation", &minimal_triangulation_py,
          "Compute minimal triangulation and return edge list",
          py::arg("graph"));

    m.def("clique_minimal_separators", &clique_minimal_separators_py,
          "Compute clique-minimal separators and return as list of lists",
          py::arg("graph"));
}
