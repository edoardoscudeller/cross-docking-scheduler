#ifndef CD_GREEDY_HH
#define CD_GREEDY_HH
#include "CD_Data.hh"

// ---------------------------------------------------------------------------
// GreedyCDSolver  —  versione v0.5 (Composite-Score Greedy, multi-door)
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
// NOVITÀ v0.5 — Porte multiple:
//   La simulazione inbound e outbound usa d_in / d_out porte parallele.
//   L'assegnazione alla porta segue la politica EAD (Earliest Available Door):
//   ogni truck viene assegnato alla porta che si libera prima.
//   ComputePartialOutboundMakespan è aggiornato di conseguenza.
// ---------------------------------------------------------------------------

void GreedyCDSolver(const CD_Input& in, CD_Output& out);

#endif