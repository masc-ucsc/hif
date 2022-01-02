//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "hif_read.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <iostream>

std::shared_ptr<Hif_read> Hif_read::open(std::string_view fname) {
  auto ptr = std::make_shared<Hif_read>(fname);

  return ptr->is_ok() ? ptr : nullptr;
}

Hif_read::Hif_read(std::string_view fname) {
  std::string sname(fname.data(), fname.size());

  const char *path = sname.c_str();

  DIR *dir = opendir(path);
  if (dir == nullptr) {
    std::cerr << "Hif_read::open not valid HIF file " << fname << "\n";
    return;
  }

  struct dirent *dirp;

  while ((dirp = readdir(dir)) != NULL) {
    std::string_view sv(dirp->d_name, strlen(dirp->d_name));
    if (sv == ".." || sv == ".")
      continue;

    bool unexpected_file = !std::isdigit(sv[0]) || sv.size() < 4;
    auto end_sv          = sv.substr(sv.size() - 3);
    if (!unexpected_file) {
      unexpected_file = (end_sv != ".st" && end_sv != ".id");
    }

    if (unexpected_file) {
      continue;  // ignore unexpected files
    }

    if (end_sv == ".id")
      idflist.push_back(sname + "/" + std::string(sv));
    else
      stflist.push_back(sname + "/" + std::string(sv));
  }
  closedir(dir);

  std::sort(idflist.begin(), idflist.end());
  std::sort(stflist.begin(), stflist.end());

  bool corrupted = stflist.size() != idflist.size();
  if (!corrupted) {
    for (auto i = 0u; i < stflist.size(); ++i) {
      if (stflist[i].size() != idflist[i].size()) {
        corrupted = true;
        break;
      }
      auto sz = stflist[i].size() - 3;
      if (stflist[i].substr(0, sz) != idflist[i].substr(0, sz)) {
        corrupted = true;
        break;
      }
    }
  }
  if (corrupted) {
    std::cerr << "Hif_read::open corrupted HIF file " << fname << "\n";
    std::cerr << "  id list:";
    for (const auto &e : idflist) {
      std::cerr << " " << e;
    }
    std::cerr << "\n";
    std::cerr << "  st list:";
    for (const auto &e : stflist) {
      std::cerr << " " << e;
    }
    std::cerr << "\n";
  }

  filepos = 0;
}

std::tuple<uint8_t *, uint32_t, int> Hif_read::open_file(const std::string &file) {
  int fd = ::open(file.c_str(), O_RDONLY, 0644);
  if (fd < 0) {
    std::cerr << "Hif_read could not open HIF chunk " << file << "\n";
    return std::make_tuple(nullptr, 0, -1);
  }

  struct stat sb;

  if (fstat(fd, &sb) == -1) {
    std::cerr << "Hif_read could not get size for filename:" << file << "\n";
    close(fd);
    return std::make_tuple(nullptr, 0, -1);
  }

  uint8_t *ptr
      = static_cast<uint8_t *>(mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
  if (ptr == MAP_FAILED) {
    std::cerr << "Hif_read could not allocate a mmap for filename:" << file << " with "
              << sb.st_size << " bytes\n";
    close(fd);
    return std::make_tuple(nullptr, 0, -1);
  }

  return std::make_tuple(ptr, sb.st_size, fd);
}

void Hif_read::read_idfile(const std::string &idfile) {
  pos2id.clear();

  auto [ptr, ptr_size, fd] = open_file(idfile);

  uint8_t *ptr_base = ptr;
  uint8_t *ptr_end  = ptr + ptr_size;

  uint32_t pos = 0;

  while (ptr < ptr_end) {
    uint8_t ttt   = *ptr & 0x07;
    bool    small = (*ptr & 0x08) != 0;

    uint32_t sz = (*ptr) >> 4;
    if (small) {
      ptr += 1;
    } else {
      uint32_t sz1 = *((uint16_t *)(ptr + 1));
      sz1 <<= 4;
      sz |= sz1;

      ptr += 3;
    }

    if (ptr + sz > ptr_end) {
      std::cerr << "Hif_read::read_idfile corrupted HIF file " << idfile << " with " << sz
                << "\n";
      return;
    }
    std::string_view sv((char *)ptr, sz);

    if (ttt > ID_cat::Custom_cat) {
      std::cerr << "Hif_read::read_idfile corrupted HIF file " << idfile << " cat " << ttt
                << "\n";
      return;
    }

    if (pos2id.size() <= pos)
      pos2id.resize(pos + 1);

    pos2id[pos].ttt = static_cast<ID_cat>(ttt);
    pos2id[pos].txt = sv;

    ptr += sz;
    pos += 1;
  }

  munmap(ptr_base, ptr_size);
  close(fd);
}

#if 0
size_t Hif_read::read_declare(size_t pos) {

  uint8_t sttt = (base[pos]>>4) & 0x0F;
  if ((sttt&0x7) > ID_cat::Custom_cat) {
    std::cerr << "Hif_read::each invalid sttt " << sttt << " (aborting)\n";
    return base_size;
  }

  size_t id_sz = base[pos+1];
  {
    bool small   = (sttt & 0x7)!=0;
    if (small) {
      pos += 2;
    }else{
      id_sz <<= 8;
      id_sz |= base[pos+2];
      id_sz <<= 8;
      id_sz |= base[pos+3];
      pos += 4;
    }
  }

  pos += id_sz;

  return pos;
}
#endif


uint8_t *Hif_read::read_te(uint8_t *ptr, uint8_t *ptr_end, std::vector<Tuple_entry> &io) {
  int lhs_pos = -1;

  while (*ptr != 0xFF) {
    bool    small = (*ptr & 1) != 0;
    uint8_t ee    = (*ptr >> 1) & 0x3;

    bool input;
    bool portid;
    if (ee == 0) {  // input or attribute lhs
      input  = true;
      portid = true;
    } else if (ee == 1) {  // output lhs
      input  = false;
      portid = true;
    } else if (ee == 2) {  // rhs
      input  = true;
      portid = false;
    } else {
      std::cerr << "Hif_read corrupted st ee " << *ptr << " (aborting)\n";
      return ptr_end;
    }

    uint32_t pos = *ptr >> 3;
    if (small) {
      ptr += 1;
    } else {
      uint32_t pos2 = *(uint16_t *)(ptr + 1);
      pos2 <<= 4;
      pos |= pos2;
      ptr += 3;
    }

    if (pos >= pos2id.size()) {
      std::cerr << "Hif_read corrupted st pos " << pos << " (aborting)\n";
      return ptr_end;
    }

    if (input) {
      if (lhs_pos >= 0) {
        io.emplace_back(pos2id[lhs_pos].txt,
                        pos2id[pos].txt,
                        pos2id[lhs_pos].ttt,
                        pos2id[pos].ttt);
        lhs_pos = -1;
      } else {
        lhs_pos = pos;
      }
    } else {
      if (lhs_pos >= 0) {
        std::cerr << "Hif_read corrupted st lhs_pos " << lhs_pos
                  << " withou rhs (aborting)\n";
        return ptr_end;
      }
      io.emplace_back(pos2id[pos].txt);
    }

    if (ptr > ptr_end) {
      std::cerr << "Hif_read corrupted st overflow (aborting)\n";
      return ptr_end;
    }
  }

  if (lhs_pos >= 0) {
    std::cerr << "Hif_read corrupted st lhs_pos " << lhs_pos
              << " withou rhs (aborting)\n";
    return ptr_end;
  }

  ptr += 1;

  return ptr;
}

uint8_t *Hif_read::read_header(uint8_t *ptr, uint8_t *ptr_end, Statement &stmt) {
  uint8_t cccc = (*ptr) >> 4;
  if (cccc > Statement_class::Use_class) {
    std::cerr << "Hif_read invalid cccc " << cccc << "\n";
    return ptr_end;
  }

  stmt.sclass = static_cast<Statement_class>(cccc);

  {
    uint16_t type  = *ptr & 0xF;
    uint16_t type1 = ptr[1];
    type |= type1 << 4;

    stmt.type = type;
  }
  ptr += 2;

  if (*ptr == 0xFF) { // no instance identifier
    ptr += 1;
  }else{

    uint32_t pos = *ptr >> 3;
    bool  small = (*ptr & 1) != 0;
    if (small) {
      ptr += 1;
    } else {
      uint32_t pos2 = *(uint16_t *)(ptr + 1);
      pos2 <<= 4;
      pos |= pos2;
      ptr += 3;
    }

    if (pos >= pos2id.size()) {
      std::cerr << "Hif_read corrupted instance pos " << pos << " (aborting)\n";
      return ptr_end;
    }
    stmt.instance = pos2id[pos].txt;
  }

  return ptr;
}

void Hif_read::each(const std::function<void(const Statement &stmt)> fn) {
  read_idfile(idflist[0]);

  auto [ptr, ptr_size, fd] = open_file(stflist[0]);

  uint8_t *ptr_base = ptr;
  uint8_t *ptr_end  = ptr + ptr_size;

  while (ptr < ptr_end) {
    Statement stmt;

    ptr = read_header(ptr, ptr_end, stmt);
    ptr = read_te(ptr, ptr_end, stmt.io);
    ptr = read_te(ptr, ptr_end, stmt.attr);

    fn(stmt);
  }

  munmap(ptr_base, ptr_size);
  close(fd);
}
