# Cross-Docking Scheduler — v0.1

> **Greedy heuristic** (ERT + SPT) for the Cross-Docking Truck Scheduling problem with **multiple inbound and outbound doors**.  
> Minimizes the **makespan** (completion time of the last outbound truck).

---

## Problem Description

A cross-docking warehouse receives goods via **inbound trucks**, transfers them directly to **outbound trucks** (without storage), and the goal is to schedule both sides so as to minimize the total makespan.

Each inbound truck `i` has:
- a **release time** — earliest time it can start being unloaded
- an **unload time** — time to empty it at an inbound door
- a **transfer time** matrix `transfer_time[i][j]` — time to move goods from inbound truck `i` to outbound truck `j`

Each outbound truck `j` has:
- a **load time** — time to fill it at an outbound door

The warehouse has `d_inbound` inbound doors and `d_outbound` outbound doors, all operating in parallel.

### Input Format (`.dzn`)

Instances use a MiniZinc-style data format:

```
InboundTrucks  = 8;
OutboundTrucks = 5;
InboundDoors   = 2;
OutboundDoors  = 1;
ReleaseTime    = [18, 6, 17, 11, 14, 1, 21, 16];
UnloadTime     = [4, 10, 4, 5, 8, 4, 6, 8];
LoadTime       = [2, 5, 4, 3, 2];
TransferTime   = [| 4,5,3,2,4 | 3,2,4,4,2 | ... |];
```

---

## Algorithm — Greedy ERT+SPT

The solver (`CD_Greedy.cc`) applies two independent sorting rules:

| Side | Rule | Tie-breaking |
|------|------|--------------|
| Inbound trucks | **ERT** — Earliest Release Time first | **SPT** — shorter unload time first |
| Outbound trucks | **SPT** — Shortest Processing Time (load time) first | — |

**Door assignment** uses the **EAD** (Earliest Available Door) policy: the truck is always assigned to the door that becomes free first (greedy, O(n·d) scan).

### Makespan Computation

`ComputeMakespan()` in `CD_Data.cc` simulates execution in two steps:

1. **Inbound simulation** — schedules trucks on `d_inbound` parallel doors (EAD policy), computing `finish_unload[i]` for every truck.
2. **Outbound simulation** — for each outbound truck `j`, computes `goods_ready[j]` as the maximum over all inbound trucks of `finish_unload[i] + transfer_time[i][j]`, then assigns truck `j` to the earliest available outbound door.

Makespan = maximum finish time across all outbound trucks.

---

## Repository Structure

```
cross-docking-scheduler/
├── SourceFiles/
│   ├── CD_Data.hh / CD_Data.cc       # Input/Output data model + makespan
│   ├── CD_Greedy.hh / CD_Greedy.cc   # Greedy ERT+SPT solver
│   ├── CD_Driver.cc                  # main() entry point
│   ├── CD_Test                       # Pre-compiled Linux binary
│   └── Makefile                      # Build rules (g++ -std=c++17 -O3)
├── Instances/                        # .dzn benchmark instances
│   └── generate.py                   # Instance generator script
├── run_all.sh                        # Batch runner → results_v0.1.txt
├── run_instances.sh                  # Alternative batch runner
├── python genera_excel.py            # Parses results and exports Excel report
└── README.md
```

---

## Build

Requirements: `g++` with C++17 support.

```bash
cd SourceFiles
make
```

This produces `CD_Test.exe` (or `CD_Test` on Linux) using `-std=c++17 -O3 -Wall -Wfatal-errors`.

To clean build artifacts:

```bash
make clean
```

> **Note:** A pre-compiled Linux binary `CD_Test` is already included in `SourceFiles/`.

---

## Usage

### Single instance

```bash
./SourceFiles/CD_Test <path/to/instance.dzn>
```

Example output (small instances with ≤ 20 inbound trucks print full input/output):

```
InboundTrucks  = 8;  OutboundTrucks = 5;  ...
Doors : in=2 out=1
Inbound sequence : [5, 1, 3, 4, 7, 6, 2, 0]
Outbound sequence: [0, 4, 2, 3, 1]
Makespan : 47
CPU time : 0.000031 s
```

### Batch run (all scenarios)

```bash
bash run_all.sh
```

Results are written to `results_v0.1.txt`, organized by scenario and instance size.

### Export to Excel

After running `run_all.sh`, generate a formatted Excel report:

```bash
pip install openpyxl
python "python genera_excel.py"
```

Output: `output/Cross-Docking-Results-v0.1.xlsx` — one sheet per scenario, with columns: instance name, seed, n, m, makespan, CPU time (s), CPU time (min), release times, inbound/outbound sequences.

---

## Benchmark Instances

Instances are named following this pattern:

```
cd_n<NNNN>_m<MMMM>_<scenario>_s<SEED>.dzn
```

### Instance sizes covered

| Inbound (n) | Outbound (m) |
|-------------|--------------|
| 8 | 5 |
| 12 | 8 |
| 20 | 13 |
| 40 | 20 |
| 60 | 30 |
| 80 | 40 |
| 100 | 50 |
| 120 | 60 |
| 200 | 100 |
| 300 | 150 |
| 400 | 200 |
| 500 | 250 |
| 600 | 300 |

### Scenarios

| Scenario | Description |
|----------|-------------|
| `uniform` | Release and service times uniformly distributed |
| `sparse` | Low transfer times, few dependencies |
| `clustered` | Trucks grouped by time windows |
| `asymmetric` | Strong asymmetry between inbound and outbound load |
| `congested` | High door utilization, many conflicts |
| `urgent` | Short release times, tight deadlines |
| `mixed` | Combination of the above |

For small sizes (n ≤ 20), three random seeds are provided per scenario; for larger sizes, typically one seed.

---

## Source Files Overview

| File | Role |
|------|------|
| `CD_Data.hh / .cc` | `CD_Input` (parser + data model), `CD_Output` (solution + `ComputeMakespan`) |
| `CD_Greedy.hh / .cc` | `GreedyCDSolver()` — ERT+SPT sorting + EAD door assignment |
| `CD_Driver.cc` | CLI entry point, timing via `std::chrono` |
| `Makefile` | Explicit dependency rules, `-O3` optimization |
| `run_all.sh` | Iterates over all (scenario, size, seed) combinations, collects output |
| `python genera_excel.py` | Regex-based parser of `results_v0.1.txt` → styled `.xlsx` |

---

## Roadmap

- [ ] **v0.2** — Local Search (swap / insert moves on inbound/outbound sequences)
- [ ] Simulated Annealing or Tabu Search metaheuristic
- [ ] Comparison against exact MiniZinc model on small instances
- [ ] Statistical analysis across seeds and scenarios

---

## License

Academic project — University of Udine.  
See individual source files for authorship details.
