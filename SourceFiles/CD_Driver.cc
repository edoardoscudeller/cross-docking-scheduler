#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <sstream>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

using namespace std;

struct ResultRecord
{
    string instance_name;
    unsigned makespan;
    double cpu_s;
    string original_seq;
    string inbound_seq;
    string outbound_seq;
};

// ---------------------------------------------------------------------------
// Estrae il nome base del file di istanza (senza path e senza .dzn)
// es: "../Instances/cd_n004_m003.dzn" -> "cd_n004_m003"
// ---------------------------------------------------------------------------
static string basename_no_ext(const string& path)
{
    size_t last_sep = path.find_last_of("/\\");
    string base = (last_sep == string::npos) ? path : path.substr(last_sep + 1);
    size_t dot = base.rfind('.');
    if (dot != string::npos) base = base.substr(0, dot);
    return base;
}

// ---------------------------------------------------------------------------
// Converte un vector<unsigned> in stringa "0 -> 1 -> 2 -> ..."
// ---------------------------------------------------------------------------
static string SequenceToString(const vector<unsigned>& seq)
{
    ostringstream os;
    for (unsigned i = 0; i < seq.size(); i++)
    {
        os << seq[i];
        if (i + 1 < seq.size()) os << " -> ";
    }
    return os.str();
}

// ---------------------------------------------------------------------------
// Sequenza di arrivo inbound ordinata per ReleaseTime.
// Tie-break: indice camion crescente.
// Questa è la vera "original sequence" del problema.
// ---------------------------------------------------------------------------
static vector<unsigned> ComputeArrivalSequence(const CD_Input& in)
{
    vector<unsigned> arrival_seq(in.InboundTrucks());
    iota(arrival_seq.begin(), arrival_seq.end(), 0);

    sort(arrival_seq.begin(), arrival_seq.end(),
         [&](unsigned a, unsigned b)
         {
             if (in.ReleaseTime(a) != in.ReleaseTime(b))
                 return in.ReleaseTime(a) < in.ReleaseTime(b);
             return a < b;
         });

    return arrival_seq;
}

static void PrintArrivalSequence(const CD_Input& in)
{
    vector<unsigned> arrival_seq = ComputeArrivalSequence(in);

    cout << "Original sequence : " << SequenceToString(arrival_seq) << "\n";

    cout << "Release times     : ";
    for (unsigned k = 0; k < arrival_seq.size(); k++)
    {
        unsigned i = arrival_seq[k];
        cout << "r[" << i << "]=" << in.ReleaseTime(i)
             << (k + 1 < arrival_seq.size() ? " | " : "\n");
    }
}

// ---------------------------------------------------------------------------
// Legge il file risultati nel formato a sezioni.
// Se il file non esiste, restituisce vettore vuoto.
// ---------------------------------------------------------------------------
static vector<ResultRecord> ReadResultsFile(const string& results_file)
{
    ifstream ifs(results_file);
    vector<ResultRecord> rows;

    if (!ifs) return rows;

    string line;
    vector<string> instances, makespans, cpus, originals, inbounds, outbounds;

    auto read_section = [&](vector<string>& target)
    {
        string s;
        while (getline(ifs, s))
        {
            if (s.empty()) break;
            target.push_back(s);
        }
    };

    while (getline(ifs, line))
    {
        if (line == "Instances")                read_section(instances);
        else if (line == "Makespan")            read_section(makespans);
        else if (line == "CPU time (s)")        read_section(cpus);
        else if (line == "Original sequence")   read_section(originals);
        else if (line == "Inbound sequence")    read_section(inbounds);
        else if (line == "Outbound sequence")   read_section(outbounds);
    }

    size_t n = instances.size();
    rows.resize(n);

    for (size_t i = 0; i < n; i++)
    {
        rows[i].instance_name = (i < instances.size() ? instances[i] : "");
        rows[i].makespan      = (i < makespans.size() ? static_cast<unsigned>(stoul(makespans[i])) : 0);
        rows[i].cpu_s         = (i < cpus.size() ? stod(cpus[i]) : 0.0);
        rows[i].original_seq  = (i < originals.size() ? originals[i] : "");
        rows[i].inbound_seq   = (i < inbounds.size() ? inbounds[i] : "");
        rows[i].outbound_seq  = (i < outbounds.size() ? outbounds[i] : "");
    }

    return rows;
}

// ---------------------------------------------------------------------------
// Riscrive il file dei risultati in formato verticale a sezioni.
// ---------------------------------------------------------------------------
static void WriteResultsFile(const string& results_file,
                             const vector<ResultRecord>& rows)
{
    ofstream ofs(results_file);
    if (!ofs)
    {
        cerr << "Attenzione: impossibile scrivere su " << results_file << endl;
        return;
    }

    ofs << "Instances\n";
    for (const auto& r : rows) ofs << r.instance_name << "\n";
    ofs << "\n";

    ofs << "Makespan\n";
    for (const auto& r : rows) ofs << r.makespan << "\n";
    ofs << "\n";

    ofs << "CPU time (s)\n";
    ofs << fixed << setprecision(6);
    for (const auto& r : rows) ofs << r.cpu_s << "\n";
    ofs << "\n";

    ofs << "Original sequence\n";
    for (const auto& r : rows) ofs << r.original_seq << "\n";
    ofs << "\n";

    ofs << "Inbound sequence\n";
    for (const auto& r : rows) ofs << r.inbound_seq << "\n";
    ofs << "\n";

    ofs << "Outbound sequence\n";
    for (const auto& r : rows) ofs << r.outbound_seq << "\n";
    ofs << "\n";
}

// ---------------------------------------------------------------------------
// Aggiunge o aggiorna il record dell'istanza e poi riscrive tutto il file.
// ---------------------------------------------------------------------------
static void append_result(const string& results_file,
                          const string& instance_name,
                          unsigned      makespan,
                          double        cpu_s,
                          const string& original_seq,
                          const string& inbound_seq,
                          const string& outbound_seq)
{
    vector<ResultRecord> rows = ReadResultsFile(results_file);

    bool updated = false;
    for (auto& r : rows)
    {
        if (r.instance_name == instance_name)
        {
            r.makespan     = makespan;
            r.cpu_s        = cpu_s;
            r.original_seq = original_seq;
            r.inbound_seq  = inbound_seq;
            r.outbound_seq = outbound_seq;
            updated = true;
            break;
        }
    }

    if (!updated)
    {
        ResultRecord r;
        r.instance_name = instance_name;
        r.makespan      = makespan;
        r.cpu_s         = cpu_s;
        r.original_seq  = original_seq;
        r.inbound_seq   = inbound_seq;
        r.outbound_seq  = outbound_seq;
        rows.push_back(r);
    }

    WriteResultsFile(results_file, rows);
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cerr << "Usage: " << argv[0]
             << " <input_file> [--results <file>]" << endl;
        return 1;
    }

    string input_file   = argv[1];
    string results_file = "results_v02.txt";

    for (int i = 2; i < argc - 1; i++)
        if (string(argv[i]) == "--results")
            results_file = argv[i + 1];

    auto t_start = chrono::steady_clock::now();

    CD_Input  in(input_file);
    CD_Output out(in);

    if (in.InboundTrucks() <= 20)
        cout << in << endl;

    GreedyCDSolver(in, out);

    vector<unsigned> arrival_seq = ComputeArrivalSequence(in);
    string original_seq = SequenceToString(arrival_seq);
    string inbound_seq  = SequenceToString(out.InboundSeq());
    string outbound_seq = SequenceToString(out.OutboundSeq());

    if (in.InboundTrucks() <= 20)
    {
        PrintArrivalSequence(in);
        cout << "Inbound  sequence : " << inbound_seq << "\n";
        cout << "Outbound sequence : " << outbound_seq << "\n";
    }

    unsigned makespan = out.ComputeMakespan();

    auto t_end = chrono::steady_clock::now();
    double elapsed_s = chrono::duration<double>(t_end - t_start).count();

    cout << "Makespan : " << makespan << endl;
    cout << fixed << setprecision(6);
    cout << "CPU time : " << elapsed_s << " s" << endl;

    append_result(results_file,
                  basename_no_ext(input_file),
                  makespan,
                  elapsed_s,
                  original_seq,
                  inbound_seq,
                  outbound_seq);

    cout << "Result saved in: " << results_file << endl;

    return 0;
}