#pragma once
#include "benchmarks/report/csv_reporter.h"
#include <iomanip>
#include <iostream>
#include <vector>

namespace rt::bench {

class ConsoleReporter {
public:
    static void PrintHeader(const std::string& sceneName, size_t primCount,
                            const std::string& raySetName, size_t rayCount) {
        std::cout << "\n=================================================================================================================\n";
        std::cout << " Scene: " << sceneName << " (" << primCount << " prims) | Workload: " << raySetName << " (" << rayCount << " rays)\n";
        std::cout << "=================================================================================================================\n";
        std::cout << std::left << std::setw(26) << " AS Variant"
                  << " | " << std::right << std::setw(11) << "Build (ms)"
                  << " | " << std::right << std::setw(11) << "Query (ms)"
                  << " | " << std::right << std::setw(8) << "Speedup"
                  << " | " << std::right << std::setw(10) << "MRay/s"
                  << " | " << std::right << std::setw(9) << "Nodes"
                  << " | " << std::right << std::setw(7) << "Fanout"
                  << " | " << std::right << std::setw(10) << "Mem (KB)\n";
        std::cout << "---------------------------+-------------+-------------+----------+------------+-----------+---------+------------\n";
    }

    static void PrintRow(const BenchmarkRecord& r, double baselineQueryMs) {
        double speedup = (baselineQueryMs > 0.0) ? (baselineQueryMs / r.queryMedianMs) : 1.0;
        double memKb = r.memoryBytes / 1024.0;

        std::cout << std::left << std::setw(26) << (" " + r.asVariant)
                  << " | " << std::right << std::setw(8) << std::fixed << std::setprecision(2) << r.buildMedianMs << " ms"
                  << " | " << std::right << std::setw(8) << std::fixed << std::setprecision(2) << r.queryMedianMs << " ms"
                  << " | " << std::right << std::setw(7) << std::fixed << std::setprecision(2) << speedup << "x"
                  << " | " << std::right << std::setw(7) << std::fixed << std::setprecision(2) << r.mraysPerSec << " M"
                  << " | " << std::right << std::setw(9) << r.nodeCount
                  << " | " << std::right << std::setw(7) << std::fixed << std::setprecision(2) << r.avgFanout
                  << " | " << std::right << std::setw(7) << std::fixed << std::setprecision(1) << memKb << " KB\n";
    }

    static void PrintFooter() {
        std::cout << "=================================================================================================================\n";
    }
};

}
