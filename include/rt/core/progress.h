#pragma once
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>

namespace rt {

class ProgressReporter {
public:
    explicit ProgressReporter(int totalUnits, double reportIntervalSeconds = 0.2)
        : total_(totalUnits), interval_(reportIntervalSeconds),
          start_(std::chrono::steady_clock::now()), lastReport_(start_) {}

    void Advance(int n = 1) {
        int done = completed_.fetch_add(n, std::memory_order_relaxed) + n;
        MaybeReport(done);
    }

    void Finish() {
        Report(completed_.load(std::memory_order_relaxed), true);
        std::fprintf(stderr, "\n");
    }

private:
    void MaybeReport(int done) {
        auto now = std::chrono::steady_clock::now();
        auto lastMs = lastReportMs_.load(std::memory_order_relaxed);
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_).count();
        if (nowMs - lastMs < static_cast<long long>(interval_ * 1000) && done < total_) return;
        if (!lastReportMs_.compare_exchange_strong(lastMs, nowMs, std::memory_order_relaxed)) return;
        Report(done, false);
    }

    void Report(int done, bool force) {
        double elapsed = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start_).count();
        double frac = total_ > 0 ? static_cast<double>(done) / total_ : 1.0;
        double rate = elapsed > 0.0 ? done / elapsed : 0.0;
        double eta = rate > 0.0 ? (total_ - done) / rate : 0.0;

        std::fprintf(stderr,
            "\r[%-30s] %5.1f%%  %6d/%-6d rows  %6.1f rows/s  elapsed %6.1fs  eta %6.1fs   ",
            ProgressBar(frac).c_str(), frac * 100.0, done, total_, rate, elapsed,
            force ? 0.0 : eta);
        std::fflush(stderr);
    }

    static std::string ProgressBar(double frac) {
        int filled = static_cast<int>(frac * 30);
        if (filled < 0) filled = 0;
        if (filled > 30) filled = 30;
        return std::string(filled, '=') + std::string(30 - filled, ' ');
    }

    int total_;
    double interval_;
    std::chrono::steady_clock::time_point start_, lastReport_;
    std::atomic<int> completed_{0};
    std::atomic<long long> lastReportMs_{0};
};

}
