//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class File_write {
public:
  static std::shared_ptr<File_write> create(std::string_view fname);
  static std::shared_ptr<File_write> create(const std::string &fname) {
    return create(std::string_view(fname.data(), fname.size()));
  }

  void add(std::string_view txt);
  void add(const std::string &txt) { add(std::string_view(txt.data(), txt.size())); }
  void add8(uint8_t x);
  void add16(uint16_t v);
  void add24(uint32_t v);
  void add32(uint32_t v);

  template <typename T>
  void add(const std::vector<T> &v) {
    std::string_view sv(v.data(), v.size() * sizeof(T));
    add(sv);
  }

  File_write(int fd_);
  ~File_write();

private:
  void drain();

  int                     fd;
  static constexpr size_t buffer_max = 8192;
  uint8_t                 buffer[buffer_max + 64];  // extra space to handle esily
  size_t                  buffer_pos;
};
