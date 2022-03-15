//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <chrono>
#include <string>

#include "benchmark/benchmark.h"
#include "hif/hif_read.hpp"
#include "hif/hif_write.hpp"

void create_stmt() {

  auto stmt = Hif_write::create_assign();

  stmt.instance = "jojojo";
  stmt.add_input("A", "0");
  stmt.add_input("A", "1");
  stmt.add_input("A", "2");
  stmt.add_input("A", "3");

  stmt.add_output("Z");

  stmt.add_attr("loc", "3");

  auto stmt2 = Hif_write::create_assign();

  stmt.instance = "jojojo";
  stmt.add_input("A", "0");
  stmt.add_input("A", "1");
  stmt.add_input("A", "2");
  stmt.add_input("A", "3");

  stmt.add_output("Z");

  stmt.add_attr("loc", "different");

  benchmark::DoNotOptimize(!(stmt == stmt2));
}

void hif_write_test_1() {

  auto stmt = Hif_write::create_assign();

  stmt.instance = "jojojo";
  stmt.add_input("A", "0");
  stmt.add_input("A", "1");
  stmt.add_input("A", "2");
  stmt.add_input("A", "3");

  stmt.add_output("Z");

  stmt.add_attr("loc", "3");

  auto wr = Hif_write::create(std::string("hif_test_bench"), "hif_bench", "0.xxx");

  wr->add(stmt);

  wr = nullptr;  // close
}

void hif_write_test_1000() {

  auto stmt = Hif_write::create_assign();

  stmt.instance = "jojojo";
  stmt.add_input("A", "0");
  stmt.add_input("A", "1");
  stmt.add_input("A", "2");
  stmt.add_input("A", "3");

  stmt.add_output("Z");

  stmt.add_attr("loc", "3");

  auto wr = Hif_write::create(std::string("hif_test_bench"), "hif_bench", "0.xxx");

  for(auto i=0u;i<1000;++i) {
    wr->add(stmt);
  }

  wr = nullptr;  // close
}

void hif_rdwr_test_1000() {

  auto stmt = Hif_write::create_assign();

  stmt.instance = "jojojo";
  stmt.add_input("A", "0");
  stmt.add_input("A", "1");
  stmt.add_input("A", "2");
  stmt.add_input("A", "3");

  stmt.add_output("Z");

  stmt.add_attr("loc", "3");

  auto wr = Hif_write::create(std::string("hif_test_bench"), "hif_bench", "0.xxx");

  for(auto i=0u;i<1000;++i) {
    wr->add(stmt);
  }

  wr = nullptr;  // close

  auto rd = Hif_read::open(std::string("hif_test_bench"));
  assert(rd != nullptr);

  auto conta = 0u;
  rd->each([&conta](const Hif_base::Statement &stmt) { ++conta; });

  benchmark::DoNotOptimize(!(conta == 1000));
}

void hif_get_current_stmt() {
  auto rd = Hif_read::open(std::string("hif_test_bench"));
  assert(rd != nullptr);

  auto conta = 0u;
  while (rd->next_stmt()) {
    auto stmt = rd->get_current_stmt();
    conta += static_cast<int>(stmt.type);
  }
}

static void BM_hif_stmt(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    create_stmt();
  }
}

static void BM_hif_write_1(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    hif_write_test_1();
  }
}

static void BM_hif_write_1000(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    hif_write_test_1000();
  }
}

static void BM_hif_rdwr_1000(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    hif_rdwr_test_1000();
  }
}

static void BM_hif_get_current_stmt_1000(benchmark::State& state) {
  // Perform setup here
  hif_write_test_1000();
  for (auto _ : state) {
    // This code gets timed
    hif_get_current_stmt();
  }
}

BENCHMARK(BM_hif_stmt);
BENCHMARK(BM_hif_write_1);
BENCHMARK(BM_hif_write_1000);
BENCHMARK(BM_hif_rdwr_1000);
BENCHMARK(BM_hif_get_current_stmt_1000);

// Run the benchmark
BENCHMARK_MAIN();
