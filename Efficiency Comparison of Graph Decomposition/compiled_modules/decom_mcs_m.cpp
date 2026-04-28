//#define _CRTDBG_MAP_ALLOC //用于内存泄露检测
//#include <crtdbg.h>


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <igraph.h>
#include "decom_mcs_m.h"  // 你的函数声明头文件

namespace py = pybind11;

//---------------- C 接口声明 ----------------
extern "C" void mcs_m_plus(
    const igraph_t *g,
    igraph_vector_int_t *alpha,
    igraph_vector_int_t *min_sep_gen,
    igraph_t *minimal_chordal
);

extern "C" void atoms(
    const igraph_t *g,
    igraph_vector_int_list_t *A,
    igraph_vector_int_list_t *S_C
);

//---------------- pybind11 包装 ----------------
py::tuple mcs_m_plus_wrapper(py::object graph_obj) {
    // 从 Python 对象获取 igraph capsule
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    const igraph_t *g = static_cast<igraph_t*>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!g) throw std::runtime_error("Invalid igraph capsule");

    // 初始化输出向量
    igraph_vector_int_t alpha;
    igraph_vector_int_init(&alpha, 0);
    igraph_vector_int_t min_sep_gen;
    igraph_vector_int_init(&min_sep_gen, 0);

    // 初始化最小弦图
    igraph_t minimal_chordal;
    igraph_empty(&minimal_chordal, 0, IGRAPH_UNDIRECTED);

    // 调用核心算法
    mcs_m_plus(g, &alpha, &min_sep_gen, &minimal_chordal);

    // 转换 alpha 为 Python list
    py::list py_alpha;
    for (long i = 0; i < igraph_vector_int_size(&alpha); ++i) {
        py_alpha.append(VECTOR(alpha)[i]);
    }

    // 转换 min_sep_gen 为 Python list
    py::list py_min_sep;
    for (long i = 0; i < igraph_vector_int_size(&min_sep_gen); ++i) {
        py_min_sep.append(VECTOR(min_sep_gen)[i]);
    }

    // 转换 minimal_chordal 的边为 Python list
    py::list edges_list;
    igraph_integer_t edge_count = igraph_ecount(&minimal_chordal);
    igraph_vector_int_t edge;
    igraph_vector_int_init(&edge, 2);
    for (igraph_integer_t e = 0; e < edge_count; ++e) {
        igraph_edge(&minimal_chordal, e, &VECTOR(edge)[0], &VECTOR(edge)[1]);
        edges_list.append(py::make_tuple(VECTOR(edge)[0], VECTOR(edge)[1]));
    }
    igraph_vector_int_destroy(&edge);

    // 释放资源
    igraph_vector_int_destroy(&alpha);
    igraph_vector_int_destroy(&min_sep_gen);
    igraph_destroy(&minimal_chordal);

    // 返回三类信息
    return py::make_tuple(py_alpha, py_min_sep, edges_list);
}

py::tuple atoms_wrapper(py::object graph_obj) {
    py::object graph_capsule = graph_obj.attr("__graph_as_capsule")();
    const igraph_t *g = static_cast<igraph_t*>(PyCapsule_GetPointer(graph_capsule.ptr(), nullptr));
    if (!g) throw std::runtime_error("Invalid igraph capsule");

    igraph_vector_int_list_t A;
    igraph_vector_int_list_t S_C;
    igraph_vector_int_list_init(&A, 0);
    igraph_vector_int_list_init(&S_C, 0);

    atoms(g, &A, &S_C);

    py::list py_A;
    for (long i = 0; i < igraph_vector_int_list_size(&A); ++i) {
        igraph_vector_int_t &v = VECTOR(A)[i];  // 修正这里，使用引用
        py::list inner;
        for (long j = 0; j < igraph_vector_int_size(&v); ++j) {
            inner.append(VECTOR(v)[j]);
        }
        py_A.append(inner);
    }

    py::list py_S_C;
    for (long i = 0; i < igraph_vector_int_list_size(&S_C); ++i) {
        igraph_vector_int_t &v = VECTOR(S_C)[i];  // 修正这里，使用引用
        py::list inner;
        for (long j = 0; j < igraph_vector_int_size(&v); ++j) {
            inner.append(VECTOR(v)[j]);
        }
        py_S_C.append(inner);
    }

    igraph_vector_int_list_destroy(&A);
    igraph_vector_int_list_destroy(&S_C);

    return py::make_tuple(py_A, py_S_C);
}

//---------------- 模块注册 ----------------
PYBIND11_MODULE(decom_mcs_m, m) {
    m.def("mcs_m_plus", &mcs_m_plus_wrapper, py::arg("graph"),
          "Run mcs_m_plus algorithm and return alpha vector as Python list.");

    m.def("atoms", &atoms_wrapper, py::arg("graph"),
          "Compute atoms decomposition and return tuple (A, S_C) of Python lists.");
}