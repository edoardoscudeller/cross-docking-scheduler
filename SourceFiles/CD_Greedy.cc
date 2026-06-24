#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>

// GreedyCDSolver
//
// INBOUND sequence: sort trucks by Earliest Release Time (ERT).
//   Ties broken by shorter unload time (SPT rule).
//
// INBOUND doors: politica EAD (Earliest Available Door).
//
// OUTBOUND sequence: sort trucks by Earliest Goods Ready (EGR).
//   goods_ready[j] = max over all inbound i of (finish_unload[i] + transfer_time[i][j])
//   Ties broken by shorter load time (SPT rule).
//
// OUTBOUND doors: politica EAD (Earliest Available Door) basata su goods_ready.
//
// La soluzione prodotta e' completa e puo' essere valutata direttamente da
// ComputeMakespan() senza ricalcolare sequencing e assegnazioni.

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

  // Step 2: Inbound doors — EAD sulla sequenza inbound fissata
  vector<unsigned> door_free_in(in.InboundDoors(), 0);
  vector<unsigned> assigned_door_in(in.InboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
      if (door_free_in[d] < door_free_in[best_door])
        best_door = d;

    unsigned truck = in_seq[pos];
    unsigned start_unload =
      max(door_free_in[best_door], in.ReleaseTime(truck));
    unsigned finish_unload =
      start_unload + in.UnloadTime(truck);

    door_free_in[best_door] = finish_unload;
    assigned_door_in[pos]   = best_door;
  }

  out.SetInboundDoor(assigned_door_in);

  // Step 3: Goods ready dalla soluzione inbound appena costruita
  vector<unsigned> goods_ready = out.ComputeGoodsReadyFromCurrentInbound();

  // Step 4: Outbound sequence — EGR, tie-break SPT
  vector<unsigned> out_seq(in.OutboundTrucks());
  iota(out_seq.begin(), out_seq.end(), 0);

  sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b)
  {
    if (goods_ready[a] != goods_ready[b])
      return goods_ready[a] < goods_ready[b];
    return in.LoadTime(a) < in.LoadTime(b);
  });

  out.SetOutboundSeq(out_seq);

  // Step 5: Outbound doors — EAD sulla sequenza outbound fissata
  vector<unsigned> door_free_out(in.OutboundDoors(), 0);
  vector<unsigned> assigned_door_out(in.OutboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.OutboundDoors(); d++)
      if (door_free_out[d] < door_free_out[best_door])
        best_door = d;

    unsigned truck = out_seq[pos];
    unsigned start_load =
      max(door_free_out[best_door], goods_ready[truck]);
    unsigned finish_load =
      start_load + in.LoadTime(truck);

    door_free_out[best_door] = finish_load;
    assigned_door_out[pos]   = best_door;
  }

  out.SetOutboundDoor(assigned_door_out);
}