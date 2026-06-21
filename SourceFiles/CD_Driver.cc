#include <iostream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    auto t_start = std::chrono::steady_clock::now();

    CD_Input  in(argv[1]);
    CD_Output out(in);

    if (in.InboundTrucks() <= 20)
        std::cout << in << std::endl;

    std::cout << "Doors : in=" << in.InboundDoors()
              << " out=" << in.OutboundDoors() << std::endl;

    GreedyCDSolver(in, out);

    if (in.InboundTrucks() <= 20)
        std::cout << out << std::endl;

    unsigned lb = out.ComputeLowerBound();
    unsigned makespan = out.ComputeMakespan();

    auto t_end = std::chrono::steady_clock::now();

    double elapsed_s =
        std::chrono::duration<double>(t_end - t_start).count();

    std::cout << "Lower Bound : " << lb << std::endl;
    std::cout << "Makespan    : " << makespan << std::endl;

    std::cout << std::fixed << std::setprecision(2);
    if (lb > 0)
        std::cout << "Gap (%)     : " << 100.0 * (makespan - lb) / (double)lb << "%" << std::endl;
    else
        std::cout << "Gap (%)     : n/a" << std::endl;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "CPU time    : " << elapsed_s << " s" << std::endl;

    return 0;
}