//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "hif_base.hpp"

#include <iostream>

void Hif_base::Statement::print_te(const std::string                       &start,
                                   const std::vector<Hif_base::Tuple_entry> io) const {
  if (io.empty())
    return;

  std::cout << start;

  for (const auto &io : io) {
    if (!io.lhs.empty()) {
      if (!io.input) {
        std::cout << " ->" << io.lhs;
      } else if (io.lhs_cat == Hif_base::ID_cat::String_cat) {
        std::cout << " " << io.lhs;

      } else {
        std::cout << " " << io.lhs << ":" << (int)io.lhs_cat;
      }
    }

    if (!io.rhs.empty()) {
      if (io.rhs_cat == Hif_base::ID_cat::String_cat) {
        std::cout << "<-" << io.rhs;

      } else {
        std::cout << "<-" << io.rhs << ":" << (int)io.rhs_cat;
      }
    }
  }

  std::cout << "\n";
}

void Hif_base::Statement::dump() const {
  static const char *class2name[] = {
    "node",
    "assing",
    "attr",
    "open_call",
    "close_call",
    "open_def",
    "close_def",
    "end",
    "use"
  };

  std::cout << class2name[sclass];

  if (!instance.empty()) {
    std::cout << " " << instance;
  }

  std::cout << " type " << type << "\n";

  print_te("  io  ", io);
  print_te("  attr", attr);
}

