//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <chrono>
#include <string>

#include "benchmark/benchmark.h"
#include "hif/hif_read.hpp"

void code_hif_load() {
  std::string s1(1024, '-');
  std::string s2(1024, '-');
  benchmark::DoNotOptimize(s1.compare(s2));
}

static void BM_hif_load(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    code_hif_load();
  }
}

BENCHMARK(BM_hif_load);

// Run the benchmark
BENCHMARK_MAIN();
