//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

#include "hif.hpp"

Hif Hif::input(std::string_view txt) {

  char *ptr = static_cast<char *>(::mmap(0, txt.size(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));  // superpages
  if (ptr == MAP_FAILED) {
    std::cerr << "Hif::input could not allocate a mmap for " << txt.size() << " bytes\n";
    exit(-3);
  }
  memcpy(ptr, txt.data(), txt.size());

  return Hif(-1, std::string_view(ptr, txt.size()));
}

Hif::Hif(int fd_, std::string_view base_)
  :fd(fd_)
  ,base(base_) {
}

void Hif::dump() const {

  std::cout << "fd:" << fd << "\n";

  std::cout << base << std::endl;

}

std::tuple<Hif::Entry_cat, size_t> Hif::process_edge_entry(size_t pos) const {
  assert(false); // TODO

  return {Edge_entry, pos+1};
}

void Hif::each(const std::function<void(const Entry &entry)> fn) const {

  if (base.size()<2)
    return;

  size_t pos=0;
  int scope_level=0;

  while(pos < base.size()) {
    Entry_cat ecat;

    if (base[pos]=='{') {
      ++scope_level;
      ecat = Begin_entry;
      ++pos;
    }else if (base[pos]=='}') {
      --scope_level;
      ecat = End_entry;
      ++pos;
    }else if (base[pos]=='+') {
      ecat = Topo_node_entry;
      ++pos;
    }else if (base[pos]=='-') {
      ecat = Node_entry;
      ++pos;
    }else if (base[pos]=='=') {
      std::tie(ecat,pos) = process_edge_entry(pos);
    }else{
      std::cerr << "invalid HIF format, line starting with " << base[pos] << "\n";
      exit(-3);
    }

    Entry ent;
    ent.ecat =ecat;

    fn(ent);
  }
}
