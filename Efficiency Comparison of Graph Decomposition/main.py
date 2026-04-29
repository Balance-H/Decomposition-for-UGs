import csv
from decom import benchmark_random_graphs
from decom import benchmark_real_graphs

# ============================================================================
# RANDOM GRAPH BENCHMARKING
# ============================================================================
# Test the performance of decomposition algorithms on random graphs
# Parameters:
#   - ns: list of graph sizes (number of nodes)
#   - ps: list of edge probabilities (density parameters)
#   - rep: number of repetitions for each configuration
#   - output_file: CSV file to save random graph results
print("Starting random graph benchmarking...")
ns = [1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000]
ps = [0.1, 0.05, 0.01, 0.005]
rep = 50
output_file_random = "results_random.csv"
avg_results_random = benchmark_random_graphs(ns, ps, rep=rep, output_file=output_file_random)
print(f"Random graph results saved to: {output_file_random}")

# ============================================================================
# REAL GRAPH BENCHMARKING
# ============================================================================
# Test the performance of decomposition algorithms on real-world networks
# Parameters:
#   - filenames: list of graph files to benchmark
#   - repeat: number of repetitions for each graph
#   - add_connectivity: whether to compute connectivity metrics
print("\nStarting real graph benchmarking...")

# List of real-world network files to test
# Local edgelist files (edge list format): DD6.txt, com-youtube.ungraph.txt
# Library datasets: all others loaded via netdecom.get_example()
filenames = [
    "Animal-Network.txt",
    "bio-CE-GT.txt",
    "DD6.txt",                          # Local edgelist file
    "as20000102.txt",
    "CA-HepTh.txt",
    "com-youtube.ungraph.txt"           # Local edgelist file
]

# CSV file for real graph results
output_file_real = "results_real.csv"

# Collect all real graph results
all_real_results = []

# Benchmark each real graph
for filename in filenames:
    print(f"Processing: {filename}")
    try:
        # Check if this is a local edgelist file (as opposed to loading from library)
        is_local_edgelist = filename in ["DD6.txt", "com-youtube.ungraph.txt"]
        
        avg_results = benchmark_real_graphs(
            filename, 
            repeat=50, 
            add_connectivity=True,
            is_edgelist=is_local_edgelist
        )
        # Extract timing information from results dictionary
        print(f"  Nodes: {avg_results.get('Nodes', 'N/A')}, "
              f"Edges: {avg_results.get('Edges', 'N/A')}")
        print(f"  MCSM avg time: {avg_results.get('Avg_Time_mcsm', 0):.6f}s")
        print(f"  CMSA avg time: {avg_results.get('Avg_Time_CMSA', 0):.6f}s")
        print(f"  Matrix avg time: {avg_results.get('Avg_Time_Matrix', 0):.6f}s")
        all_real_results.append({
            'graph_name': filename,
            'results': avg_results
        })
    except Exception as e:
        print(f"  Error processing {filename}: {e}")

# Write real graph results to CSV file
if all_real_results:
    print(f"\nSaving real graph results to: {output_file_real}")
    try:
        with open(output_file_real, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            # Write header: graph name, nodes, edges, and average times for each algorithm
            writer.writerow([
                'Graph Name', 'Nodes', 'Edges', 
                'Avg_Time_mcsm (s)', 'Avg_Time_CMSA (s)', 'Avg_Time_Matrix (s)'
            ])
            # Write data rows - extract timing information from results
            for entry in all_real_results:
                results_dict = entry['results']
                writer.writerow([
                    results_dict.get('Filename', entry['graph_name']),
                    results_dict.get('Nodes', 'N/A'),
                    results_dict.get('Edges', 'N/A'),
                    f"{results_dict.get('Avg_Time_mcsm', 0):.6f}",
                    f"{results_dict.get('Avg_Time_CMSA', 0):.6f}",
                    f"{results_dict.get('Avg_Time_Matrix', 0):.6f}"
                ])
        print(f"Successfully saved real graph results to: {output_file_real}")
    except Exception as e:
        print(f"Error writing to CSV: {e}")

print("\nBenchmarking completed!")
