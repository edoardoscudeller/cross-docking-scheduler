#ifndef CD_GREEDY_HH
#define CD_GREEDY_HH
#include "CD_Data.hh"

// ---------------------------------------------------------------------------
// GreedyCDSolver  —  versione v0.4 (Composite-Score Greedy)
//
// Algoritmo composito derivato da:
//   [1] Boysen, Fliedner & Scholl (2010) — OR Spectrum 32(1)
//       goods_ready come criterio primario per l'outbound
//   [2] Chen & Song (2009/2013) — Discrete Applied Mathematics
//       early completion proxy r[i]+p[i] come base dello score inbound
//   [3] Nawaz, Enscore & Ham (1983) — NEH heuristic adattato al cross-dock
//       inserimento iterativo della sequenza outbound con valutazione
//       del makespan parziale
//   [4] Yu & Egbelu (2008) — European Journal of Operations Research 184(1)
//       Score composito inbound (criterio UNICO, non tie-break):
//         score(i) = r[i] + p[i] + weighted_impact(i)
//       dove:
//         weighted_impact(i) = sum_j( t[i][j] * q[j] ) / sum_j( t[i][j] )
//       Ordine crescente: il truck con completion + impatto outbound minore
//       viene servito prima.
//       Variante conservativa: se due truck differiscono in ERT di più della
//       soglia (media p[i]), l'ERT puro prevale per evitare violazioni gravi
//       del vincolo di release time.
//
// Complessità:
//   Inbound  : O(n log n)  [precompute weighted_impact + score: O(n*m)]
//   Outbound : O(m² × n)   [NEH-style insertion: m passi, ognuno O(m×n)]
//   Totale   : O(n*m + n log n + m² × n)  ≃  O(m² × n)
//
// Scalabilità: l'algoritmo funziona per qualsiasi n e m.
// ---------------------------------------------------------------------------
void GreedyCDSolver(const CD_Input& in, CD_Output& out);

#endif