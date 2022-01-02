//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class Hif_base {
public:
  // ID categories (ttt field)
  enum ID_cat : uint8_t {
    String_cat = 0,
    Base2_cat  = 1,
    Base3_cat  = 2,
    Base4_cat  = 3,
    Custom_cat = 4
  };

  // statement class (cccc field)
  enum Statement_class : uint8_t {
    Node_class = 0,
    Assign_class,
    Attr_class,
    Begin_open_scope_class,
    Begin_close_scope_class,
    Begin_open_function_class,
    Begin_close_function_class,
    End_class,
    Use_class
  };

  struct Tuple_entry {
    // add input
    Tuple_entry(std::string_view l, std::string_view r, ID_cat lc, ID_cat rc)
        : input(true), lhs(l), rhs(r), lhs_cat(lc), rhs_cat(rc) {}

    // add output
    Tuple_entry(std::string_view l)
        : input(false)
        , lhs(l)
        , lhs_cat(ID_cat::String_cat)
        , rhs_cat(ID_cat::String_cat) {}

    bool             input;
    std::string_view lhs;
    std::string_view rhs;
    ID_cat           lhs_cat;  // Either String or Net (net in output, string in input)
    ID_cat           rhs_cat;

    const bool operator==(const Tuple_entry &r) const {
      return input == r.input && lhs == r.lhs && rhs == r.rhs && lhs_cat == r.lhs_cat
             && rhs_cat == r.rhs_cat;
    }
  };

  struct Statement {
    Statement_class sclass;

    uint16_t type;  // 12 bit type

    std::string_view instance;

    std::vector<Tuple_entry> io;
    std::vector<Tuple_entry> attr;

    Statement() : sclass(Statement_class::Node_class), type(0) {}
    Statement(Statement_class c) : sclass(c), type(0) {}

    void add_input_string(std::string_view lhs, std::string_view rhs) {
      io.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_input_base2(std::string_view lhs, std::string_view rhs) {
      io.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::Base2_cat);
    }
    void add_input_base4(std::string_view lhs, std::string_view rhs) {
      io.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::Base4_cat);
    }
    void add_input_custom(std::string_view lhs, std::string_view rhs) {
      io.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::Custom_cat);
    }

    void add_output(std::string_view lhs) { io.emplace_back(lhs); }

    void add_attr_string(std::string_view lhs, std::string_view rhs) {
      attr.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_attr_base2(std::string_view lhs, std::string_view rhs) {
      attr.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::Base2_cat);
    }
    void add_attr_base4(std::string_view lhs, std::string_view rhs) {
      attr.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::Base4_cat);
    }
    void add_attr_custom(std::string_view lhs, std::string_view rhs) {
      attr.emplace_back(lhs, rhs, ID_cat::String_cat, ID_cat::Custom_cat);
    }

    const bool operator==(const Statement &rhs) const {
      return sclass == rhs.sclass && instance == rhs.instance && type == rhs.type && io == rhs.io && attr == rhs.attr;
    }

    void dump() const;
    void print_te(const std::string                       &start,
                  const std::vector<Hif_base::Tuple_entry> io) const;
  };

  static Statement create_assign() { return Statement(Statement_class::Assign_class); }

protected:
  Hif_base() {}
};
