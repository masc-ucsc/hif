//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <string_view>

class Hif_base {
public:

  // ID categories (ttt field)
  enum ID_cat : uint8_t { Net_cat=0, String_cat=1, Base2_cat=2, Base3_cat=3, Base4_cat=4, Custom_cat=5 };

  // statement class (cccc field)
  enum Statement_class : uint8_t { Node_class=0, Forward_class, Assign_class, Begin_open_scope_class, Begin_close_scope_class, Begin_open_function_class, Begin_close_function_class, End_class, Use_class, Declara_class };

  struct Tuple_entry {
    std::string_view lhs;
    std::string_view rhs;
    ID_cat  lhs_cat; // Either String or Net (net in output, string in input)
    ID_cat  rhs_cat;
  };

  struct Statement {
    Statement_class          sclass;

    std::string_view         type;
    std::string_view         id;

    std::vector<Tuple_entry> io;
    std::vector<Tuple_entry> attr;
  };

  enum Stmt_cat : uint8_t { Begin_entry, End_entry, Net_entry, Edge_entry, Topo_node_entry, Node_entry };

  struct Field {
    std::string_view lhs;
    std::string_view rhs;
  };

  struct Stmt {
    Stmt_cat ecat;

    std::string_view type;
    std::string_view id;
    std::vector<Field> outputs;
    std::vector<Field> inputs;
  };

};

