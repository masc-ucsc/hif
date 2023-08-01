//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

#define USE_ABSL_MAP 1

#ifdef USE_ABSL_MAP
#include "absl/container/flat_hash_map.h"
#else
#include <unordered_map>
#endif

#include "file_write.hpp"
#include "hif_base.hpp"

class Hif_write : public Hif_base {
public:
  static std::shared_ptr<Hif_write> create(std::string_view fname, std::string_view tool,
                                           std::string_view version);
  static std::shared_ptr<Hif_write> create(const std::string &fname,
                                           std::string_view   tool,
                                           std::string_view   version) {
    return create(std::string_view(fname.data(), fname.size()), tool, version);
  }

  void add(const Statement &stmt);

  Hif_write(std::string_view sname, std::string_view tool, std::string_view version);

protected:
  bool is_ok() const { return stbuff != nullptr; }

  // add_* adds data structure and likely to fbuff too
  // write_* adds to fbuff only
  // track_* adds to data structures only

  void add_declare(const Hif_base::Tuple_entry &ent);
  void add_io(const Hif_base::Tuple_entry &ent);
  void add_attr(const Hif_base::Tuple_entry &ent);

  void write_id(const Hif_base::ID_cat, std::string_view txt);
  void write_idref(uint8_t ee, Hif_base::ID_cat ttt, std::string_view txt);
  void write_st(const Hif_base::Tuple_entry &ent);

  std::shared_ptr<File_write> stbuff;
  std::shared_ptr<File_write> idbuff;

  struct id_entry {
    Hif_base::ID_cat ttt;
    uint32_t         pos;
  };
#ifdef USE_ABSL_MAP
  absl::flat_hash_map<std::string, id_entry> id2pos;
#else
  std::unordered_map<std::string, id_entry> id2pos;
#endif
};
