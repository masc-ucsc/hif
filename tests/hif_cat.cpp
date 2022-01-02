//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <chrono>
#include <iostream>
#include <string>

#include "hif/hif_read.hpp"
#include "hif/hif_write.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage:\n";
    std::cerr << "\thif_cat <filename>\n";
    exit(-3);
  }

  std::string fname(argv[1]);

  auto rd = Hif_read::open(fname);
  if (rd == nullptr) {
    std::cerr << "could not open " << fname << " as HIF file\n";
    exit(-3);
  }

  rd->each([](const Hif_base::Statement &stmt) { stmt.dump(); });
}
