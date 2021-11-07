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

std::tuple<std::string_view, size_t> Hif::get_next_key(size_t pos) const {

  auto x = base.find(':', pos);
  if (x == std::string::npos) {
    std::cerr << "invalid HIF format, could not find : in " << base.substr(pos,20) << "...\n";
    exit(-3);
  }

  return {base.substr(pos, x-pos), x+1};
}

size_t Hif::get_next_list(std::vector<Field> &list, size_t pos) const {

  bool equal_found= false;
  std::string_view lhs;

  std::string escaped_string;
  
  while(true) {
    auto next = base.find_first_of(",:=\n\\", pos);
    if (next == std::string::npos) {
      std::cerr << "invalid HIF format, could not find delimiter " << base.substr(pos,20) << "...\n";
      exit(-3);
    }

    if (base[next-1] == '\\') {
      if (escaped_string.empty()) {
        escaped_string = base.substr(pos, pos-next-1);
      }
      escaped_string.append(1,base[next]);
      pos = next+1;
      continue;
    }else if (!escaped_string.empty()) {
      escaped_string.append(base.substr(pos,next-pos));
    }

    if (base[next] == ',') {
      Field f;
      if (equal_found) {
        f.lhs = lhs;
        equal_found = false;
      }
      if (escaped_string.empty()) {
        f.rhs = base.substr(pos, next-pos);
      }else{
        escaped.push_back(escaped_string); // keep ptr stability
        f.rhs = std::string_view(escaped.back().data(), escaped_string.size());
      }
      pos   = next+1;

      list.emplace_back(f);
    }else if (base[next] == '=') {
      if (equal_found) {
        std::cerr << "invalid HIF format, two equals in a row " << base.substr(pos,20) << "...\n";
        exit(-3);
      }

      equal_found = true;
      if (escaped_string.empty()) {
        lhs = base.substr(pos, next-pos);
      }else{
        escaped.push_back(escaped_string); // keep ptr stability
        lhs = std::string_view(escaped.back().data(), escaped_string.size());
      }
      pos   = next+1;
    }else if (base[next] == ':' || base[next] == '\n') {
      return pos+1;
    }
  }
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

    std::tie(ent.type   , pos) = get_next_key(pos);
    std::tie(ent.id     , pos) = get_next_key(pos);

    pos = get_next_list(ent.outputs, pos);

    fn(ent);
  }
}
