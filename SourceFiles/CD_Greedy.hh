#ifndef CD_GREEDY_HH
#define CD_GREEDY_HH
#include "CD_Data.hh"

// Greedy ERT + EGR:
//   Inbound  sequence: sort by Earliest Release Time (ERT), tie-break by SPT.
//   Outbound sequence: sort by Earliest Goods Ready (EGR), computed after
//                      simulating the inbound schedule. Tie-break by SPT (load_time).
void GreedyCDSolver(const CD_Input& in, CD_Output& out);

#endif