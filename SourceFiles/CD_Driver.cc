#include <iostream>
#include <cstdlib>
#include <chrono>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        exit(1);
    }

    CD_Input  in(argv[1]);
    CD_Output out(in);

    if (in.InboundTrucks() <= 20)
        cout << in << endl;

    // with timestamp
    auto t_start = chrono::high_resolution_clock::now();
    GreedyCDSolver(in, out);
    auto t_end = chrono::high_resolution_clock::now();

    if (in.InboundTrucks() <= 20)
        cout << out << endl;

double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    cout << "Makespan : " << out.ComputeMakespan() << endl;
    std::cout << "TIMING ENABLED" << std::endl;
    cout << "CPU time : " << elapsed_ms << " ms" << endl;

    return 0;
}