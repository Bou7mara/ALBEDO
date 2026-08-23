#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace rt::bench {

struct BenchmarkRecord {
    std::string sceneName;
    std::string asVariant;
    std::string raySetName;
    size_t primitiveCount;
    size_t rayCount;
    double buildMedianMs;
    double buildMinMs;
    double buildMaxMs;
    double queryMedianMs;
    double queryMinMs;
    double queryMaxMs;
    double mraysPerSec;
    size_t memoryBytes;
    size_t nodeCount;
    float avgFanout;
    int hitCount;
};

struct RepetitionRecord {
    std::string sceneName;
    std::string asVariant;
    std::string raySetName;
    int repId; // 1..N
    size_t primitiveCount;
    size_t rayCount;
    double buildTimeMs;
    double queryTimeMs;
    double mraysPerSec;
    size_t memoryBytes;
    size_t nodeCount;
    float avgFanout;
    int hitCount;
};

class CSVReporter {
public:
    explicit CSVReporter(const std::string& directory = "benchmark_results") {
        std::filesystem::create_directories(directory);
        
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#if defined(_MSC_VER)
        localtime_s(&tm_buf, &in_time_t);
#else
        localtime_r(&in_time_t, &tm_buf);
#endif
        std::stringstream ssSummary, ssReps;
        ssSummary << directory << "/as_evaluation_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << "_summary.csv";
        ssReps << directory << "/as_evaluation_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << "_reps.csv";
        
        filePathSummary_ = ssSummary.str();
        filePathReps_ = ssReps.str();

        fileSummary_.open(filePathSummary_, std::ios::out);
        if (fileSummary_.is_open()) {
            fileSummary_ << "scene,as_variant,ray_set,primitives,ray_count,build_median_ms,build_min_ms,build_max_ms,"
                         << "query_median_ms,query_min_ms,query_max_ms,mrays_per_sec,memory_bytes,node_count,avg_fanout,hits\n";
        }

        fileReps_.open(filePathReps_, std::ios::out);
        if (fileReps_.is_open()) {
            fileReps_ << "scene,as_variant,ray_set,rep_id,primitives,ray_count,build_time_ms,"
                      << "query_time_ms,mrays_per_sec,memory_bytes,node_count,avg_fanout,hits\n";
        }
    }

    void AddSummaryRecord(const BenchmarkRecord& r) {
        if (fileSummary_.is_open()) {
            fileSummary_ << "\"" << r.sceneName << "\","
                         << "\"" << r.asVariant << "\","
                         << "\"" << r.raySetName << "\","
                         << r.primitiveCount << ","
                         << r.rayCount << ","
                         << r.buildMedianMs << ","
                         << r.buildMinMs << ","
                         << r.buildMaxMs << ","
                         << r.queryMedianMs << ","
                         << r.queryMinMs << ","
                         << r.queryMaxMs << ","
                         << r.mraysPerSec << ","
                         << r.memoryBytes << ","
                         << r.nodeCount << ","
                         << r.avgFanout << ","
                         << r.hitCount << "\n";
            fileSummary_.flush();
        }
    }

    void AddRepetitionRecord(const RepetitionRecord& r) {
        if (fileReps_.is_open()) {
            fileReps_ << "\"" << r.sceneName << "\","
                      << "\"" << r.asVariant << "\","
                      << "\"" << r.raySetName << "\","
                      << r.repId << ","
                      << r.primitiveCount << ","
                      << r.rayCount << ","
                      << r.buildTimeMs << ","
                      << r.queryTimeMs << ","
                      << r.mraysPerSec << ","
                      << r.memoryBytes << ","
                      << r.nodeCount << ","
                      << r.avgFanout << ","
                      << r.hitCount << "\n";
            fileReps_.flush();
        }
    }

    const std::string& SummaryFilePath() const { return filePathSummary_; }
    const std::string& RepsFilePath() const { return filePathReps_; }

private:
    std::string filePathSummary_;
    std::string filePathReps_;
    std::ofstream fileSummary_;
    std::ofstream fileReps_;
};

} // namespace rt::bench
