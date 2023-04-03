#include <ctime>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <ratio>
#include <string>
#include <vector>

#include "hif/hif_read.hpp"
#include "hif/hif_write.hpp"

namespace fs = std::filesystem;

std::string gen_str(const uint32_t len) {
  static const char ch_set[] =
    "_\\"
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  std::string str;
  str.reserve(len);

  for (auto i = 0u; i < len; ++i) {
    str += ch_set[rand() % (sizeof(ch_set) - 1)];
  }

  return str;
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    std::cout << "hif_read_test [number of unique strings] [number of statements]";
    return 0;
  }

  uint32_t n_strs = std::stoi(argv[1]);
  uint32_t n_stmts = std::stoi(argv[2]);

  using namespace std::chrono;
  high_resolution_clock::time_point t_start, t_end;

  std::cout << "Generate random strings" << std::endl;

  std::vector<std::string> names;
  for (uint32_t i = 0; i < n_strs; ++i) {
    names.emplace_back(gen_str(20));
  }

  std::cout << "Starting test" << std::endl;

  // Write Test
  t_start = high_resolution_clock::now();

  auto writer = Hif_write::create(std::string("hif_rand_test"), "test", "1.0");
  for (uint32_t i = 0; i < n_stmts; ++i) {
    auto n = Hif_write::create_assign();
    auto name = names[rand() % n_strs];
    n.instance = name;
    n.add_attr(name, name);
    n.add_input(name, name);
    n.add_output(name, name);
    writer->add(n);
  }

  writer = nullptr;

  t_end = high_resolution_clock::now();
  auto time_span = duration_cast<duration<double>>(t_end - t_start);
  std::cout << "write time = " << time_span.count() << " s" << std::endl;

  // Read Test
  t_start = high_resolution_clock::now();

  uint64_t count = 0;
  auto reader = Hif_read::open("hif_rand_test");
  reader->each([&count] (const Hif_base::Statement &stmt) { count += stmt.io.size() + stmt.attr.size(); });
  std::cout << count << std::endl;
  reader = nullptr;

  t_end = high_resolution_clock::now();

  time_span = duration_cast<duration<double>>(t_end - t_start);
  std::cout << "read time = " << time_span.count() << " s" << std::endl;

  std::uintmax_t file_size = fs::file_size("hif_rand_test/0.id") + fs::file_size("hif_rand_test/0.st");
  std::cout << "file size = " << (double)file_size / 1024 / 1024 << " MB" << std::endl;

  fs::remove_all("hif_rand_test");

  return 0;
}
