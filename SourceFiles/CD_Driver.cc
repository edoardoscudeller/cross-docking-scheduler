#include <iostream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <vector>
#include <numeric>
#include <algorithm>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    CD_Input  in(argv[1]);
    CD_Output out(in);

    std::vector<unsigned> init_seq(in.InboundTrucks());
    std::iota(init_seq.begin(), init_seq.end(), 0);
    std::sort(init_seq.begin(), init_seq.end(), [&in](unsigned a, unsigned b) {
        return in.ReleaseTime(a) < in.ReleaseTime(b);
    });

    std::cout << "Initial Arrival  : ";
    for (unsigned i : init_seq) {
        std::cout << i << "(t=" << in.ReleaseTime(i) << ") ";
    }
    std::cout << std::endl;

    GreedyCDSolver(in, out);

    std::cout << out; // Stampa Inbound e Outbound sequences

    unsigned lb = out.ComputeLowerBound();
    unsigned makespan = out.ComputeMakespan();

    std::cout << "Lower Bound : " << lb << std::endl;
    std::cout << "Makespan    : " << makespan << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    if (lb > 0)
        std::cout << "Gap (%)     : " << 100.0 * (makespan - lb) / (double)lb << "%" << std::endl;
    else
        std::cout << "Gap (%)     : n/a" << std::endl;

    return 0;
}