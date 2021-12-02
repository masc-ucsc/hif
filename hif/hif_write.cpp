//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "hif_write.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

std::shared_ptr<Hif_write> Hif_write::open(const std::string &fname) {
  int fd = ::open(fname.c_str(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    std::cerr << "Hif_write::open could not open filename:" << fname << "\n";
    exit(3);
  }

  return std::make_shared<Hif_write>(fd);
}

Hif_write::Hif_write(int fd_) : fd(fd_) {
  dvector.resize(8192);  // up to small ptr (13bits)
  dvector_next = 0;
}

int Hif_write::append_declare(Hif_base::ID_cat ttt, std::string_view txt) {
  assert(txt.size());  // FIXME: no empty IDs (encoded with 8bits)

  size_t sz = txt.size();
  if (sz >> 24) {
    std::cerr << "Hif_write::append_declare txt size " << txt.size()
              << "<< is too long\n";
    exit(3);
  }

  bool small = sz <= 255;
  if (small)
    dvector[dvector_next].resize(2 + txt.size());
  else
    dvector[dvector_next].resize(4 + txt.size());

  uint8_t first_byte = Statement_class::Declare_class;
  if (small)
    first_byte |= 0x80;
  first_byte |= static_cast<uint8_t>(ttt) << 4;

  dvector[dvector_next][0] = first_byte;
  dvector[dvector_next][1] = sz;  // lower 8 bits
  if (!small) {
    dvector[dvector_next][2] = sz >> 8;
    dvector[dvector_next][3] = sz >> 16;
    memcpy(dvector[dvector_next].data() + 4, txt.data(), txt.size());
  } else {
    memcpy(dvector[dvector_next].data() + 2, txt.data(), txt.size());
  }

  const auto it = declare2pos.find(cmp_declare_ptr(dvector[dvector_next]));
  if (it != declare2pos.end())
    return it->second;

  auto insert_pos = dvector_next;

  // WARNING: must be deleted because it is a pointer and it can be stale after
  // rotating

  auto it2 = declare2pos.find(cmp_declare_ptr(dvector[dvector_next]));
  if (it2 != declare2pos.end()) {
    declare2pos.erase(it2);
  }
  declare2pos[cmp_declare_ptr(dvector[dvector_next])] = insert_pos;

  ++dvector_next;
  if (dvector_next > dvector.size()) {  // Either grow the dvector or wrap around
    if (dvector.size() > 8192) {
      dvector_next = 0;
      assert(dvector.size() == 20 * 1024 * 1024);
    } else {
      dvector.resize(20 * 1024 * 1024);
    }
  }

  if (!dvector[dvector_next].empty()) {
    auto it2 = declare2pos.find(cmp_declare_ptr(dvector[dvector_next]));
    if (it2 != declare2pos.end()) {
      if (it2->second == dvector_next) {
        declare2pos.erase(it2);
      }
    }
  }

  return insert_pos;
}

void Hif_write::append_entry(const Hif_base::Tuple_entry &ent) {}

void Hif_write::append(const Statement &stmt) {
  assert(fd >= 0);

  assert(stmt.sclass != Statement_class::Declare_class);
  assert((stmt.type >> 12) == 0);  // 12 bit type identifer (0xFFF is not used)

  assert(pre_buffer.empty());  // future may optimize less writes
  assert(buffer.empty());      // future may optimize less writes

  buffer.push_back((stmt.type & 0xF) | ((stmt.sclass) << 4));
  buffer.push_back(stmt.type >> 4);
  for (const auto &ent : stmt.io) {
    append_entry(ent);
  }
  buffer.push_back(0xFF);  // END OF IOs
  for (const auto &ent : stmt.attr) {
    append_entry(ent);
  }
  buffer.push_back(0xFF);  // END OF ATTRs

  if (!pre_buffer.empty()) {  // may add declare statments in pre-buffer
    auto sz = ::write(fd, pre_buffer.data(), pre_buffer.size());
    if (sz != pre_buffer.size()) {
      std::cerr << "Hif_write::append could append, write error " << sz << "\n";
      exit(3);
    }
    pre_buffer.clear();
  }

  auto sz = ::write(fd, buffer.data(), buffer.size());
  if (sz != buffer.size()) {
    std::cerr << "Hif_write::append could append, write error " << sz << "\n";
    exit(3);
  }
  buffer.clear();
}

void Hif_write::dump() const { std::cout << "fd:" << fd << "\n"; }
