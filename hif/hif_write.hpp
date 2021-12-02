//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <memory>

#include "hif_base.hpp"
#include "waterhash.hpp"

struct cmp_declare_ptr {
  cmp_declare_ptr(std::vector<uint8_t> &p) : ptr(&p) {}

  std::vector<uint8_t> *ptr;
  bool operator==(const cmp_declare_ptr &other) const { return *ptr == *other.ptr; }
};

namespace std {
template <>
struct hash<cmp_declare_ptr> {
  std::size_t operator()(const cmp_declare_ptr &k) const {
    return waterhash(k.ptr->data(), k.ptr->size(), k.ptr->size());
  }
};
}  // namespace std

class Hif_write : public Hif_base {
public:
  // Load a file (fname) and populate the Hif
  static std::shared_ptr<Hif_write> open(const std::string &fname);

  void append(const Statement &stmt);

  void dump() const;

  static Statement create_assign() { return Statement(Statement_class::Assign_class); }

  Hif_write(int fd_);
protected:

  int  append_declare(const Hif_base::ID_cat, std::string_view txt);
  void append_entry(const Hif_base::Tuple_entry &ent);

  int fd;

  std::vector<uint8_t> pre_buffer;
  std::vector<uint8_t> buffer;

  std::unordered_map<cmp_declare_ptr, int> declare2pos;
  std::vector<std::vector<uint8_t>>        dvector;
  size_t                                   dvector_next;
};
