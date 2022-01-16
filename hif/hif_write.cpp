//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "hif_write.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <cassert>
#include <iostream>

std::shared_ptr<Hif_write> Hif_write::create(std::string_view fname, std::string_view tool, std::string_view version) {
  auto ptr = std::make_shared<Hif_write>(fname, tool, version);

  return ptr->is_ok() ? ptr : nullptr;
}

Hif_write::Hif_write(std::string_view fname, std::string_view tool, std::string_view version) {
  std::string sname(fname.data(), fname.size());

  const char *path = sname.c_str();

  DIR *dir = opendir(path);
  if (dir) {  // directory exists
    struct dirent *dirp;

    while ((dirp = readdir(dir)) != NULL) {
      std::string_view sv(dirp->d_name, strlen(dirp->d_name));
      if (sv == ".." || sv == ".")
        continue;
      if (sv == "0.st" || sv == "0.id")
        continue;

      bool unexpected_file = !std::isdigit(sv[0]) || sv.size() < 4;
      if (!unexpected_file) {
        auto end_sv     = sv.substr(sv.size() - 3);
        unexpected_file = (end_sv != ".st" && end_sv != ".id");
      }

      if (unexpected_file) {
        std::cerr << "Hif_write::create directory " << fname << " has extra files like "
                  << sv << " (aborting)\n";
        return;
      }

      remove(dirp->d_name);
    }
    closedir(dir);
  } else {
    int fail = mkdir(path, 0755);
    if (fail) {
      std::cerr << "Hif_write::create could creater directory " << fname << "\n";
    }
  }

  stbuff = File_write::create(sname + "/" + "0.st");
  idbuff = File_write::create(sname + "/" + "0.id");

  {
    auto conf_stmt = Hif_write::create_attr();
    conf_stmt.add_attr("HIF",hif_version);
    conf_stmt.add_attr("tool", tool);
    conf_stmt.add_attr("version",version);

    add(conf_stmt);
  }
}

void Hif_write::write_idref(uint8_t ee, Hif_base::ID_cat ttt, std::string_view txt_) {
#ifdef USE_ABSL_MAP
  std::string_view txt = txt_;
#else
  std::string txt(txt_.data(), txt_.size());
#endif

  auto it = id2pos.find(txt);

  uint32_t pos;
  if (it == id2pos.end()) {
    pos             = id2pos.size();
    id2pos[txt].pos = pos;
    id2pos[txt].ttt = ttt;

    write_id(ttt, txt);
  } else {
    pos = it->second.pos;
  }

  uint32_t ref = (pos << 3) | (ee << 1);
  if (pos < 31) { // WARNING: if 31 is allowed it aliases with 0xFF end
    stbuff->add8(ref | 1);  // small
  } else {
    stbuff->add8(ref);
    stbuff->add16(ref>>8);
  }
}

void Hif_write::write_id(Hif_base::ID_cat ttt, std::string_view txt) {
  uint16_t x = txt.size() << 4;
  if (txt.size() < 16) {
    idbuff->add8(x | ttt | 0x08);  // 0x8==small
  } else {
    idbuff->add8(x | ttt);
    idbuff->add16(txt.size() >> 4);
  }

  idbuff->add(txt);
}

void Hif_write::add_io(const Hif_base::Tuple_entry &ent) {
  uint8_t ee = ent.input ? 1 : 0;  // input or output port id

  if (!ent.lhs.empty()) {
    if (ent.rhs.empty())
      ee |= 0x2; // last in lhs/rhs sequence
    write_idref(ee, ent.lhs_cat, ent.lhs);
  }
  if (!ent.rhs.empty()) {
    ee |= 0x2;
    write_idref(ee, ent.rhs_cat, ent.rhs);
  }
}

void Hif_write::add_attr(const Hif_base::Tuple_entry &ent) {
  assert(!ent.lhs.empty());  // attr must have lhs AND rhs
  assert(!ent.rhs.empty());  // attr must have lhs AND rhs

  uint8_t lhs_ee = 1;  // attr left-hand-side
  write_idref(lhs_ee, ent.lhs_cat, ent.lhs);

  uint8_t rhs_ee = 3;  // right-hand-side
  write_idref(rhs_ee, ent.rhs_cat, ent.rhs);
}

void Hif_write::add(const Statement &stmt) {
  assert((stmt.type >> 12) == 0);  // max 12 bit type identifer

  // worst case. Time to create new 0/1 id file
  if ((2 * stmt.io.size() + 2 * stmt.attr.size() + id2pos.size()) > (1 << 20)) {
    assert(false);  // FIXME: create new ID file
  }

  stbuff->add8((stmt.type & 0xF) | ((stmt.sclass) << 4));
  stbuff->add8(stmt.type >> 4);

  if (stmt.instance.empty()) {
    stbuff->add8(0xFF);  // no instance identifier
  }else{
    write_idref(0x3, Hif_base::ID_cat::String_cat, stmt.instance);
  }

  for (const auto &ent : stmt.io) {
    add_io(ent);
  }
  stbuff->add8(0xFF);  // END OF IOs
  for (const auto &ent : stmt.attr) {
    add_attr(ent);
  }
  stbuff->add8(0xFF);  // END OF ATTRs
}

