#ifndef CD_GREEDY_HH
#define CD_GREEDY_HH
#include "CD_Data.hh"

// Greedy ERT + EGR:
//   Inbound  sequence: sort by Earliest Release Time (ERT), tie-break by SPT.
//   Outbound sequence: sort by Earliest Goods Ready (EGR), computed after
//                      simulating the inbound schedule. Tie-break by SPT (load_time).
//
// La soluzione costruita e' completa:
//   - inbound sequence
//   - inbound door assignment
//   - outbound sequence
//   - outbound door assignment
void GreedyCDSolver(const CD_Input& in, CD_Output& out);

#endif