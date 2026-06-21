#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>

// GreedyCDSolver
//
// INBOUND sequence: sort trucks by Earliest Release Time (ERT).
//   Ties broken by shorter unload time (SPT rule).
//
// OUTBOUND sequence: sort trucks by Earliest Goods Ready (EGR).
//   goods_ready[j] = max over all inbound i of (finish_unload[i] + transfer_time[i][j])
//   Ties broken by shorter load time (SPT rule).
//   Rationale: rispetto alla v0.1 (SPT puro), l'ordine outbound dipende ora
//   dall'effettiva disponibilita' delle merci calcolata sulla sequenza inbound
//   fissata, riducendo i tempi di attesa alle porte outbound e il makespan.
//
// Multi-door: politica EAD (Earliest Available Door) su entrambi i lati.

void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // Step 1: Inbound sequence — ERT, tie-break SPT

  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
      return in.ReleaseTime(a) < in.ReleaseTime(b);
    return in.UnloadTime(a) < in.UnloadTime(b);
  });

  out.SetInboundSeq(in_seq);

  // Step 2: Simulazione inbound con d_inbound porte parallele (EAD)
  //         Calcola finish_unload[i] per ogni truck i (indicizzato per truck ID,
  //         non per posizione nella sequenza).

  vector<unsigned> finish_unload(in.InboundTrucks(), 0);
  vector<unsigned> door_free_in(in.InboundDoors(), 0);
  vector<unsigned> assigned_door_in(in.InboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
      if (door_free_in[d] < door_free_in[best_door])
        best_door = d;

    unsigned truck = in_seq[pos];
    finish_unload[truck] =
      max(door_free_in[best_door], in.ReleaseTime(truck))
      + in.UnloadTime(truck);
    door_free_in[best_door] = finish_unload[truck];
    assigned_door_in[pos]   = best_door;
  }

  out.SetInboundDoor(assigned_door_in);

  // Step 3: Calcola goods_ready[j] per ogni outbound truck j
  //         goods_ready[j] = max_i ( finish_unload[i] + transfer_time[i][j] )
  //         Rappresenta il momento in cui TUTTE le merci destinate a j
  //         sono disponibili sulla banchina, data la sequenza inbound fissata.

  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready[j] = max(goods_ready[j],
                           finish_unload[i] + in.TransferTime(i, j));

  // Step 4: Outbound sequence — EGR (Earliest Goods Ready), tie-break SPT
  //         Ordina per goods_ready[j] crescente: il truck outbound che puo'
  //         partire prima viene caricato per primo, minimizzando le attese
  //         alle porte outbound e quindi il makespan.

  vector<unsigned> out_seq(in.OutboundTrucks());
  iota(out_seq.begin(), out_seq.end(), 0);

  sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b)
  {
    if (goods_ready[a] != goods_ready[b])
      return goods_ready[a] < goods_ready[b];
    return in.LoadTime(a) < in.LoadTime(b);
  });

  out.SetOutboundSeq(out_seq);
}