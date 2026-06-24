#include "CD_Data.hh"
#include <fstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cassert>

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

// Ricostruisce finish_unload dalla soluzione inbound memorizzata in CD_Output
// e calcola goods_ready[j] = max_i (finish_unload[i] + transfer_time[i][j]).
//
// Questo helper centralizza il calcolo usato sia dal greedy sia dal makespan,
// evitando duplicazione della logica di simulazione inbound + propagazione
// verso gli outbound truck.
// Solo le coppie (i,j) con transfer_time > 0 contribuiscono: lo zero indica
// assenza di connessione fisica tra l'inbound i e l'outbound j.
vector<unsigned> CD_Output::ComputeGoodsReadyFromCurrentInbound() const
{
  vector<unsigned> finish_unload(in.InboundTrucks(), 0);
  vector<unsigned> door_free_in(in.InboundDoors(), 0);

  for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
  {
    assert(inbound_door[pos] < in.InboundDoors());

    unsigned start_unload = max(door_free_in[inbound_door[pos]], in.ReleaseTime(inbound_seq[pos]));
    unsigned finish = start_unload + in.UnloadTime(inbound_seq[pos]);

    finish_unload[inbound_seq[pos]] = finish;
    door_free_in[inbound_door[pos]]   = finish;
  }

  vector<unsigned> goods_ready(in.OutboundTrucks(), 0);
  
  for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
      if (in.TransferTime(i, j) > 0)
        goods_ready[j] = max(goods_ready[j], finish_unload[i] + in.TransferTime(i, j));
  return goods_ready;
}


// ComputeMakespan — valuta la soluzione gia' costruita.
//
// Non ricalcola piu' sequencing e assegnazioni porte; usa:
//   - inbound_seq + inbound_door
//   - outbound_seq + outbound_door
//
// goods_ready e' ricavato dalla soluzione inbound memorizzata.
// Poi si simula il lato outbound seguendo esattamente le porte assegnate
// nella soluzione, ottenendo il makespan coerente con la soluzione stessa.

unsigned CD_Output::ComputeMakespan() const
{
  vector<unsigned> goods_ready = ComputeGoodsReadyFromCurrentInbound();

  vector<unsigned> door_free_out(in.OutboundDoors(), 0);
  unsigned makespan = 0;

  for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
  {
    unsigned start_load = max(door_free_out[outbound_door[pos]], goods_ready[outbound_seq[pos]]);
    unsigned finish = start_load + in.LoadTime(outbound_seq[pos]);

    door_free_out[outbound_door[pos]] = finish;
    makespan = max(makespan, finish);
  }
  return makespan;
}


// ComputeLowerBound — calcola un limite inferiore al makespan ottimo.
//
// L'idea di base: calcoliamo 4 "tempi minimi garantiti" che la soluzione
// non può MAI battere, indipendentemente da come ordini i camion.
// Il lower bound finale è il massimo tra i quattro.
//
// LB1 — "Quanto tardi può andare il camion outbound più sfortunato?"
//   Per ogni camion outbound j, chiediamo: qual è il prima assoluto
//   in cui la sua merce può essere pronta, nel caso migliore?
//   La risposta è: prendi l'inbound i che finisce di scaricare prima
//   e ha merce da mandare a j. Da quel momento, aggiungi il tempo
//   di carico di j. Il makespan non può essere meno del massimo
//   di questi valori su tutti i j.
//
// LB2 — "Le porte inbound sono un collo di bottiglia?"
//   Somma tutti i tempi di scarico degli inbound. Dividi per il numero
//   di porte inbound disponibili. Anche se le porte lavorassero sempre
//   senza pause e tutti i camion arrivassero subito, non puoi finire
//   prima di questo tempo.
//
// LB3 — "Le porte outbound sono un collo di bottiglia?"
//   Stesso ragionamento di LB2, ma sul lato uscita: somma tutti i
//   tempi di carico degli outbound e dividi per le porte outbound.
//
// LB4 — "Quanto tardi può andare il camion inbound più sfortunato?"
//   Per ogni camion inbound i, anche se lo scheduli per primo e usa
//   la porta subito libera, non può finire di scaricare e spedire la
//   merce prima di: release_time[i] + unload_time[i] + il trasferimento
//   minimo verso qualunque outbound. Il makespan non può essere meno
//   del massimo di questi valori su tutti gli i.

unsigned CD_Output::ComputeLowerBound() const
{
    // LB1 — Critical Path per ogni outbound truck j
    unsigned lb1 = 0;
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
    {
        unsigned best_j = UINT_MAX;
        for (unsigned i = 0; i < in.InboundTrucks(); i++)
        {
            if (in.TransferTime(i, j) > 0) 
            {
                unsigned candidate = in.ReleaseTime(i) + in.UnloadTime(i) + in.TransferTime(i, j);
                if (candidate < best_j)
                    best_j = candidate;
            }
        }
        if (best_j < UINT_MAX)
            lb1 = max(lb1, best_j + in.LoadTime(j));
    }

    // LB2 — Carico totale sulle porte inbound
    unsigned total_unload = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
        total_unload += in.UnloadTime(i);
    unsigned lb2 = ceil((double)total_unload / in.InboundDoors());

    // LB3 — Carico totale sulle porte outbound
    unsigned total_load = 0;
    for (unsigned j = 0; j < in.OutboundTrucks(); j++)
        total_load += in.LoadTime(j);
    unsigned lb3 = ceil((double)total_load / in.OutboundDoors());

    // LB4 — Critical Path per ogni inbound truck i
    unsigned lb4 = 0;
    for (unsigned i = 0; i < in.InboundTrucks(); i++)
    {
        unsigned min_transfer = UINT_MAX;
        for (unsigned j = 0; j < in.OutboundTrucks(); j++)
        {
            if (in.TransferTime(i, j) > 0 && in.TransferTime(i, j) < min_transfer)
                min_transfer = in.TransferTime(i, j);
        }
        if (min_transfer < UINT_MAX) 
        {
            unsigned path_i = in.ReleaseTime(i) + in.UnloadTime(i) + min_transfer;
            if (path_i > lb4)
                lb4 = path_i;
        }
    }
    // Ritorna il massimo assoluto tra i LB teorici
    return max({lb1, lb2, lb3, lb4});
}


// ComputeAverageWaits — calcola il tempo medio di attesa nel piazzale
// per i camion inbound (attesa prima di accedere a una porta di scarico)
// e per i camion outbound (attesa prima di accedere a una porta di carico).
//
// Wait_in[truck]  = start_unload - release_time[truck]
// Wait_out[truck] = start_load   - goods_ready[truck]
//
// Restituisce (avg_wait_in, avg_wait_out) in minuti.

pair<double, double> CD_Output::ComputeAverageWaits() const
{
    // ── Inbound waits ────────────────────────────────────────────────
    vector<unsigned> door_free_in(in.InboundDoors(), 0);
    double total_wait_in = 0;

    for (unsigned pos = 0; pos < in.InboundTrucks(); pos++)
    {
        unsigned truck = inbound_seq[pos];
        unsigned start_unload = max(door_free_in[inbound_door[pos]], in.ReleaseTime(truck));
        total_wait_in += (start_unload - in.ReleaseTime(truck));

        unsigned finish = start_unload + in.UnloadTime(truck);
        door_free_in[inbound_door[pos]] = finish;
    }

    // ── Outbound waits ───────────────────────────────────────────────
    vector<unsigned> goods_ready = ComputeGoodsReadyFromCurrentInbound();
    vector<unsigned> door_free_out(in.OutboundDoors(), 0);
    double total_wait_out = 0;

    for (unsigned pos = 0; pos < in.OutboundTrucks(); pos++)
    {
        unsigned start_load = max(door_free_out[outbound_door[pos]], goods_ready[outbound_seq[pos]]);
        total_wait_out += (start_load - goods_ready[outbound_seq[pos]]);

        unsigned finish = start_load + in.LoadTime(outbound_seq[pos]);
        door_free_out[outbound_door[pos]] = finish;
    }

    return { total_wait_in  / in.InboundTrucks(), total_wait_out / in.OutboundTrucks() };
}


ostream& operator<<(ostream& os, const CD_Output& out)
{
  unsigned max_in = std::min((unsigned)20, out.in.InboundTrucks());
  os << "Inbound sequence : ";
  for (unsigned i = 0; i < max_in; i++)
    os << out.inbound_seq[i] << " ";
  os << endl;

  os << "Inbound doors    : ";
  for (unsigned i = 0; i < max_in; i++)
    os << out.inbound_door[i] << " ";
  os << endl;

  unsigned max_out = std::min((unsigned)20, out.in.OutboundTrucks());
  os << "Outbound sequence: ";
  for (unsigned j = 0; j < max_out; j++)
    os << out.outbound_seq[j] << " ";
  os << endl;

  os << "Outbound doors   : ";
  for (unsigned j = 0; j < max_out; j++)
    os << out.outbound_door[j] << " ";
  os << endl;

  return os;
}