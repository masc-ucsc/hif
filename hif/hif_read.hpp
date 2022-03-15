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

  bool next_stmt();
  Hif_base::Statement get_current_stmt() { return cur_stmt; }
  void each(const std::function<void(const Hif_base::Statement &stmt)>);

  Hif_read(std::string_view fname);

  std::string_view get_tool() const { return tool; }
  std::string_view get_version() const { return version; }

protected:
  bool is_ok() const { return !idflist.empty(); }

  std::tuple<uint8_t *, uint32_t, int> open_file(const std::string &file);
  
  void     read_idfile(const std::string &idfile);
  uint8_t *read_te(uint8_t *ptr, uint8_t *ptr_end, std::vector<Tuple_entry> &io);
  uint8_t *read_header(uint8_t *ptr, uint8_t *ptr_end, Hif_base::Statement &stmt);

  std::vector<std::string> idflist;
  std::vector<std::string> stflist;

  size_t filepos;

  Statement cur_stmt;

  struct id_entry {
    Hif_base::ID_cat ttt;
    std::string      txt;
  };

  std::string tool;
  std::string version;

  uint8_t *ptr;
  uint8_t *ptr_end;
  uint8_t *ptr_base;
  size_t   ptr_size;
  int      ptr_fd;

  std::vector<id_entry> pos2id;
};
