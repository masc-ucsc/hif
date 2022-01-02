//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <cstdint>
#include <tuple>
#include <memory>
#include <functional>

#include "hif_base.hpp"

class Hif_read : public Hif_base {
public:
  // Load a file (fname) and populate the Hif
  static std::shared_ptr<Hif_read> open(std::string_view fname);

  void each(const std::function<void(const Hif_base::Statement &stmt)>);

  Hif_read(std::string_view fname);

protected:
  bool is_ok() const { return !idflist.empty(); }

  std::tuple<uint8_t *, uint32_t, int> open_file(const std::string &file);

  void     read_idfile(const std::string &idfile);
  uint8_t *read_te(uint8_t *ptr, uint8_t *ptr_end, std::vector<Tuple_entry> &io);
  uint8_t *read_header(uint8_t *ptr, uint8_t *ptr_end, Hif_base::Statement &stmt);

  std::vector<std::string> idflist;
  std::vector<std::string> stflist;

  size_t filepos;

  struct id_entry {
    Hif_base::ID_cat ttt;
    std::string      txt;
  };

  std::vector<id_entry> pos2id;
};
