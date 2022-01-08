//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#pragma once

#include <string.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class Hif_base {
public:
  // ID categories (ttt field)
  enum ID_cat : uint8_t {
    String_cat = 0,
    Base2_cat  = 1,
    Base3_cat  = 2,
    Base4_cat  = 3,
    Custom_cat = 4
  };

  // statement class (cccc field)
  enum Statement_class : uint8_t {
    Node= 0,
    Assign,
    Attr,
    Open_call,
    Closed_call,
    Open_def,
    Closed_def,
    End,
    Use
  };

  struct Common_base {
    constexpr Common_base(const void *d, uint32_t s) : data(d), size(s) {}
    const void    *data;
    uint32_t size;
    std::string_view to_sv() { return std::string_view(static_cast<const char *>(data),size); }
    bool operator==(const Common_base &r) const {
      return size == r.size && memcmp(data, r.data, size) == 0;
    }
  };

  struct String : public Common_base {
  };
  struct Base2 : public Common_base {
    Base2(const int64_t *r, uint32_t sz=sizeof(int64_t))
       :Common_base(static_cast<const void *>(r) , sz) {}
  };
  struct Base4 : public Common_base {
  };
  struct Custom : public Common_base {
  };

  struct Tuple_entry {
    Tuple_entry(bool i, std::string_view l, std::string_view r, ID_cat lc, ID_cat rc)
        : input(i), lhs(l), rhs(r), lhs_cat(lc), rhs_cat(rc) {}

    bool             input;
    std::string_view lhs;
    std::string_view rhs;
    ID_cat           lhs_cat;  // Either String or Net (net in output, string in input)
    ID_cat           rhs_cat;

    bool operator==(const Tuple_entry &r) const {
      return input == r.input && lhs == r.lhs && rhs == r.rhs && lhs_cat == r.lhs_cat
             && rhs_cat == r.rhs_cat;
    }
  };

  struct Statement {
    Statement_class sclass;

    uint16_t type;  // 12 bit type

    std::string_view instance;

    std::vector<Tuple_entry> io;
    std::vector<Tuple_entry> attr;

    Statement() : sclass(Statement_class::Node), type(0) {}
    Statement(Statement_class c) : sclass(c), type(0) {}

    void add_input(std::string_view l) {
      io.emplace_back(true, l, "", ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_input(std::string_view l, std::string_view r) {
      io.emplace_back(true, l, r, ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_input(String l, String r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_input(String l, Base2 r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Base2_cat);
    }
    void add_input(String l, Base4 r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Base4_cat);
    }
    void add_input(String l, Custom r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Custom_cat);
    }
    void add_input(Base2 l, String r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::String_cat);
    }
    void add_input(Base2 l, Base2 r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Base2_cat);
    }
    void add_input(Base2 l, Base4 r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Base4_cat);
    }
    void add_input(Base2 l, Custom r) {
      io.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Custom_cat);
    }

    void add_output(std::string_view l) {
      io.emplace_back(false, l, "", ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_output(String l) {
      io.emplace_back(false, l.to_sv(), "", ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_output(String l, String r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_output(String l, Base2 r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Base2_cat);
    }
    void add_output(String l, Base4 r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Base4_cat);
    }
    void add_output(String l, Custom r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Custom_cat);
    }
    void add_output(Base2 l, String r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::String_cat);
    }
    void add_output(Base2 l, Base2 r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Base2_cat);
    }
    void add_output(Base2 l, Base4 r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Base4_cat);
    }
    void add_output(Base2 l, Custom r) {
      io.emplace_back(false, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Custom_cat);
    }

    void add_attr(std::string_view l) {
      attr.emplace_back(true, l, "", ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_attr(std::string_view l, std::string_view r) {
      attr.emplace_back(true, l, r, ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_attr(String l, String r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::String_cat);
    }
    void add_attr(String l, Base2 r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Base2_cat);
    }
    void add_attr(String l, Base4 r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Base4_cat);
    }
    void add_attr(String l, Custom r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::String_cat, ID_cat::Custom_cat);
    }
    void add_attr(Base2 l, String r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::String_cat);
    }
    void add_attr(Base2 l, Base2 r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Base2_cat);
    }
    void add_attr(Base2 l, Base4 r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Base4_cat);
    }
    void add_attr(Base2 l, Custom r) {
      attr.emplace_back(true, l.to_sv(), r.to_sv(), ID_cat::Base2_cat, ID_cat::Custom_cat);
    }

    bool operator==(const Statement &rhs) const {
      return sclass == rhs.sclass && instance == rhs.instance && type == rhs.type && io == rhs.io && attr == rhs.attr;
    }

    void dump() const;
    void print_te(const std::string                       &start,
                  const std::vector<Hif_base::Tuple_entry> io) const;
  };

  static Statement create_node()        { return Statement(Statement_class::Node); }
  static Statement create_assign()      { return Statement(Statement_class::Assign); }
  static Statement create_attr()        { return Statement(Statement_class::Attr); }
  static Statement create_open_call()   { return Statement(Statement_class::Open_call); }
  static Statement create_closed_call() { return Statement(Statement_class::Closed_call); }
  static Statement create_open_def()    { return Statement(Statement_class::Open_def); }
  static Statement create_closed_def()  { return Statement(Statement_class::Closed_def); }
  static Statement create_end()         { return Statement(Statement_class::End); }
  static Statement create_use()         { return Statement(Statement_class::Use); }

protected:
  Hif_base() {}
};
