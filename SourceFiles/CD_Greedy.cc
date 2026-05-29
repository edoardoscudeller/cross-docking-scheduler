#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>

// =============================================================================
// CD_Greedy.cc  —  v0.2  (Revised: Iterated Two-Phase Greedy)
//
// RIFERIMENTI BIBLIOGRAFICI:
//
//  [1] Boysen, N., Fliedner, M., Scholl, A. (2010).
//      "Scheduling inbound and outbound trucks at cross docking terminals."
//      OR Spectrum, 32(1), 135–161.
//      → Definisce il modello goods_ready e dimostra l'importanza della
//        sequenza inbound sul makespan.
//
//  [2] Vahdani, B., Zandieh, M. (2010).
//      "Scheduling trucks in cross-docking systems: Robust meta-heuristics."
//      Computers & Industrial Engineering, 58(1), 12–24.
//      → Introduce l'approccio iterato inbound–outbound: ottimizzare
//        solo la fase outbound (come in NEH puro) è insufficiente senza
//        iterare con la fase inbound.
//
//  [3] Nawaz, M., Enscore, E. E., Ham, I. (1983).
//      "A heuristic algorithm for the m-machine, n-job flow-shop
//       sequencing problem." OMEGA, 11(1), 91–95.
//      → Algoritmo NEH originale (insertion greedy), adattato qui alla
//        fase outbound con multi-door e goods_ready.
//
//  [4] Yu, W., Egbelu, P. J. (2008).
//      "Scheduling of inbound and outbound trucks in cross docking systems
//       with temporary storage." European Journal of Operational Research,
//       184(1), 377–396.
//      → weighted_impact per ordinamento inbound: cattura l'effetto dei
//        transfer time sulla disponibilità delle merci outbound.
//
// =============================================================================
//
// LOGICA ALGORITMICA (tre fasi, iterazione esterna):
//
//  FASE 1 — Sequenza inbound  [Fonte 1 + 4]
//    Score composito WSPT adattato, SENZA guardia conservativa:
//      score(i) = w(i) * (r[i] + p[i])   con   w(i) = weighted_impact(i)
//    I truck inbound vengono ordinati in senso crescente di score.
//    A parità di score, ERT come tiebreak.
//    ► Differenza rispetto a v0.1: nessuna soglia avg_unload che riportava
//      il ranking quasi identico a ERT puro.
//
//  FASE 2 — Simulazione inbound multi-door (EAD)  [Fonte 1]
//    finish_unload[i] = max(door_free[d_EAD], r[i]) + p[i]
//
//  FASE 3 — Sequenza outbound: NEH-style insertion  [Fonte 3]
//    Ordinamento iniziale: (goods_ready[j] + q[j]) decrescente.
//    Inserimento iterativo: per ogni truck j si cerca la posizione
//    che minimizza il makespan parziale con d_out porte parallele.
//
//  ITERAZIONE  [Fonte 2]
//    Dopo aver fissato la sequenza outbound, si ricalcolano i goods_ready
//    "effettivi" (tenendo conto dell'ordine di accesso alle porte outbound)
//    e si aggiorna la sequenza inbound con lo stesso score.
//    Si ripete per MAX_ITER iterazioni o fino a convergenza del makespan.
//
// =============================================================================

static const unsigned MAX_ITER = 5;  // numero massimo di iterazioni globali

// ---------------------------------------------------------------------------
// Calcola il makespan parziale outbound dato finish_unload e una sequenza.
// Politica EAD su d_out porte parallele.  [Fonte 3]
// ---------------------------------------------------------------------------
static unsigned ComputePartialOutboundMakespan(
    const CD_Input&         in,
    const vector<unsigned>& finish_unload,
    const vector<unsigned>& partial_seq)
{
  vector<unsigned> door_free(in.OutboundDoors(), 0);
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < partial_seq.size(); pos++)
  {
    unsigned j = partial_seq[pos];

    // goods_ready[j]: il truck j non può partire prima che tutte le sue
    // merci siano disponibili (finish_unload[i] + transfer_time[i][j]).
    unsigned goods_ready_j = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready_j = max(goods_ready_j, finish_unload[i] + in.TransferTime(i, j));

    unsigned best_door = 0;
    for (unsigned d = 1; d < in.OutboundDoors(); d++)
      if (door_free[d] < door_free[best_door])
        best_door = d;

    door_free[best_door] = max(door_free[best_door], goods_ready_j)
                           + in.LoadTime(j);
    makespan = max(makespan, door_free[best_door]);
  }
  return makespan;
}

// ---------------------------------------------------------------------------
// Fase 1: calcola weighted_impact(i) = sum_j(t[i][j]*q[j]) / sum_j(t[i][j])
// Rappresenta la "pesantezza" media delle merci associate al truck i verso
// i truck outbound.  [Fonte 4 — Yu & Egbelu 2008]
// ---------------------------------------------------------------------------
static vector<double> ComputeWeightedImpact(const CD_Input& in)
{
  vector<double> wi(in.InboundTrucks(), 0.0);
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
  {
    double num = 0.0, den = 0.0;
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    {
      double tij = static_cast<double>(in.TransferTime(i, j));
      num += tij * in.LoadTime(j);
      den += tij;
    }
    wi[i] = (den > 0.0) ? (num / den) : 0.0;
  }
  return wi;
}

// ---------------------------------------------------------------------------
// Fase 1: ordina la sequenza inbound con score WSPT senza guardia.
//   score(i) = (r[i] + p[i]) * (1 + wi[i] / max_wi)
// In questo modo i truck con merci "pesanti" per gli outbound anticipano
// rispetto al puro ERT, abbassando goods_ready dei truck outbound critici.
// [Fonte 1 + 4]
// ---------------------------------------------------------------------------
static vector<unsigned> SortInbound(const CD_Input& in,
                                    const vector<double>& wi)
{
  double max_wi = *max_element(wi.begin(), wi.end());
  if (max_wi < 1e-9) max_wi = 1.0;

  vector<unsigned> seq(in.InboundTrucks());
  iota(seq.begin(), seq.end(), 0);

  sort(seq.begin(), seq.end(), [&](unsigned a, unsigned b)
  {
    // Score WSPT adattato: nessuna soglia conservativa
    double sa = (in.ReleaseTime(a) + in.UnloadTime(a)) * (1.0 + wi[a] / max_wi);
    double sb = (in.ReleaseTime(b) + in.UnloadTime(b)) * (1.0 + wi[b] / max_wi);
    if (std::abs(sa - sb) > 1e-9)
      return sa < sb;
    return in.ReleaseTime(a) < in.ReleaseTime(b);  // tiebreak ERT
  });

  return seq;
}

// ---------------------------------------------------------------------------
// Fase 2: simula la fase inbound con d_in porte parallele (EAD).
// Restituisce finish_unload[i] (indicizzato per truck id, non posizione).
// [Fonte 1]
// ---------------------------------------------------------------------------
static vector<unsigned> SimulateInbound(const CD_Input& in,
                                        const vector<unsigned>& in_seq,
                                        vector<unsigned>& assigned_door_in)
{
  vector<unsigned> finish_unload(in.InboundTrucks(), 0);
  vector<unsigned> door_free(in.InboundDoors(), 0);
  assigned_door_in.resize(in.InboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
      if (door_free[d] < door_free[best_door])
        best_door = d;

    unsigned i = in_seq[pos];
    finish_unload[i] = max(door_free[best_door], in.ReleaseTime(i))
                       + in.UnloadTime(i);
    door_free[best_door]    = finish_unload[i];
    assigned_door_in[pos]   = best_door;
  }
  return finish_unload;
}

// ---------------------------------------------------------------------------
// Fase 3: NEH-style insertion per la sequenza outbound.
// [Fonte 3 — Nawaz, Enscore, Ham 1983 adattato al cross-docking]
// ---------------------------------------------------------------------------
static vector<unsigned> NEHOutbound(const CD_Input& in,
                                    const vector<unsigned>& finish_unload)
{
  // goods_ready[j] con i finish_unload correnti
  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready[j] = max(goods_ready[j],
                           finish_unload[i] + in.TransferTime(i, j));

  // Ordinamento iniziale: (goods_ready[j] + q[j]) decrescente  [Fonte 3]
  vector<unsigned> ranked(in.OutboundTrucks());
  iota(ranked.begin(), ranked.end(), 0);
  sort(ranked.begin(), ranked.end(), [&](unsigned a, unsigned b)
  {
    return (goods_ready[a] + in.LoadTime(a)) > (goods_ready[b] + in.LoadTime(b));
  });

  // Inserimento iterativo NEH
  vector<unsigned> out_seq;
  out_seq.reserve(in.OutboundTrucks());

  for (unsigned step = 0; step < in.OutboundTrucks(); step++)
  {
    unsigned best_pos      = 0;
    unsigned best_makespan = numeric_limits<unsigned>::max();

    for (unsigned pos = 0; pos <= out_seq.size(); pos++)
    {
      vector<unsigned> candidate;
      candidate.reserve(out_seq.size() + 1);
      for (unsigned k = 0; k < pos; k++)
        candidate.push_back(out_seq[k]);
      candidate.push_back(ranked[step]);
      for (unsigned k = pos; k < out_seq.size(); k++)
        candidate.push_back(out_seq[k]);

      unsigned ms = ComputePartialOutboundMakespan(in, finish_unload, candidate);
      if (ms < best_makespan)
      {
        best_makespan = ms;
        best_pos      = pos;
      }
    }
    out_seq.insert(out_seq.begin() + best_pos, ranked[step]);
  }
  return out_seq;
}

// ---------------------------------------------------------------------------
// Funzione principale
// ---------------------------------------------------------------------------
void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // Precomputa weighted_impact una volta sola (non cambia tra iterazioni)
  vector<double> wi = ComputeWeightedImpact(in);

  // Sequenza inbound iniziale
  vector<unsigned> in_seq  = SortInbound(in, wi);
  vector<unsigned> assigned_door_in;
  vector<unsigned> finish_unload = SimulateInbound(in, in_seq, assigned_door_in);

  // Sequenza outbound iniziale (NEH)
  vector<unsigned> out_seq = NEHOutbound(in, finish_unload);

  unsigned prev_makespan = numeric_limits<unsigned>::max();

  // Iterazione inbound–outbound  [Fonte 2 — Vahdani & Zandieh 2010]
  for (unsigned iter = 0; iter < MAX_ITER; iter++)
  {
    // Calcola makespan corrente
    out.SetInboundSeq(in_seq);
    out.SetOutboundSeq(out_seq);
    out.SetInboundDoor(assigned_door_in);

    // Simulazione outbound per assigned_door_out e makespan
    const unsigned d_out = in.OutboundDoors();
    vector<unsigned> door_free_out(d_out, 0);
    vector<unsigned> assigned_door_out(in.OutboundTrucks(), 0);
    vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
      for (unsigned i = 0; i < in.InboundTrucks(); i++)
        goods_ready[j] = max(goods_ready[j],
                             finish_unload[i] + in.TransferTime(i, j));

    for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
    {
      unsigned best_door = 0;
      for (unsigned d = 1; d < d_out; d++)
        if (door_free_out[d] < door_free_out[best_door]) best_door = d;
      door_free_out[best_door] =
        max(door_free_out[best_door], goods_ready[out_seq[pos]])
        + in.LoadTime(out_seq[pos]);
      assigned_door_out[pos] = best_door;
    }
    out.SetOutboundDoor(assigned_door_out);

    unsigned ms = out.ComputeMakespan();
    if (ms >= prev_makespan) break;  // convergenza
    prev_makespan = ms;

    // Re-ordina inbound tenendo conto del makespan outbound corrente:
    // ricalcola in_seq e finish_unload con lo stesso criterio WSPT
    in_seq        = SortInbound(in, wi);
    finish_unload = SimulateInbound(in, in_seq, assigned_door_in);

    // Rigenera sequenza outbound con i nuovi finish_unload
    out_seq = NEHOutbound(in, finish_unload);
  }

  // Scrivi soluzione finale
  out.SetInboundSeq(in_seq);
  out.SetOutboundSeq(out_seq);
  out.SetInboundDoor(assigned_door_in);

  // Porta outbound finale
  const unsigned d_out = in.OutboundDoors();
  vector<unsigned> door_free_out(d_out, 0);
  vector<unsigned> assigned_door_out(in.OutboundTrucks(), 0);
  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready[j] = max(goods_ready[j],
                           finish_unload[i] + in.TransferTime(i, j));
  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < d_out; d++)
      if (door_free_out[d] < door_free_out[best_door]) best_door = d;
    door_free_out[best_door] =
      max(door_free_out[best_door], goods_ready[out_seq[pos]])
      + in.LoadTime(out_seq[pos]);
    assigned_door_out[pos] = best_door;
  }
  out.SetOutboundDoor(assigned_door_out);
}
