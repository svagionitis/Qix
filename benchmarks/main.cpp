#include "CollisionDetector.h"
#include "Playfield.h"
#include "Qix.h"
#include "Sparx.h"
#include "TerritoryFill.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

void benchmarkTerritoryFill(std::int32_t width, std::int32_t height, int iterations)
{
    qix::Playfield field {width, height};
    qix::TerritoryFill fill {width, height};

    // Draw vertical partition at x = width / 4
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < height; ++y) {
        trail.push_back(qix::Point {width / 4, y});
    }

    std::vector<qix::Point> qixPositions {qix::Point {width / 2, height / 2}};

    const auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // Reset field to empty inside
        field.initBorders();
        fill.execute(field, trail, qixPositions, qix::DrawMode::Slow, 75);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double avgUs = (static_cast<double>(totalNs) / iterations) / 1000.0;

    std::cout << "[BENCHMARK] TerritoryFill " << width << "x" << height << " (" << field.getInteriorCount()
              << " cells): " << std::fixed << std::setprecision(2) << avgUs << " µs/fill ("
              << static_cast<double>(iterations) / (totalNs / 1e9) << " fills/sec)\n";
}

void benchmarkCollisionDetection(int iterations)
{
    qix::Playfield field {80, 60};
    qix::Marker marker {qix::Point {10, 0}, 3};
    std::vector<qix::Qix> qixList {qix::Qix {qix::LineSegment {qix::Point {20, 20}, qix::Point {30, 20}}}};
    std::vector<qix::Sparx> sparxList {qix::Sparx {qix::Point {40, 0}}};
    qix::Fuse fuse {};

    const auto start = std::chrono::high_resolution_clock::now();

    volatile int dummy = 0;
    for (int i = 0; i < iterations; ++i) {
        const auto event = qix::CollisionDetector::check(marker, qixList, sparxList, fuse);
        dummy += static_cast<int>(event);
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const auto totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double avgNs = static_cast<double>(totalNs) / iterations;

    std::cout << "[BENCHMARK] Collision Detection: " << std::fixed << std::setprecision(2) << avgNs << " ns/audit ("
              << static_cast<double>(iterations) / (totalNs / 1e9) << " checks/sec)\n";
}

int main()
{
    std::cout << "====================================================\n";
    std::cout << "       QIX C++17 ENGINE PERFORMANCE BENCHMARKS      \n";
    std::cout << "====================================================\n";

    benchmarkTerritoryFill(80, 60, 5000);
    benchmarkTerritoryFill(160, 120, 2000);
    benchmarkTerritoryFill(256, 240, 500);

    std::cout << "----------------------------------------------------\n";
    benchmarkCollisionDetection(1000000);
    std::cout << "====================================================\n";

    return 0;
}
