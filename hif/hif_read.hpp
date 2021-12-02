//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <cstdint>
#include <tuple>

#include "hif_base.hpp"

class Hif_read : public Hif_base {
public:
  // Load a file (fname) and populate the Hif
  static Hif_read open(const std::string &fname);

  // Populate the Hif from the input string
  static Hif_read input(std::string_view txt);
  static Hif_read input(const std::string &txt) {
    return input(std::string_view(txt.data(), txt.size()));
  }

  void each(const std::function<void(const Statement &stmt)>) const;

  void dump() const;

protected:
  Hif_read(int fd_, std::string_view base_);

  int              fd;
  std::string_view base;
};
