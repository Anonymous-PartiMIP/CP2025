# PartiMIP-CP2025

<div align="center">

**Parallel MIP Solving with Dynamic Task Decomposition**

</div>

## 📋 Project Overview

PartiMIP is an innovative framework for parallel mixed integer programming (MIP) solving that achieves efficient parallelization through dynamic task decomposition. This repository is anonymized for the CP 2025 paper review. Both the scheduler and worker processes are implemented in C++ using g++ 9.4.0 with the -O3 optimization flag, and parallelization is achieved via the pthread library.

## 📄 License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.


## 🗂️ Directory Structure

```
PartiMIP-CP2025/
├── Benchmark/          # MIPLIB 2017 test dataset
│   ├── benchmark.txt   # List of instances
│   └── setup.sh        # Script to download the dataset (downloads instances into the mps/ directory)
├── Evaluation/         # Experimental evaluation materials
│   ├── pre-compiled/   # Pre-compiled comparison solvers
│   └── run/            # Experiment execution scripts
├── PartiMIP/           # Core source code and build files
│   ├── BaseSolver/     # Base solvers (HiGHS, SCIP)
│   ├── PartiMIP-HiGHS/ # PartiMIP implementation using HiGHS as the base solver
│   └── PartiMIP-SCIP/  # PartiMIP implementation using SCIP as the base solver
├── Record/             # New best known solutions to open instances
├── Result/             # Summary of experimental results for the paper
└── Test/               # Test cases
```

## 🚀 Quick Start

### System Requirements

- **Operating System**: Linux / Unix
- **Compiler**: A modern compiler supporting C++17 (e.g., GCC 9.0+)
- **Build Tool**: CMake 3.15 or higher
- **Processor**: A modern multi-threaded CPU

### Installation Steps

1. **Build the Base Solver HiGHS**

   ```bash
   cd PartiMIP/BaseSolver/HiGHS
   bash build_highs.sh
   ```

2. **Build PartiMIP-HiGHS**

   ```bash
   cd PartiMIP/PartiMIP-HiGHS
   mkdir -p build && cd build
   cmake ..
   make -j
   ```

3. **Build PartiMIP-SCIP**

   ```bash
   cd PartiMIP/PartiMIP-SCIP
   mkdir -p build && cd build
   cmake ..
   make -j
   ```

## 💻 Usage

After compilation, the executables for each project are located in their respective `build` directories.

### Command Line Parameters

| Parameter      | Description                                   | Example             |
|----------------|-----------------------------------------------|---------------------|
| `--instance`   | Path to the MIP instance file (.mps)          | Test/app1-1.mps     |
| `--threadNum`  | Maximum number of worker processes (cores)    | 8                   |
| `--cutoff`     | Time limit for solving (in seconds)           | 300                 |

### Usage Example

```bash
./PartiMIP-HiGHS \
    --instance=Test/app1-1.mps \
    --threadNum=8 \
    --cutoff=300
```

## 🔬 Experimental Evaluation

### Dataset Preparation

The experiments utilize the MIPLIB2017 benchmark, which comprises 240 general MIP instances collected from various real-world applications. Running the following command will automatically download and extract the dataset into the `Benchmark/mps/` directory:

```bash
cd Benchmark
bash setup.sh
```

### Pre-compiled Software

The pre-compiled comparison solvers are stored in the `Evaluation/pre-compiled/` directory and include:

1. **Relevant Solvers**
   - **SCIP (v9.2.0)**: A widely used open-source MIP solver in academia and industry, developed continuously for over 20 years.
   - **FiberSCIP (v1.0.0, based on SCIP 9.2.0)**: A parallel version of SCIP developed by the SCIP team that employs a conventional progressive parallel strategy.
   - **HiGHS (v1.9.0)**: A top-performing open-source MIP solver in recent years, featuring a parallel dual simplex method, symmetry detection, and clique detection.

2. **Our Implementations**
   - **PartiMIP-HiGHS**: The PartiMIP implementation based on HiGHS.
   - **PartiMIP-SCIP**: The PartiMIP implementation based on SCIP.

### Hardware Configuration

- **CPU**: 2 × AMD EPYC 9654
- **Memory**: 2048 GB
- **Operating System**: Ubuntu 20.04.4

### Experimental Setup

- **Parallel Core Configurations**: 8, 16, 32, 64, and 128 cores
- **Solving Objective**: Achieve a relative optimality gap of 0 (aligned with the Hans Mittelmann ranking standard)
- **Time Limit**: 300 seconds per instance
- **Total Experiment Time**: Accumulated over 2.3 CPU years



### Experiment Execution

The experiment execution scripts are located in the `Evaluation/run/` directory and can be used to run the experiments directly.

To fully utilize computing resources, multiple parallel configurations are adopted, such as:
- Running 128 single-threaded instances simultaneously
- Running 16 instances with 8 threads each concurrently
- Running 4 instances with 32 threads each concurrently
- Running a single instance using 128 threads

## 📊 Experimental Results

The experimental results are stored in the `Result/` directory and include the following:

### Base Solver Results

- `SCIP_Sequential.txt`: Sequential solving results for SCIP
- `HiGHS_Sequential.txt`: Sequential solving results for HiGHS

### Parallel Solver Results

- **FiberSCIP Results**
  - Files ranging from `FiberSCIP_8.txt` to `FiberSCIP_128.txt`: Parallel test results for 8 to 128 cores.
- **Parallel HiGHS Results**
  - Files ranging from `HiGHS_8.txt` to `HiGHS_128.txt`: Parallel test results for 8 to 128 cores.

### PartiMIP Implementation Results

- **PartiMIP-SCIP Results**
  - Files ranging from `PartiMIP_SCIP_8.txt` to `PartiMIP_SCIP_128.txt`: Parallel test results for 8 to 128 cores.
- **PartiMIP-HiGHS Results**
  - Files ranging from `PartiMIP_HiGHS_8.txt` to `PartiMIP_HiGHS_128.txt`: Parallel test results for 8 to 128 cores.

All experimental result files adhere to the following naming convention:

```[Solver Name] – [Thread Count], [Instance], [Status], [Objective Value (if applicable)], [Solve Time (if applicable)].```

## 🔥 New Best Known Solutions

The `Record/` directory contains our breakthrough results for open MIP instances from MIPLIB. Open instances are those for which the optimal solution remains unproven and represent significant challenges in the field of MIP solving.

### Key Achievements

- PartiMIP has established new best known solutions for 21 open instances.
- All solutions have been verified and accepted by MIPLIB.
- Notably, the instance "shiftreg5-1" was not only improved but also proven optimal by PartiMIP.

---

<div align="center">
Made with ❤️ by the PartiMIP Team
</div>


