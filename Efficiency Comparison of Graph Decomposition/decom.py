import os
import sys
import time
import pandas as pd
import importlib.util

# ==== 配置运行环境 ====
# 1. 确保 vcpkg 的 DLL 可以找到
os.environ['PATH'] = (
    r"D:\vcpkg-master\installed\x64-windows\bin;" + os.environ['PATH']
)

# 2. 把 C++ 扩展模块的路径加到 sys.path
module_paths = [
    os.path.abspath(".")  # 当前目录
]

for p in module_paths:
    if p not in sys.path:
        sys.path.append(p)

# ==== 封装动态加载函数 ====
def load_module(module_name, so_path):
    """动态加载 .pyd 模块"""
    if not os.path.exists(so_path):
        raise FileNotFoundError(f"找不到模块文件: {so_path}")
    spec = importlib.util.spec_from_file_location(module_name, so_path)
    if spec is None:
        raise ImportError(f"无法为 {module_name} 创建 spec")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module

# ==== 实际加载你的 C++ 扩展 ====
decom_h = load_module(
    "decom_h",
    r"D:\Jupyter\A_final_algorithms\netdecom_c\build\python_modules\decom_h.pyd",
)

decom_matrix = load_module(
    "decom_matrix",
    r"D:\Jupyter\A_final_algorithms\decom_matrix\build\python_modules\Release\decom_matrix.pyd",
)

decom_mcs_m = load_module(
    "decom_mcs_m",
    r"D:\Jupyter\A_final_algorithms\decom_mcs_m\build\python_modules\Release\decom_mcs_m.pyd",
)




# ==========================
# 工具函数
# ==========================
def is_equal(list1, list2):
    set1 = set(frozenset(edge) for edge in list1)
    set2 = set(frozenset(edge) for edge in list2)
    return set1 == set2

# ==========================
# 对真实图进行性能测试
# ==========================
def benchmark_real_graphs(filename, repeat=50, add_connectivity=True):
    """
    对单个真实图文件进行算法运行时间测试
    :param filename: 图文件名
    :param repeat: 重复次数
    :param add_connectivity: 是否在图不连通时添加 n-1 条边
    :return: dict，包括节点数、边数、三种算法平均运行时间
    """
    import netdecom as nd

    G_ig = nd.get_example(filename, class_type="ig")

    if add_connectivity:
        connected_components = G_ig.components()
        if len(connected_components) > 1:
            nodes = [component[0] for component in connected_components]
            for i in range(len(nodes) - 1):
                G_ig.add_edge(nodes[i], nodes[i + 1])

    times_mcsm, times_matrix, times_cmsa = [], [], []

    for _ in range(repeat):
        start = time.time()
        atom_cmsa = decom_h.recursive_decom(G_ig, method="cmsa")[0]
        times_cmsa.append(time.time() - start)

        start = time.time()
        clique_sep_matrix = decom_matrix.clique_minimal_separators(G_ig)
        times_matrix.append(time.time() - start)

        start = time.time()
        atom_mcsm, clique_sep_mcsm = decom_mcs_m.atoms(G_ig)
        times_mcsm.append(time.time() - start)

    result = {
        "Filename": filename,
        "Nodes": G_ig.vcount(),
        "Edges": G_ig.ecount(),
        "Avg_Time_mcsm": sum(times_mcsm)/repeat,
        "Avg_Time_CMSA": sum(times_cmsa)/repeat,
        "Avg_Time_Matrix": sum(times_matrix)/repeat
    }

    return result

# ==========================
# 对随机生成图进行性能测试
# ==========================
def benchmark_random_graphs(ns, ps, rep=20, output_file="all_results.csv"):
    """
    对随机生成的连通图进行算法性能测试，并保存结果
    :param ns: list, 图节点数
    :param ps: list, 边概率
    :param rep: int, 每个参数重复次数
    :param output_file: str, 保存 CSV 文件名
    :return: dict {(n,p): {'Avg_mcsm': ..., 'Avg_CMSA': ..., 'Avg_matrix': ...}}
    """
    import netdecom as nd

    results = []
    avg_results = {}

    for n in ns:
        for p in ps:
            times_mcsm, times_cmsa, times_matrix = [], [], []
            valid = True

            for i in range(rep):
                # 生成连通随机图
                G_ig = nd.generator_connected_ug(n, p, class_type="ig")
                
                # CMSA
                start = time.time()
                atom_cmsa = decom_h.recursive_decom(G_ig, method="cmsa")[0]
                t_cmsa = time.time() - start

                # Matrix 
                start = time.time()
                clique_sep_matrix = decom_matrix.clique_minimal_separators(G_ig)
                t_matrix = time.time() - start

                # Atoms
                start = time.time()
                atom_mcsm, clique_sep_mcsm = decom_mcs_m.atoms(G_ig)
                t_mcsm = time.time() - start

                # 可选一致性检查
                if not is_equal(atom_mcsm, atom_cmsa) or not len(clique_sep_matrix)==len(clique_sep_mcsm):
                    print(f"一致性错误: n={n}, p={p}, rep={i}")
                    valid = False
                    break

                # 保存每次结果
                results.append({
                    "n": n,
                    "p": p,
                    "rep": i,
                    "Time_mcsm": t_mcsm,
                    "Time_CMSA": t_cmsa,
                    "Time_Matrix": t_matrix
                })

                times_mcsm.append(t_mcsm)
                times_cmsa.append(t_cmsa)
                times_matrix.append(t_matrix)

            # 计算平均
            if valid and len(times_mcsm) == rep:
                avg_mcsm = sum(times_mcsm)/rep
                avg_cmsa = sum(times_cmsa)/rep
                avg_matrix = sum(times_matrix)/rep
                print(f"n={n}, p={p} | Avg mcsm={avg_mcsm:.4f}s, "
                      f"Avg CMSA={avg_cmsa:.4f}s, Avg matrix={avg_matrix:.4f}s")
                avg_results[(n,p)] = {
                    "Avg_mcsm": avg_mcsm,
                    "Avg_CMSA": avg_cmsa,
                    "Avg_matrix": avg_matrix
                }

    # 保存 CSV
    df = pd.DataFrame(results)
    df.to_csv(output_file, index=False)
    print(f"所有结果已保存到 {output_file}")

    return avg_results
