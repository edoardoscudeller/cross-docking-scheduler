#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>
#include <limits>

// Funzione ausiliaria: calcola il makespan parziale outbound dato:
//   - finish_unload[i] : completamento scarico di ogni truck inbound i
//   - partial_seq      : sequenza parziale outbound (può essere di lunghezza < m)
//
// Usata nella fase NEH per valutare ogni posizione di inserimento.
// Non dipende dal numero di porte: funziona per qualsiasi n e m.

static unsigned ComputePartialOutboundMakespan(
    const CD_Input&          in,
    const vector<unsigned>&  finish_unload,
    const vector<unsigned>&  partial_seq)
{
  unsigned door_time = 0;
  unsigned makespan  = 0;

  for (unsigned pos = 0; pos < partial_seq.size(); pos++)
  {
    unsigned j = partial_seq[pos];

    // goods_ready[j] = max_i (finish_unload[i] + t[i][j])
    unsigned goods_ready = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready = max(goods_ready, finish_unload[i] + in.TransferTime(i, j));

    door_time = max(door_time, goods_ready) + in.LoadTime(j);
    makespan  = max(makespan, door_time);
  }
  return makespan;
}

// GreedyCDSolver  —  v0.2 Multi-Source Enhanced Greedy
//
// FASE 1 — Sequenza inbound  [Fonte 2 + 4]
//   Criterio primario  : ERT  (Earliest Release Time)  [invariato]
//   Tie-break 1        : r[i] + p[i]  (early completion proxy, da Chen & Song)
//                        Preferisce i truck che completano prima lo scarico
//                        rispetto al solo SPT.
//   Tie-break 2        : max_j(t[i][j]) crescente  (WSPT-inspired)
//                        I truck con transfer time piccoli verso tutti gli
//                        outbound vengono anticipati: "sboccano" la catena.
//
// FASE 2 — Simulazione goods_ready  [Fonte 1 — Boysen et al.]
//   Con la sequenza inbound fissata si calcola finish_unload[i] per ogni
//   inbound truck. Poi si calcola goods_ready[j] per ogni outbound truck.
//   Questo lega esplicitamente inbound e outbound.
//
// FASE 3 — Sequenza outbound: NEH-style insertion  [Fonte 3 — NEH + Fonte 1]
//   3a. Ordinamento iniziale: goods_ready[j] + q[j]  (stima completamento)
//       → Prima inseriamo i truck che presumibilmente finiscono per ultimi
//         (NEH inserisce per peso decrescente).
//   3b. Inserimento iterativo: per ogni truck (in ordine decrescente di
//       goods_ready[j]+q[j]) si prova ogni posizione della sequenza parziale
//       e si sceglie quella che minimizza il makespan parziale outbound.


void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // FASE 1 — Sequenza inbound
  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);

  // Peso WSPT per ogni truck inbound: max transfer time verso qualsiasi outbound
  // Truck con max_transfer piccolo → viene anticipato (sblocca prima gli outbound)
  vector<unsigned> max_transfer(in.InboundTrucks(), 0);
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
      max_transfer[i] = max(max_transfer[i], in.TransferTime(i, j));

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    // Criterio 1: ERT
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
      return in.ReleaseTime(a) < in.ReleaseTime(b);

    // Tie-break 1: early completion proxy r[i]+p[i]  (Chen & Song)
    unsigned ecp_a = in.ReleaseTime(a) + in.UnloadTime(a);
    unsigned ecp_b = in.ReleaseTime(b) + in.UnloadTime(b);
    if (ecp_a != ecp_b)
      return ecp_a < ecp_b;

    // Tie-break 2: WSPT — priorità a truck con transfer time piccoli
    return max_transfer[a] < max_transfer[b];
  });

  out.SetInboundSeq(in_seq);

  // FASE 2 — Calcolo finish_unload e goods_ready  (Boysen et al.)
  vector<unsigned> finish_unload(in.InboundTrucks());
  unsigned inbound_door_time = 0;

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned i = in_seq[pos];
    finish_unload[i] = max(inbound_door_time, in.ReleaseTime(i)) + in.UnloadTime(i);
    inbound_door_time = finish_unload[i];
  }

  // goods_ready[j]: momento in cui tutte le merci per j sono disponibili
  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready[j] = max(goods_ready[j], finish_unload[i] + in.TransferTime(i, j));

      
  // FASE 3 — Sequenza outbound: NEH-style insertion

  // 3a. Ordinamento iniziale per goods_ready[j] + q[j] decrescente
  //     (NEH inserisce prima i job "pesanti"/lenti — stima completamento alta)
  vector<unsigned> ranked(in.OutboundTrucks());
  iota(ranked.begin(), ranked.end(), 0);

  sort(ranked.begin(), ranked.end(), [&](unsigned a, unsigned b)
  {
    unsigned score_a = goods_ready[a] + in.LoadTime(a);
    unsigned score_b = goods_ready[b] + in.LoadTime(b);
    return score_a > score_b;  // decrescente: prima i "pesanti"
  });

  // 3b. Costruzione per inserimento iterativo (NEH adattato al cross-dock)
  vector<unsigned> out_seq;
  out_seq.reserve(in.OutboundTrucks());

  for (unsigned step = 0; step < in.OutboundTrucks(); step++)
  {
    unsigned j_new = ranked[step];

    // Prova tutte le posizioni di inserimento: 0 .. out_seq.size()
    unsigned best_pos      = 0;
    unsigned best_makespan = numeric_limits<unsigned>::max();

    for (unsigned pos = 0; pos <= out_seq.size(); pos++)
    {
      // Costruisce la sequenza parziale con j_new inserito in posizione pos
      vector<unsigned> candidate;
      candidate.reserve(out_seq.size() + 1);
      for (unsigned k = 0; k < pos; k++)        candidate.push_back(out_seq[k]);
      candidate.push_back(j_new);
      for (unsigned k = pos; k < out_seq.size(); k++) candidate.push_back(out_seq[k]);

      unsigned ms = ComputePartialOutboundMakespan(in, finish_unload, candidate);
      if (ms < best_makespan)
      {
        best_makespan = ms;
        best_pos      = pos;
      }
    }

    // Inserisce j_new nella posizione ottimale trovata
    out_seq.insert(out_seq.begin() + best_pos, j_new);
  }

  out.SetOutboundSeq(out_seq);
}