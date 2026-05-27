#ifndef CD_GREEDY_HH
#define CD_GREEDY_HH
#include "CD_Data.hh"

// ---------------------------------------------------------------------------
// GreedyCDSolver  —  versione v0.2 (Multi-Source Enhanced Greedy)
//
// Algoritmo composito derivato da:
//   [1] Boysen, Fliedner & Scholl (2010) — OR Spectrum 32(1)
//       goods_ready come criterio primario per l'outbound
//   [2] Chen & Song (2009/2013) — Discrete Applied Mathematics
//       tie-break inbound basato su r[i]+p[i] (early completion proxy)
//   [3] Nawaz, Enscore & Ham (1983) — NEH heuristic adattato al cross-dock
//       inserimento iterativo della sequenza outbound con valutazione
//       del makespan parziale
//   [4] WSPT con release times — peso inbound = max_j(t[i][j])
//       I truck con transfer time piccoli vengono prioritizzati
//
// Complessità:
//   Inbound  : O(n log n)
//   Outbound : O(m² × n)   [NEH-style insertion: m passi, ognuno O(m×n)]
//   Totale   : O(n log n + m² × n)
//
// Scalabilità: l'algoritmo funziona per qualsiasi n e m.
// ---------------------------------------------------------------------------
void GreedyCDSolver(const CD_Input& in, CD_Output& out);

#endif