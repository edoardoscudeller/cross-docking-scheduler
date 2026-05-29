#include "CD_Greedy.hh"
#include <algorithm>
#include <numeric>

// ---------------------------------------------------------------------------
// GreedyCDSolver
//
// INBOUND  sequence: sort trucks by Earliest Release Time (ERT).
//   Ties broken by shorter unload time (SPT rule).
//
// OUTBOUND sequence: sort trucks by Shortest Processing Time (SPT).
//   Rationale: minimizes average completion time; good makespan heuristic.
//
// Multi-door: la simulazione inbound usa politica EAD (Earliest Available Door).
//   La logica di ordinamento rimane invariata.
// ---------------------------------------------------------------------------
void GreedyCDSolver(const CD_Input& in, CD_Output& out)
{
  // --- Inbound sequence: ERT, ties broken by SPT ---
  vector<unsigned> in_seq(in.InboundTrucks());
  iota(in_seq.begin(), in_seq.end(), 0);

  sort(in_seq.begin(), in_seq.end(), [&](unsigned a, unsigned b)
  {
    if (in.ReleaseTime(a) != in.ReleaseTime(b))
      return in.ReleaseTime(a) < in.ReleaseTime(b);
    return in.UnloadTime(a) < in.UnloadTime(b);
  });

  out.SetInboundSeq(in_seq);

  // --- Simulazione inbound con d_inbound porte parallele (EAD) ---
  vector<unsigned> door_free_in(in.InboundDoors(), 0);
  vector<unsigned> assigned_door_in(in.InboundTrucks(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
      if (door_free_in[d] < door_free_in[best_door])
        best_door = d;

    unsigned truck = in_seq[pos];
    door_free_in[best_door] =
      max(door_free_in[best_door], in.ReleaseTime(truck))
      + in.UnloadTime(truck);
    assigned_door_in[pos] = best_door;
  }

  out.SetInboundDoor(assigned_door_in);

  // --- Outbound sequence: SPT (shortest load time first) ---
  vector<unsigned> out_seq(in.OutboundTrucks());
  iota(out_seq.begin(), out_seq.end(), 0);

  sort(out_seq.begin(), out_seq.end(), [&](unsigned a, unsigned b)
  {
    return in.LoadTime(a) < in.LoadTime(b);
  });

  out.SetOutboundSeq(out_seq);
}