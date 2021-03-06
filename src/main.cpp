#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include "floorplanner.h"
using namespace std;

int main(int argc, char** argv)
{
    fstream input_blk, input_net, output;
    double alpha;
    bool plot = false;
    string plot_file = "best.png";

    if (argc >= 5 && argc <= 7) {
        alpha = stod(argv[1]);
        input_blk.open(argv[2], ios::in);
        input_net.open(argv[3], ios::in);
        output.open(argv[4], ios::out);
        if (!input_blk) {
            cerr << "Cannot open the input file \"" << argv[2]
                 << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!input_net) {
            cerr << "Cannot open the input file \"" << argv[3]
                 << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (!output) {
            cerr << "Cannot open the output file \"" << argv[4]
                 << "\". The program will be terminated..." << endl;
            exit(1);
        }
        if (argc >= 6) {
            if (argv[5] == string("-p")) {
                plot = true;
                if (argc == 7) {
                    plot_file = argv[6];
                }
            }
        }
    }
    else {
        cerr << "Usage: ./Floorplanner <alpha> <input block file> " <<
                "<input net file> <output file> <-p(optional)> <plot file(optional)>" << endl;
        exit(1);
    }

    auto start = chrono::high_resolution_clock::now();
    Floorplanner fp(alpha, input_blk, input_net);
    fp.floorplan();
    auto stop = chrono::high_resolution_clock::now();
    chrono::duration<double> exe_time = stop - start;
    fp.printSummary(output, exe_time.count());
    if (plot) {
        fp.plot(plot_file);
    }

    return 0;
}
