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

// GreedyCDSolver  —  v0.4 Composite-Score Greedy
//
// FASE 1 — Sequenza inbound  [Fonte 2 + 4]
//
//   Score composito (criterio UNICO, sostituisce la gerarchia ERT + tie-break):
//
//     score(i) = r[i] + p[i] + weighted_impact(i)
//
//   dove:
//     weighted_impact(i) = sum_j( t[i][j] * q[j] ) / sum_j( t[i][j] )
//
//   Interpretazione:
//     - r[i] + p[i]          : stima del completamento scarico (Chen & Song)
//     - weighted_impact(i)   : load time outbound medio ponderato per il
//                              transfer time (Yu & Egbelu 2008)
//                              Penalizza i truck che alimentano outbound lenti.
//   Ordine CRESCENTE: il truck con score minore va per primo.
//
//   Variante conservativa con soglia ERT:
//     Se la differenza di release time tra due truck supera la media dei
//     tempi di scarico (avg_unload), l'ERT puro prevale. Questo evita di
//     ritardare eccessivamente un truck con release time molto basso solo
//     perché ha un weighted_impact alto — violazione grave in scenari
//     "urgent" e "asymmetric" con release times molto dispersi.
//
//   Nota sul cambio rispetto a v0.3:
//     In v0.3 weighted_impact era usato come tie-break (terzo livello),
//     quindi non modificava quasi mai la sequenza. In v0.4 diventa parte
//     integrante del criterio primario: agisce su tutti i truck, non solo
//     sui pareggi ERT + ECP.
//
// FASE 2 — Simulazione goods_ready  [Fonte 1 — Boysen et al.]  [invariata]
//   Con la sequenza inbound fissata si calcola finish_unload[i] per ogni
//   inbound truck. Poi si calcola goods_ready[j] per ogni outbound truck.
//
// FASE 3 — Sequenza outbound: NEH-style insertion  [Fonte 3 — NEH]  [invariata]
//   3a. Ordinamento iniziale: goods_ready[j] + q[j] decrescente.
//   3b. Inserimento iterativo con valutazione del makespan parziale.


void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // FASE 1 — Sequenza inbound
  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);

  // Precompute weighted_impact per ogni truck inbound  [Fonte 4 — Yu & Egbelu 2008]
  //
  // weighted_impact[i] = sum_j( t[i][j] * q[j] ) / sum_j( t[i][j] )
  //
  // Caso degenere: se sum_j(t[i][j]) == 0, il valore è 0.0 (nessun impatto).
  // Usato come double per evitare troncamento integer nella divisione.
  vector<double> weighted_impact(in.InboundTrucks(), 0.0);
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
  {
    double num = 0.0;  // sum_j( t[i][j] * q[j] )
    double den = 0.0;  // sum_j( t[i][j] )
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    {
      double tij = static_cast<double>(in.TransferTime(i, j));
      num += tij * static_cast<double>(in.LoadTime(j));
      den += tij;
    }
    weighted_impact[i] = (den > 0.0) ? (num / den) : 0.0;
  }

  // Score composito per ogni truck inbound  [Fonte 2 + 4]
  //
  // score[i] = r[i] + p[i] + weighted_impact[i]
  //
  // Combina il completion proxy (Chen & Song) con l'impatto outbound
  // ponderato (Yu & Egbelu). Un valore basso indica un truck "leggero"
  // che finisce presto e non blocca outbound costosi → va servito prima.
  vector<double> score(in.InboundTrucks());
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
    score[i] = static_cast<double>(in.ReleaseTime(i) + in.UnloadTime(i))
               + weighted_impact[i];

  // Soglia conservativa per la guardia ERT:
  // se due truck differiscono in release time di più della media dei
  // tempi di scarico, l'ERT puro prevale sullo score composito.
  // Questo protegge scenari con release times molto dispersi (urgent,
  // asymmetric) dove ritardare un truck con r[i] basso sarebbe penalizzante.
  double avg_unload = 0.0;
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
    avg_unload += static_cast<double>(in.UnloadTime(i));
  avg_unload /= static_cast<double>(in.InboundTrucks());

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    // Guardia ERT conservativa: se la differenza di release time supera
    // la soglia, il truck con ERT minore ha priorità assoluta.
    double ert_diff = static_cast<double>(in.ReleaseTime(a))
                    - static_cast<double>(in.ReleaseTime(b));
    if (std::abs(ert_diff) > avg_unload)
      return in.ReleaseTime(a) < in.ReleaseTime(b);

    // Criterio composito (v0.4): score crescente  (Yu & Egbelu + Chen & Song)
    // Agisce su tutti i truck con ERT simile, non solo sui pareggi esatti.
    return score[a] < score[b];
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