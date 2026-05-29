#include "CD_Data.hh"
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>

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
    // InboundDoors = D_IN;
    is >> buffer >> ch >> d_inbound >> ch;
    // OutboundDoors = D_OUT;
    is >> buffer >> ch >> d_outbound >> ch;

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
            is >> transfer_time[i][j] >> ch;
}

ostream& operator<<(ostream& os, const CD_Input& in)
{
  os << "InboundTrucks  = " << in.n_inbound  << ";\n";
  os << "OutboundTrucks = " << in.n_outbound << ";\n";
  os << "InboundDoors   = " << in.d_inbound  << ";\n";
  os << "OutboundDoors  = " << in.d_outbound << ";\n\n";

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
    outbound_seq(my_in.OutboundTrucks()),
    inbound_door(my_in.InboundTrucks(), 0),
    outbound_door(my_in.OutboundTrucks(), 0)
{
  Reset();
}

CD_Output& CD_Output::operator=(const CD_Output& other)
{
  inbound_seq   = other.inbound_seq;
  outbound_seq  = other.outbound_seq;
  inbound_door  = other.inbound_door;
  outbound_door = other.outbound_door;
  return *this;
}

void CD_Output::Reset()
{
  iota(inbound_seq.begin(),  inbound_seq.end(),  0);
  iota(outbound_seq.begin(), outbound_seq.end(), 0);
  fill(inbound_door.begin(),  inbound_door.end(),  0);
  fill(outbound_door.begin(), outbound_door.end(), 0);
}

// ComputeMakespan con porte multiple
//
// Modello EAD (Earliest Available Door):
//   Inbound: d_in porte parallele.
//     Per ogni truck nella sequenza si assegna la porta con finish_time minore.
//     finish_unload[i] = max(door_free[d_EAD], release_time[i]) + unload_time[i]
//
//   Outbound: d_out porte parallele, stessa politica EAD.
//     start[j] = max(door_free[d_EAD], goods_ready[j])
//     finish[j] = start[j] + load_time[j]
//
//   Makespan = max finish su tutti gli outbound truck.

unsigned CD_Output::ComputeMakespan() const
{
  // Step 1: finish_unload[i] con porte parallele
  vector<unsigned> finish_unload(in.InboundTrucks(), 0);
  vector<unsigned> door_free_in(in.InboundDoors(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.InboundDoors(); d++)
      if (door_free_in[d] < door_free_in[best_door]) 
        best_door = d;

    finish_unload[inbound_seq[pos]] = max(door_free_in[best_door], in.ReleaseTime(inbound_seq[pos])) + in.UnloadTime(inbound_seq[pos]);
    door_free_in[best_door] = finish_unload[inbound_seq[pos]];
  }

  // Step 2: goods_ready[j]
  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      goods_ready[j] = max(goods_ready[j], finish_unload[i] + in.TransferTime(i, j));

  // Step 3: makespan con porte parallele outbound
  vector<unsigned> door_free_out(in.OutboundDoors(), 0);
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned best_door = 0;
    for (unsigned d = 1; d < in.OutboundDoors(); d++)
      if (door_free_out[d] < door_free_out[best_door]) 
        best_door = d;

    door_free_out[best_door] = max(door_free_out[best_door], goods_ready[outbound_seq[pos]]) + in.LoadTime(outbound_seq[pos]);
    makespan = max(makespan, door_free_out[best_door]);
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