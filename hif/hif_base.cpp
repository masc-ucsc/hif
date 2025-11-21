//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "hif_base.hpp"

#include <iostream>

void Hif_base::Statement::print_tuple_entries(const std::vector<Hif_base::Tuple_entry> tuple_entries, bool is_attr) const {
  if (tuple_entries.empty())
    return;

  int idx = 0;
  for (auto &te : tuple_entries) {
    if (is_attr) {
      std::cout << "    @." << idx++ << "(";
    }
    else {
      std::cout << "    %" << idx++ << ".";
      std::cout << (te.input ? "in  " : "out ") << "(";
    }

    if (te.is_lhs_string()) {
      std::cout << "\"" << te.lhs << "\"";
    }
    else if (te.is_lhs_int64()) {
      std::cout << te.get_lhs_int64() << ") :: i64";
    }

    if (!te.rhs.empty()) {
      std::cout << " = ";
      if (te.is_rhs_string()) {
        std::cout << "\"" << te.rhs << "\"" << ") :: str";
      }
      else if (te.is_rhs_int64()) {
        std::cout << te.get_rhs_int64() << ") :: i64";
      }
      else if (te.is_rhs_base2()) {
        std::cout << "0x";
        for (unsigned char c : te.get_rhs_string()) {
          std::cout << std::hex << static_cast<int>(c);
        }
        std :: cout << ") :: bytestream";
      }
    }
    else {
      std::cout << ")";
    }
    std::cout << "\n";
  }

}

void Hif_base::Statement::dump() const {
  static const char *class2name[] = {
      "node", "assign", "attr", "open_call", "closed_call",
      "open_def", "closed_def", "end", "use"};

  std::cout << "hif." << class2name[sclass];

  if (!instance.empty())
    std::cout << " \"" << instance << "\"";

  if (type != 0)
    std::cout << " type(" << type << ")";

  // For leaf ops with no I/O or attrs, omit braces entirely and return
  if (io.empty() && attr.empty()) {
    std::cout << "\n";
    return;
  }

  std::cout << " {\n";

  if (!io.empty()) {
    std::cout << "  io {\n";
    print_tuple_entries(io);
    std::cout << "  }\n";
  }

  if (!attr.empty()) {
    std::cout << "  attributes {\n";
    print_tuple_entries(attr, true);
    std::cout << "  }\n";
  }

  std::cout << "}\n";
}
