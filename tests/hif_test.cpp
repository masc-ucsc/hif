//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "hif/hif_read.hpp"
#include "hif/hif_write.hpp"

class Hif_test : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {
    // Graph_library::sync_all();
  }
};

TEST_F(Hif_test, Trivial_test1) {
  std::string fname("hif_test_data1");

  auto wr = Hif_write::create(fname);
  EXPECT_NE(wr, nullptr);

  auto stmt = Hif_write::create_assign();

  stmt.instance = "jojojo";
  stmt.add_input("A", "0");
  stmt.add_input("A", "1");
  stmt.add_input("A", "2");
  stmt.add_input("A", "3");

  stmt.add_output("Z");

  stmt.add_attr("loc", "3");

  wr->add(stmt);

  wr = nullptr;  // close

  auto rd = Hif_read::open(fname);
  EXPECT_NE(rd, nullptr);

  int conta = 0;
  rd->each([&conta, &stmt](const Hif_base::Statement &stmt2) {
    EXPECT_EQ(stmt, stmt2);
    ++conta;
  });

  EXPECT_EQ(conta, 1);
}

TEST_F(Hif_test, Large_stmt) {

  std::string fname("hif_test_data2");

  auto wr = Hif_write::create(fname);
  EXPECT_NE(wr, nullptr);

  auto stmt = Hif_write::create_assign();

  stmt.instance = "jojojo";

  for(auto i=0u;i<1024;++i) {
    stmt.add_input(std::to_string(i), std::string("a_longer_string_") + std::to_string(i));
  }
  for(auto i=0u;i<1024;++i) {
    stmt.add_output(std::to_string(i) + "_out", std::string("a_longer_string_") + std::to_string(i));
  }

  wr->add(stmt);

  wr = nullptr;  // close

  auto rd = Hif_read::open(fname);
  EXPECT_NE(rd, nullptr);

  int conta = 0;
  rd->each([&conta, &stmt](const Hif_base::Statement &stmt2) {
    EXPECT_EQ(stmt, stmt2);
    ++conta;
  });

  EXPECT_EQ(conta, 1);
}

