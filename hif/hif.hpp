//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <vector>
#include <list>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

class Hif {
public:

  enum Field_cat : uint8_t { Net_cat, String_cat, Number_cat} ;
  enum Entry_cat : uint8_t { Begin_entry, End_entry, Net_entry, Edge_entry, Topo_node_entry, Node_entry };

  struct Field {
    Field_cat cat;
    std::string_view lhs;
    std::string_view rhs;
  };

  struct Entry {
    Entry_cat ecat;

    std::string_view type;
    std::string_view id;
    std::vector<Field> outputs;
    std::vector<Field> inputs;
  };

  // Load a file (fname) and populate the Hif
  static Hif load(const std::string &fname);

  // Populate the Hif from the input string
  static Hif input(std::string_view txt);
  static Hif input(const std::string &txt) { return input(std::string_view(txt.data(), txt.size())); }

  void each(const std::function<void(const Entry &entry)>) const;

  void dump() const;
protected:
  Hif(int fd_, std::string_view base_);

  std::tuple<Entry_cat, size_t> process_edge_entry(size_t pos) const;
  std::tuple<std::string_view, size_t> get_next_key(size_t pos) const;

  size_t get_next_list(std::vector<Field> &list, size_t pos) const;

  int              fd;
  std::string_view base;

  mutable std::list<std::string> escaped; // list of escape strings that must be regenerated
};

