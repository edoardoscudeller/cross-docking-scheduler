# Cross-Docking Truck Scheduling

Discrete optimization project — Advanced Scheduling Systems.

## Problem

In a cross-docking center, a set of **inbound trucks** (that unload goods) and **outbound trucks** (that load goods) must be scheduled to dock at the available doors.

**Objective:** Minimize the **makespan** — the total time to complete all operations.

## Model

- `n` inbound trucks, each with a release time `r[i]` and unloading duration `p[i]`
- `m` outbound trucks, each with a loading duration `q[j]`
- Transfer time `t[i][j]`: time to move goods from inbound truck `i` to outbound truck `j`
- 1 inbound door, 1 outbound door (extensible)
- The outbound truck `j` can start loading only after all its required goods have been transferred

## Techniques

| Phase | Technique | File |
|---|---|---|
| Phase 1 | **Greedy ERT** (Earliest Release Time) | `CD_Greedy.cc` |
| Phase 2 | *Local Search (swap/insertion)* | *(to be added)* |

## Build

```bash
cd SourceFiles
make
./CD_Test ../Instances/cd_small.dzn
```

## Instance format (`.dzn`)

```
InboundTrucks = 4;
OutboundTrucks = 3;
ReleaseTime    = [0, 2, 1, 3];
UnloadTime     = [3, 2, 4, 2];
LoadTime       = [2, 3, 2];
TransferTime   = [| 1,2,1 | 2,1,3 | 1,1,2 | 3,2,1 |];
```
