//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

#include "hif_read.hpp"

Hif_read Hif_read::input(std::string_view txt) {

  char *ptr = static_cast<char *>(::mmap(0, txt.size(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));  // superpages
  if (ptr == MAP_FAILED) {
    std::cerr << "Hif::input could not allocate a mmap for " << txt.size() << " bytes\n";
    exit(-3);
  }
  memcpy(ptr, txt.data(), txt.size());

  return Hif_read(-1, std::string_view(ptr, txt.size()));
}


Hif_read::Hif_read(int fd_, std::string_view base_)
  :fd(fd_)
  ,base(base_) {
}

void Hif_read::dump() const {

  std::cout << "fd:" << fd << "\n";

  std::cout << base << std::endl;

}

void Hif_read::each(const std::function<void(const Statement &stmt)> fn) const {

  if (base.size()<2)
    return;

  auto pos = 0u;

  while(pos < base.size()) {
    Statement stmt;

    fn(stmt);
  }
}
