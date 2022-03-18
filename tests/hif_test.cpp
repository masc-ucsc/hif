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

  auto wr = Hif_write::create(fname, "testtool", "0.2.1");
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

  EXPECT_EQ(rd->get_tool(), "testtool");
  EXPECT_EQ(rd->get_version(), "0.2.1");

  int conta = 0;
  rd->each([&conta, &stmt](const Hif_base::Statement &stmt2) {
    EXPECT_EQ(stmt, stmt2);
    ++conta;
  });

  EXPECT_EQ(conta, 1);
}

TEST_F(Hif_test, Large_stmt) {

  std::string fname("hif_test_data2");

  auto wr = Hif_write::create(fname, "testtool", "0.0.3");
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

TEST_F(Hif_test, Statement_class_check) {
  std::string fname("hif_test_statement_class_check");
  
  Hif_base::Statement stmt;

  #define STMT_CLASS_CHECK(type)     \
  stmt = Hif_write::create_##type(); \
  EXPECT_TRUE(stmt.is_##type());
  
  STMT_CLASS_CHECK(node);
  STMT_CLASS_CHECK(assign);
  STMT_CLASS_CHECK(attr);
  STMT_CLASS_CHECK(open_call);
  STMT_CLASS_CHECK(closed_call);
  STMT_CLASS_CHECK(open_def);
  STMT_CLASS_CHECK(closed_def);
  STMT_CLASS_CHECK(end);
  STMT_CLASS_CHECK(use);
}

TEST_F(Hif_test, empty_string) {
  std::string fname = "hif_empty_string";

  auto wr = Hif_write::create(fname, "lnast", "test");
  EXPECT_NE(wr, nullptr);
  auto stmt = Hif_write::create_attr();
  stmt.add_attr("name", "");
  wr->add(stmt);

  wr = nullptr;

  auto rd = Hif_read::open(fname);
  EXPECT_NE(rd, nullptr);
  rd->get_tool();
  rd->get_version();
  rd->next_stmt();
  stmt = rd->get_current_stmt();
  EXPECT_EQ(stmt.attr[0].lhs, "name");
  EXPECT_EQ(stmt.attr[0].rhs, "");

  rd = nullptr;
}
