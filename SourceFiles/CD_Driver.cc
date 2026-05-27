#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <string>
#include <algorithm>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

// ---------------------------------------------------------------------------
// Estrae il nome base del file di istanza (senza path e senza .dzn)
// es: "../Instances/cd_n004_m003.dzn" -> "cd_n004_m003"
// ---------------------------------------------------------------------------
static std::string basename_no_ext(const std::string& path)
{
    // Trova l'ultimo separatore
    size_t last_sep = path.find_last_of("/\\");
    std::string base = (last_sep == std::string::npos) ? path : path.substr(last_sep + 1);
    // Rimuovi estensione .dzn
    size_t dot = base.rfind('.');
    if (dot != std::string::npos) base = base.substr(0, dot);
    return base;
}

// ---------------------------------------------------------------------------
// Aggiunge una riga al file dei risultati (crea il file se non esiste).
// Formato: <istanza>  <makespan>  <cpu_time_s>
// ---------------------------------------------------------------------------
static void append_result(const std::string& results_file,
                          const std::string& instance_name,
                          unsigned           makespan,
                          double             cpu_s)
{
    std::ofstream ofs(results_file, std::ios::app);
    if (!ofs)
    {
        std::cerr << "Attenzione: impossibile scrivere su " << results_file << std::endl;
        return;
    }
    ofs << std::left << std::setw(24) << instance_name
        << "  " << std::right << std::setw(6) << makespan
        << "  " << std::fixed << std::setprecision(6) << cpu_s
        << "\n";
}

int main(int argc, char* argv[])
{
    // -------------------------------------------------------------------------
    // Argomenti: <input_file> [--results <file>]
    //   --results <file>  : nome del file su cui appendere il risultato
    //                       default: results_v02.txt  (versione corrente)
    // -------------------------------------------------------------------------
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <input_file> [--results <output_results_file>]" << std::endl;
        return 1;
    }

    std::string input_file    = argv[1];
    std::string results_file  = "results_v02.txt";  // default versione corrente

    for (int i = 2; i < argc - 1; i++)
    {
        if (std::string(argv[i]) == "--results")
            results_file = argv[i + 1];
    }

    // ── Avvio misurazione ────────────────────────────────────────────────────
    auto t_start = std::chrono::steady_clock::now();

    CD_Input  in(input_file);
    CD_Output out(in);

    if (in.InboundTrucks() <= 20)
        std::cout << in << std::endl;

    GreedyCDSolver(in, out);

    if (in.InboundTrucks() <= 20)
        std::cout << out << std::endl;

    unsigned makespan = out.ComputeMakespan();

    auto t_end   = std::chrono::steady_clock::now();
    double elapsed_s = std::chrono::duration<double>(t_end - t_start).count();

    // ── Output su stdout ─────────────────────────────────────────────────────
    std::cout << "Makespan : " << makespan << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "CPU time : " << elapsed_s << " s" << std::endl;

    // ── Append su file risultati ──────────────────────────────────────────────
    std::string inst_name = basename_no_ext(input_file);
    append_result(results_file, inst_name, makespan, elapsed_s);
    std::cout << "Risultato salvato in: " << results_file << std::endl;

    return 0;
}