//#define _CRTDBG_MAP_ALLOC //用于内存泄露检测
//#include <crtdbg.h>


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <igraph.h>
#include "decom_h.h"  // 你的函数声明头文件

namespace py = pybind11;

// 包装 components_forbidden
// C 函数声明（假设在 decom_h.h 中）
extern "C" igraph_error_t components_forbidden(
    const igraph_t *graph,
    igraph_vector_ptr_t *components,
    igraph_vector_ptr_t *boundaries,
    const igraph_vector_int_t *forbidden_vertices);

// 用于释放 igraph_vector_ptr_t 中每个元素的辅助函数
void free_vector_ptr(igraph_vector_ptr_t *vec_ptr) {
    for (long i = 0; i < igraph_vector_ptr_size(vec_ptr); ++i) {
        igraph_vector_int_t *v = static_cast<igraph_vector_int_t*>(VECTOR(*vec_ptr)[i]);
        igraph_vector_int_destroy(v);
        free(v);
    }
    igraph_vector_ptr_destroy(vec_ptr);
}

// C++ 封装接口，返回 Python 的 list[(component, boundary), ...]
py::list components_forbidden_wrapper(py::object graph_obj,
                             const std::vector<int> &forbidden_vertices = {}) {
    // 从 Python igraph 对象获取 igraph_t 指针
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t *graph = static_cast<igraph_t*>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!graph) throw std::runtime_error("Invalid igraph capsule");

    // 初始化禁忌节点向量
    igraph_vector_int_t forbidden_vec;
    igraph_vector_int_init(&forbidden_vec, 0);
    for (int v : forbidden_vertices) {
        igraph_vector_int_push_back(&forbidden_vec, v);
    }

    // 初始化存储组件和边界的向量
    igraph_vector_ptr_t components;
    igraph_vector_ptr_init(&components, 0);
    igraph_vector_ptr_t boundaries;
    igraph_vector_ptr_init(&boundaries, 0);

    // 调用 C 函数
    igraph_error_t err = components_forbidden(graph, &components, &boundaries, &forbidden_vec);
    igraph_vector_int_destroy(&forbidden_vec);

    if (err != IGRAPH_SUCCESS) {
        free_vector_ptr(&components);
        free_vector_ptr(&boundaries);
        throw std::runtime_error("components_forbidden failed");
    }

    // 转换为 Python list[(list[int], list[int]), ...]
    py::list result;
    igraph_integer_t n = igraph_vector_ptr_size(&components);
    for (igraph_integer_t i = 0; i < n; i++) {
        igraph_vector_int_t *comp = static_cast<igraph_vector_int_t*>(VECTOR(components)[i]);
        igraph_vector_int_t *bound = static_cast<igraph_vector_int_t*>(VECTOR(boundaries)[i]);

        py::list py_comp;
        for (long j = 0; j < igraph_vector_int_size(comp); ++j) {
            py_comp.append(VECTOR(*comp)[j]);
        }

        py::list py_bound;
        for (long j = 0; j < igraph_vector_int_size(bound); ++j) {
            py_bound.append(VECTOR(*bound)[j]);
        }

        result.append(py::make_tuple(py_comp, py_bound));
    }

    // 释放内存
    free_vector_ptr(&components);
    free_vector_ptr(&boundaries);

    return result;
}


py::list close_separator_wrapper(py::object graph_obj,
                                        int vertex,
                                        const std::vector<int> &forbidden_vertices = {}) {
    // 获取封装的图对象
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t *graph = static_cast<igraph_t*>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!graph) throw std::runtime_error("Invalid igraph capsule");

    // 初始化禁忌节点列表
    igraph_vector_int_t forbidden_vec;
    igraph_vector_int_init(&forbidden_vec, 0);
    for (auto v : forbidden_vertices) {
        igraph_vector_int_push_back(&forbidden_vec, v);
    }

    // 初始化 bound_b
    igraph_vector_int_t bound_b;
    igraph_vector_int_init(&bound_b, 0);

    igraph_error_t err = close_separator(graph, vertex,
                                                               &forbidden_vec, &bound_b);
    igraph_vector_int_destroy(&forbidden_vec);

    if (err != IGRAPH_SUCCESS) {
        igraph_vector_int_destroy(&bound_b);
        throw std::runtime_error("close_separator_b failed");
    }

    // 转换 bound_b 为 Python 列表
    py::list py_bound_b;
    for (long i = 0; i < igraph_vector_int_size(&bound_b); ++i) {
        py_bound_b.append(VECTOR(bound_b)[i]);
    }

    igraph_vector_int_destroy(&bound_b);
    return py_bound_b;
}



extern "C" igraph_error_t find_convex_hull(
    const igraph_t *graph,
    const igraph_vector_int_t *r_nodes,
    igraph_vector_int_t *H_out,
    const char *method  // 新增参数
);

py::list find_convex_hull_wrapper(py::object graph_obj, 
                                   const std::vector<int> &r_nodes,
                                   const std::string &method) {
    // 获取 igraph_t 指针
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t *graph = static_cast<igraph_t*>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!graph) throw std::runtime_error("Invalid igraph capsule");

    // 将 r_nodes 转成 igraph_vector_int_t
    igraph_vector_int_t r_vec;
    igraph_vector_int_init(&r_vec, 0);
    for (int v : r_nodes) {
        igraph_vector_int_push_back(&r_vec, v);
    }

    // 调用 find_convex_hull，传入 method.c_str()
    igraph_vector_int_t H_vec;
    igraph_vector_int_init(&H_vec, 0);

    igraph_error_t err = find_convex_hull(graph, &r_vec, &H_vec, method.c_str());
    igraph_vector_int_destroy(&r_vec);

    if (err != IGRAPH_SUCCESS) {
        igraph_vector_int_destroy(&H_vec);
        throw std::runtime_error("find_convex_hull failed");
    }

    // 转换结果到 Python list
    py::list result;
    for (long i = 0; i < igraph_vector_int_size(&H_vec); ++i) {
        result.append(VECTOR(H_vec)[i]);
    }
    igraph_vector_int_destroy(&H_vec);

    return result;
}

// 辅助函数，将 igraph_vector_ptr_t 转换为 Python list（嵌套 list）
py::list igraph_vector_ptr_to_pylist(igraph_vector_ptr_t *vec_ptr) {
    py::list result;
    igraph_integer_t n = igraph_vector_ptr_size(vec_ptr);
    for (igraph_integer_t i = 0; i < n; i++) {
        igraph_vector_int_t *v = (igraph_vector_int_t *)VECTOR(*vec_ptr)[i];
        py::list inner_list;
        for (int j = 0; j < igraph_vector_int_size(v); j++) {
            inner_list.append(VECTOR(*v)[j]);
        }
        result.append(inner_list);
    }
    return result;
}


// 递归分解包装，直接接收 Python igraph.Graph 对象，内部调用 __graph_as_capsule 获取指针
py::list recursive_decom_wrapper(py::object graph_obj, const std::string &method = "cmsa") {
    // 通过 __graph_as_capsule 获取底层指针
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t *g = static_cast<igraph_t *>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!g) throw std::runtime_error("Invalid igraph capsule");

    // 1. 初始化两个输出容器
    igraph_vector_ptr_t atoms; // 对应 atoms (原 blocks)
    igraph_vector_ptr_t separators; // 对应 separators
    igraph_vector_ptr_init(&atoms, 0);
    igraph_vector_ptr_init(&separators, 0);

    igraph_error_t err = IGRAPH_SUCCESS;
    
    // 2. 调用修改后的 recursive_decom 函数
    try {
        err = recursive_decom(g, method.c_str(), &atoms, &separators);
    } catch (...) {
        // 捕获 C++ 异常，确保即使失败也能进行清理
        err = IGRAPH_EINTERNAL; // 使用一个内部错误代码
    }


    if (err != IGRAPH_SUCCESS) {
        // 3. 错误发生时，清理已初始化的容器
        for (igraph_integer_t i = 0; i < igraph_vector_ptr_size(&atoms); i++) {
            igraph_vector_int_destroy((igraph_vector_int_t *)VECTOR(atoms)[i]);
            free(VECTOR(atoms)[i]);
        }
        igraph_vector_ptr_destroy(&atoms);
        
        for (igraph_integer_t i = 0; i < igraph_vector_ptr_size(&separators); i++) {
            igraph_vector_int_destroy((igraph_vector_int_t *)VECTOR(separators)[i]);
            free(VECTOR(separators)[i]);
        }
        igraph_vector_ptr_destroy(&separators);

        throw std::runtime_error("recursive_decom failed with code " + std::to_string(err));
    }

    // 4. 将两个 iGraph 向量指针容器转换为 Python 列表
    py::list atoms_pylist = igraph_vector_ptr_to_pylist(&atoms);
    py::list separators_pylist = igraph_vector_ptr_to_pylist(&separators);

    // 5. 释放两个容器及其所有元素（堆分配的 igraph_vector_int_t）
    
    // 清理 atoms 容器
    for (igraph_integer_t i = 0; i < igraph_vector_ptr_size(&atoms); i++) {
        igraph_vector_int_destroy((igraph_vector_int_t *)VECTOR(atoms)[i]);
        free(VECTOR(atoms)[i]);
    }
    igraph_vector_ptr_destroy(&atoms);
    
    // 清理 separators 容器
    for (igraph_integer_t i = 0; i < igraph_vector_ptr_size(&separators); i++) {
        igraph_vector_int_destroy((igraph_vector_int_t *)VECTOR(separators)[i]);
        free(VECTOR(separators)[i]);
    }
    igraph_vector_ptr_destroy(&separators);

    // 6. 返回一个包含 (atoms, separators) 的 Python 列表或元组
    // 这里使用 py::list 返回一个 [atoms_list, separators_list] 的结构
    py::list result;
    result.append(atoms_pylist);
    result.append(separators_pylist);
    
    return result;
}



py::list SAHR_wrapper(py::object graph_obj, const std::vector<int> &r_nodes) {
    // 获取 igraph_t 指针
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    igraph_t *graph = static_cast<igraph_t*>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!graph) throw std::runtime_error("Invalid igraph capsule");

    // 转换 r_nodes 为 int 数组
    std::vector<int> r_vec = r_nodes;  // 本地 copy
    int *r_ptr = r_vec.data();
    int r_size = static_cast<int>(r_vec.size());

    // 输出参数
    int *local2global = nullptr;
    int result_size = 0;

    igraph_error_t err = SAHR(graph, r_ptr, r_size, &local2global, &result_size);
    if (err != IGRAPH_SUCCESS) {
        if (local2global) free(local2global);
        throw std::runtime_error("SAHR failed with code " + std::to_string(err));
    }

    // 转换结果为 Python list
    py::list result;
    for (int i = 0; i < result_size; i++) {
        result.append(local2global[i]);
    }

    free(local2global);
    return result;
}



PYBIND11_MODULE(decom_h, m) {


    m.def("SAHR", &SAHR_wrapper,
    py::arg("graph"),
    py::arg("r_nodes"),
    "Run SAHR algorithm and return remaining nodes' global indices as a list.");

    m.doc() = "Example igraph C extension";

    m.def("recursive_decom", &recursive_decom_wrapper,
      py::arg("graph"), // 修改参数名以匹配 Python 接口的期望
      py::arg("method") = "cmsa",
      "Perform recursive decomposition of the graph using the specified method.\n"
      "Returns: A list containing two lists: [atoms, separators].\n"
      "  - atoms: List of lists, where each inner list is an atom (a vertex set).\n"
      "  - separators: List of lists, where each inner list is a clique minimal separator (a vertex set).");

    m.def("find_convex_hull", &find_convex_hull_wrapper,
          py::arg("graph"),
          py::arg("r_nodes"),
          py::arg("method"),
          "Perform CMSA decomposition on the graph starting from nodes r_nodes.");


    m.def("components_forbidden", &components_forbidden_wrapper,
        py::arg("graph"),
        py::arg("forbidden_vertices") = std::vector<int>{},
        "Calculate connected components excluding forbidden vertices, "
        "and return each component with its boundary forbidden nodes.");

    m.def("close_separator", &close_separator_wrapper,
        py::arg("graph"),
        py::arg("vertex"),
        py::arg("forbidden_vertices") = std::vector<int>{},
        "Calculate forbidden boundary reachable from vertex without traversing forbidden vertices.");

}

