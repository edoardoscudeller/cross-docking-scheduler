#include "CD_Data.hh"
#include <fstream>
#include <algorithm>
#include <numeric>

CD_Input::CD_Input(string file_name)
{
    const unsigned MAX_DIM = 256;
    unsigned i, j;
    char ch, buffer[MAX_DIM];

    ifstream is(file_name);
    if (!is)
    {
        cerr << "Cannot open input file: " << file_name << endl;
        exit(1);
    }

    // InboundTrucks = N;
    is >> buffer >> ch >> n_inbound >> ch;
    // OutboundTrucks = M;
    is >> buffer >> ch >> n_outbound >> ch;

    release_time.resize(n_inbound);
    unload_time.resize(n_inbound);
    load_time.resize(n_outbound);
    transfer_time.resize(n_inbound, vector<unsigned>(n_outbound));

    // ReleaseTime
    is.ignore(MAX_DIM, '[');
    for (i = 0; i < n_inbound; i++)
        is >> release_time[i] >> ch;

    // UnloadTime
    is.ignore(MAX_DIM, '[');
    for (i = 0; i < n_inbound; i++)
        is >> unload_time[i] >> ch;

    // LoadTime 
    is.ignore(MAX_DIM, '[');
    for (j = 0; j < n_outbound; j++)
        is >> load_time[j] >> ch;

    // TransferTime
    is.ignore(MAX_DIM, '[');
    is >> ch;  // consume first '|'
    for (i = 0; i < n_inbound; i++)
        for (j = 0; j < n_outbound; j++)
            is >> transfer_time[i][j] >> ch;  // value then ',' or '|'
}

ostream& operator<<(ostream& os, const CD_Input& in)
{
  os << "InboundTrucks  = " << in.n_inbound  << ";\n";
  os << "OutboundTrucks = " << in.n_outbound << ";\n\n";

  os << "ReleaseTime = [";
  for (unsigned i = 0; i < in.n_inbound; i++)
    os << in.release_time[i] << (i+1 < in.n_inbound ? ", " : "];\n");

  os << "UnloadTime  = [";
  for (unsigned i = 0; i < in.n_inbound; i++)
    os << in.unload_time[i] << (i+1 < in.n_inbound ? ", " : "];\n");

  os << "LoadTime    = [";
  for (unsigned j = 0; j < in.n_outbound; j++)
    os << in.load_time[j] << (j+1 < in.n_outbound ? ", " : "];\n");

  os << "TransferTime = [|\n";
  for (unsigned i = 0; i < in.n_inbound; i++)
  {
    os << "  ";
    for (unsigned j = 0; j < in.n_outbound; j++)
      os << in.transfer_time[i][j] << (j+1 < in.n_outbound ? "," : "|");
    os << "\n";
  }
  os << "];\n";

  return os;
}



CD_Output::CD_Output(const CD_Input& my_in)
  : in(my_in),
    inbound_seq(my_in.InboundTrucks()),
    outbound_seq(my_in.OutboundTrucks())
{
  Reset();
}

CD_Output& CD_Output::operator=(const CD_Output& other)
{
  inbound_seq  = other.inbound_seq;
  outbound_seq = other.outbound_seq;
  return *this;
}

void CD_Output::Reset()
{
  // Identity permutations: 0, 1, 2, ...
  iota(inbound_seq.begin(),  inbound_seq.end(),  0);
  iota(outbound_seq.begin(), outbound_seq.end(), 0);
}

// ComputeMakespan
// Simulation of the cross-dock schedule:
//   1. Inbound trucks dock in inbound_seq order (one door, sequential).
//      finish_unload[i] = max(finish_unload[prev], release_time[i]) + unload_time[i]
//   2. For each outbound truck j (in outbound_seq order), it can start loading
//      only after ALL required goods have arrived (max transfer finish) AND
//      the outbound door is free.
//      We assume all goods from every inbound truck are needed by every outbound truck
//      (fully dense problem). Sparse variant: use a demand matrix.
//   3. Makespan = max completion time of all outbound trucks.

unsigned CD_Output::ComputeMakespan() const
{
  unsigned n = in.InboundTrucks();
  unsigned m = in.OutboundTrucks();

  // Step 1: compute finish time of each inbound truck
  vector<unsigned> finish_unload(n);
  unsigned door_free = 0;
  for (unsigned pos = 0; pos < n; pos++)
  {
    unsigned i = inbound_seq[pos];
    unsigned start = max(door_free, in.ReleaseTime(i));
    finish_unload[i] = start + in.UnloadTime(i);
    door_free = finish_unload[i];
  }

  // Step 2: compute completion time of each outbound truck
  unsigned out_door_free = 0;
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < m; pos++)
  {
    unsigned j = outbound_seq[pos];

    // Earliest time all goods for j are available on the floor
    unsigned goods_ready = 0;
    for (unsigned i = 0; i < n; i++)
      goods_ready = max(goods_ready, finish_unload[i] + in.TransferTime(i, j));

    unsigned start = max(out_door_free, goods_ready);
    unsigned finish = start + in.LoadTime(j);
    out_door_free = finish;
    makespan = max(makespan, finish);
  }

  return makespan;
}

ostream& operator<<(ostream& os, const CD_Output& out)
{
  os << "Inbound  sequence: ";
  for (unsigned pos = 0; pos < out.in.InboundTrucks(); pos++)
    os << out.inbound_seq[pos] << (pos+1 < out.in.InboundTrucks() ? " -> " : "\n");

  os << "Outbound sequence: ";
  for (unsigned pos = 0; pos < out.in.OutboundTrucks(); pos++)
    os << out.outbound_seq[pos] << (pos+1 < out.in.OutboundTrucks() ? " -> " : "\n");

  return os;
}
