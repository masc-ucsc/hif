//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <string>

#include "fmt/format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "hif/hif.hpp"

class Hif_test : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {
    // Graph_library::sync_all();
  }
};

TEST_F(Hif_test, Trivial_test1) {

  // instantiate IMLI, some configuration and test the basic API so that it it
  // learns. It is also a way to showcase the API

  std::string test {
    "{conf:::tool=some_verilog_tool,version= 3.some_string.1\n"
    "=::: file=submodule.v,loc=2\n"
    "{module:inner:\\a,\\h:\\z,\\y,order=a;b;c;d\n"
    "+and::\a=Z:A=\\y,B=\\z,loc=3\n"
    "=:::loc=4\n"
    "+and::\\tmp=Z:A=\\y,B=\\z\n"
    "+not::\\h:\\tmp\n"
    "}loc=5\n"
    "{module:submodule:\\c,\\d:\\a,\\b,loc=7\n"
    "+inner:foo:\\d=h,\\y=a:z=\\b,y=\\b,loc=8\n"
    "}loc=9\n"
    "+comment:::col=0,txt= some comment\\, and another,loc=a\n"
    "}\n"
  };

  auto h = Hif::input(test);
  h.dump();

  h.each([](const Hif::Entry &ent) {
    std::cout << "type:" << ent.type << "\n";
    std::cout << "id:" << ent.id << "\n";
    for(const auto &e:ent.outputs) {
      std::cout << "  out.ecat:" << e.cat << "\n";
      std::cout << "  out.lhs:" << e.lhs << "\n";
      std::cout << "  out.rhs:" << e.rhs << "\n";
    }
  });

  EXPECT_NE(10,100);
}

