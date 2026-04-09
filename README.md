# Decomposition of Large-Scale Undirected Graphical Models via Local Expansion

This repository contains the code accompanying the paper **“Decomposition of large-scale undirected graphical models via local expansion.”**

## Overview

The repository includes the implementations used in the numerical experiments reported in the paper.

- **Section 5.1**: *Maximum Likelihood Estimates of Gaussian Distribution on Real Graph*  
  The scripts `main_time.r` and `main_precision.r` are used to evaluate the computational efficiency gains of the graph decomposition strategy for maximum likelihood estimation in Gaussian graphical models.

- **Section 5.2**: *Efficiency Comparison of Graph Decomposition Approaches*  
  The folder `Efficiency Comparison of Graph Decomposition` contains the code for this experiment. The main procedures can be run through `main.ipynb`.

## Dependencies

Part of the Section 5.1 experiment relies on **gRips**, available at:  <https://github.com/hojsgaard/gRips>

The dynamic library `decom_h.dll` depends on `igraph.dll`, which is installed via `vcpkg-master` and located in `./vcpkg-master/installed/x64-windows/bin`. 
To ensure that `decom_d.dll` can be loaded correctly in R, please set the system path in advance:

```r
Sys.setenv(PATH = paste("./vcpkg-master/installed/x64-windows/bin", Sys.getenv("PATH"), sep=";"))
```

For Section 5.2, the required C implementations have already been compiled into `.pyd` files and are provided in the folder `compiled_modules`.

## File Structure

- `main_time.r`  
  Runtime evaluation for Section 5.1.

- `main_precision.r`  
  Precision-related evaluation for Section 5.1.

- `Efficiency Comparison of Graph Decomposition/`  
  Code for the experiments in Section 5.2.

- `compiled_modules/`  
  Precompiled Python extension modules used by the Section 5.2 experiments.

- `main.ipynb`  
  Jupyter notebook for running the Section 5.2 experiments.

## Notes

- The R scripts correspond to the Gaussian graphical model MLE experiments on real graphs.
- The Python notebook and compiled modules correspond to the efficiency comparison of different graph decomposition approaches.
- Please ensure that the required R and Python environments are properly configured before running the code.

## Citation

If you use this repository, please cite the corresponding paper:

**Decomposition of large-scale undirected graphical models via local expansion**
