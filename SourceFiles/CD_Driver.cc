#include <iostream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include "CD_Data.hh"
#include "CD_Greedy.hh"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    auto t_start = chrono::steady_clock::now();

    CD_Input  in(argv[1]);
    CD_Output out(in);

    if (in.InboundTrucks() <= 20)
        cout << in << endl;

    GreedyCDSolver(in, out);

    if (in.InboundTrucks() <= 20)
    {
        vector<unsigned> orig(in.InboundTrucks());
        iota(orig.begin(), orig.end(), 0);
        sort(orig.begin(), orig.end(), [&](unsigned a, unsigned b){
            return in.ReleaseTime(a) != in.ReleaseTime(b)
                 ? in.ReleaseTime(a) <  in.ReleaseTime(b)
                 : a < b;
        });

        cout << "Original sequence: ";
        for (unsigned i = 0; i < orig.size(); i++)
            cout << orig[i] << (i+1 < orig.size() ? " -> " : "\n");

        cout << out << endl;
    }

    unsigned makespan = out.ComputeMakespan();

    auto t_end = chrono::steady_clock::now();
    double elapsed_s = chrono::duration<double>(t_end - t_start).count();

    cout << "Makespan : " << makespan << endl;
    cout << "Doors    : in=" << in.InboundDoors() << "  out=" << in.OutboundDoors() << endl;
    cout << fixed << setprecision(6);
    cout << "CPU time : " << elapsed_s << " s" << endl;

    return 0;
}