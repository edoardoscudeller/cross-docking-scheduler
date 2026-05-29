#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>
#include <limits>

// Funzione ausiliaria: calcola il makespan parziale outbound dato:
//   - finish_unload[i] : completamento scarico di ogni truck inbound i
//   - partial_seq      : sequenza parziale outbound (può essere di lunghezza < m)
//
// Politica EAD (Earliest Available Door): ogni truck nella sequenza viene
// assegnato alla porta outbound che si libera prima.

static unsigned ComputePartialOutboundMakespan(
    const CD_Input&          in,
    const vector<unsigned>&  finish_unload,
    const vector<unsigned>&  partial_seq)
{
  vector<unsigned> door_free(in.OutboundDoors(), 0);
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < partial_seq.size(); pos++)
  {
    unsigned goods_ready = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready = max(goods_ready, finish_unload[i] + in.TransferTime(i, partial_seq[pos]));

    unsigned best_door = 0;
    for (unsigned d = 1; d < in.OutboundDoors(); d++)
      if (door_free[d] < door_free[best_door]) 
        best_door = d;

    door_free[best_door] = max(door_free[best_door], goods_ready) + in.LoadTime(partial_seq[pos]);
    makespan = max(makespan, door_free[best_door]);
  }
  return makespan;
}

// GreedyCDSolver  —  v0.5 Composite-Score Greedy, multi-door
//
// FASE 1 — Sequenza inbound  [Fonte 2 + 4]   (invariata rispetto a v0.4)
//
//   Score composito:
//     score(i) = r[i] + p[i] + weighted_impact(i)
//   con guardia ERT conservativa (soglia = media p[i]).
//
// FASE 2 — Simulazione inbound con d_in porte parallele  [NOVITÀ v0.5]
//   Politica EAD: ogni truck viene assegnato alla porta inbound
//   con finish_time minore al momento dell'assegnazione.
//   finish_unload[i] = max(door_free[d_EAD], r[i]) + p[i]
//
// FASE 3 — goods_ready e sequenza outbound: NEH-style insertion  [Fonte 3]
//   ComputePartialOutboundMakespan ora usa d_out porte parallele (EAD).


void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // FASE 1 — Sequenza inbound
  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);

  // Precompute weighted_impact  [Fonte 4 — Yu & Egbelu 2008]
  vector<double> weighted_impact(in.InboundTrucks(), 0.0);
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
  {
    double num = 0.0;
    double den = 0.0;
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    {
      double tij = in.TransferTime(i, j);
      num += tij * in.LoadTime(j);
      den += tij;
    }
    weighted_impact[i] = (den > 0.0) ? (num / den) : 0.0;
  }

  // Score composito  [Fonte 2 + 4]
  vector<double> score(in.InboundTrucks());
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
    score[i] = in.ReleaseTime(i) + in.UnloadTime(i) + weighted_impact[i];

  // Soglia conservativa ERT
  double avg_unload = 0.0;
  for (unsigned i = 0; i < in.InboundTrucks(); i++)
    avg_unload += in.UnloadTime(i);
  avg_unload /= in.InboundTrucks();

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    double ert_diff = in.ReleaseTime(a) - in.ReleaseTime(b);
    if (std::abs(ert_diff) > avg_unload)
      return in.ReleaseTime(a) < in.ReleaseTime(b);
    return score[a] < score[b];
  });

  out.SetInboundSeq(in_seq);

  // FASE 2 — Calcolo finish_unload con d_in porte parallele  [NOVITÀ v0.5]
  vector<unsigned> finish_unload(in.InboundTrucks(), 0);
  vector<unsigned> door_free_in(in.InboundDoors(), 0);
  vector<unsigned> assigned_door_in(in.InboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
      if (door_free_in[d] < door_free_in[best_door]) 
        best_door = d;

    finish_unload[in_seq[pos]]  = max(door_free_in[best_door], in.ReleaseTime(in_seq[pos])) + in.UnloadTime(in_seq[pos]);
    door_free_in[best_door] = finish_unload[in_seq[pos]];
    assigned_door_in[pos]   = best_door;
  }

  out.SetInboundDoor(assigned_door_in);

  // goods_ready[j]  (Boysen et al.)
  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready[j] = max(goods_ready[j], finish_unload[i] + in.TransferTime(i, j));


  // FASE 3 — Sequenza outbound: NEH-style insertion con d_out porte

  // 3a. Ordinamento iniziale per goods_ready[j] + q[j] decrescente
  vector<unsigned> ranked(in.OutboundTrucks());
  iota(ranked.begin(), ranked.end(), 0);
  sort(ranked.begin(), ranked.end(), [&](unsigned a, unsigned b)
  {
    unsigned score_a = goods_ready[a] + in.LoadTime(a);
    unsigned score_b = goods_ready[b] + in.LoadTime(b);
    return score_a > score_b;
  });

  // 3b. Inserimento iterativo (NEH adattato al cross-dock, multi-door)
  vector<unsigned> out_seq;
  out_seq.reserve(in.OutboundTrucks());

  for (unsigned step = 0; step < in.OutboundTrucks(); step++)
  {
    unsigned best_pos = 0;
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
  
  out.SetOutboundSeq(out_seq);

  // Registra assegnazione porte outbound (EAD, per output informativo)
  const unsigned d_out = in.OutboundDoors();
  vector<unsigned> door_free_out(d_out, 0);
  vector<unsigned> assigned_door_out(in.OutboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < d_out; d++)
      if (door_free_out[d] < door_free_out[best_door]) best_door = d;

    door_free_out[best_door] = max(door_free_out[best_door], goods_ready[out_seq[pos]]) + in.LoadTime(out_seq[pos]);
    assigned_door_out[pos]   = best_door;
  }
  out.SetOutboundDoor(assigned_door_out);
}