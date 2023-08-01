//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "file_write.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>

std::shared_ptr<File_write> File_write::create(std::string_view fname) {
  std::string name(fname.data(), fname.size());  // fname can be not zero terminated

  int fd = ::open(name.data(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    std::cerr << "File_write::open could not open filename:" << fname << "\n";
    return nullptr;
  }

  return std::make_shared<File_write>(fd);
}

File_write::File_write(int fd_) {
  buffer_pos = 0;
  fd         = fd_;
}

void File_write::add8(uint8_t x) {
  buffer[buffer_pos++] = x;

  if (buffer_pos >= buffer_max) {
    drain();
  }
}

void File_write::add16(uint16_t x) {
  buffer[buffer_pos++] = x;
  buffer[buffer_pos++] = x >> 8;

  if (buffer_pos >= buffer_max) {
    drain();
  }
}

void File_write::add24(uint32_t x) {
  buffer[buffer_pos++] = x;
  buffer[buffer_pos++] = x >> 8;
  buffer[buffer_pos++] = x >> 16;

  if (buffer_pos >= buffer_max) {
    drain();
  }
}

void File_write::add32(uint32_t x) {
  buffer[buffer_pos++] = x;
  buffer[buffer_pos++] = x >> 8;
  buffer[buffer_pos++] = x >> 16;
  buffer[buffer_pos++] = x >> 24;

  if (buffer_pos >= buffer_max) {
    drain();
  }
}

void File_write::add(std::string_view txt) {
  if (txt.size() >= buffer_max) {
    if (buffer_pos)
      drain();

    size_t sz = ::write(fd, txt.data(), txt.size());
    if (sz != buffer_pos) {
      std::cerr << "File_write::add sv write error " << sz << "\n";
    }

    return;
  }

  if ((txt.size() + buffer_pos) > buffer_max) {
    assert(buffer_pos);
    drain();
  }

  memcpy(&buffer[buffer_pos], txt.data(), txt.size());

  buffer_pos += txt.size();
}

void File_write::drain() {
  assert(buffer_pos);

  size_t sz = ::write(fd, buffer, buffer_pos);
  if (sz != buffer_pos) {
    std::cerr << "File_write::destructor could not append, write error " << sz << "\n";
  }

  buffer_pos = 0;
}

File_write::~File_write() {
  assert(fd >= 0);

  if (buffer_pos) {
    drain();
  }

  ::close(fd);
  fd = -1;
}
