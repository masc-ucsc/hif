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

  for (auto i = 0u; i < 1024; ++i) {
    stmt.add_input(std::to_string(i),
                   std::string("a_longer_string_") + std::to_string(i));
  }
  for (auto i = 0u; i < 1024; ++i) {
    stmt.add_output(std::to_string(i) + "_out",
                    std::string("a_longer_string_") + std::to_string(i));
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

#define STMT_CLASS_CHECK(type)       \
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

std::string get_random_string() {
  std::string str;

  static int counter = 0;

  if (rand() & 1) {
    str = std::string("$");
  }
  str += std::to_string(counter);
  if (rand() & 1) {
    str += "something_quite_large_and_not_reusable";
  }
  if (rand() & 1) {
    str += std::to_string(1020303 + counter);
  }

  ++counter;

  return str;
}

TEST_F(Hif_test, strings_match) {
  std::string fname("hif_test_strings_match");

  std::vector<std::string> inp_vector;
  std::vector<std::string> out_vector;

  {
    auto wr = Hif_write::create(fname, "testtool", "0.0.4");
    EXPECT_NE(wr, nullptr);

    for (int64_t i = 0; i < 1024; ++i) {
      Hif_base::Statement stmt;

      if (i & 1) {
        stmt = Hif_write::create_assign();
      } else {
        stmt = Hif_write::create_node();
      }

      std::string lhs = get_random_string();
      std::string rhs = get_random_string();

      inp_vector.emplace_back(lhs);
      out_vector.emplace_back(rhs);

      stmt.add_input(lhs, i);
      stmt.add_output(rhs, i);

      wr->add(stmt);
    }

    // wr out of scope, closes the Hif_write
  }

  {
    auto rd = Hif_read::open(fname);
    EXPECT_NE(rd, nullptr);

    int conta = 0;
    rd->each([&conta, &inp_vector, &out_vector](const Hif_base::Statement &stmt2) {
      if (conta & 1) {
        EXPECT_TRUE(stmt2.is_assign());
      } else {
        EXPECT_TRUE(stmt2.is_node());
      }

      EXPECT_TRUE(stmt2.attr.empty());  // FIXME: add random attributes too
      EXPECT_EQ(stmt2.io.size(), 2);

      for (const auto &io : stmt2.io) {
        EXPECT_TRUE(io.is_lhs_string());
        EXPECT_TRUE(io.is_rhs_int64());

        auto v = io.get_rhs_int64();
        EXPECT_EQ(conta, v);

        if (io.input) {
          EXPECT_EQ(inp_vector[conta], io.lhs);
        } else {
          EXPECT_EQ(out_vector[conta], io.lhs);
        }
      }

      ++conta;
    });

    EXPECT_EQ(conta, inp_vector.size());
    EXPECT_EQ(conta, out_vector.size());
  }

  {  // Same test but check different API
    auto rd = Hif_read::open(fname);
    EXPECT_NE(rd, nullptr);

    rd->next_stmt();  // Skip verstion/tool

    int conta = 0;
    do {
      auto stmt2 = rd->get_current_stmt();

      if (conta & 1) {
        EXPECT_TRUE(stmt2.is_assign());
      } else {
        EXPECT_TRUE(stmt2.is_node());
      }

      EXPECT_TRUE(stmt2.attr.empty());  // FIXME: add random attributes too
      EXPECT_EQ(stmt2.io.size(), 2);

      for (const auto &io : stmt2.io) {
        EXPECT_TRUE(io.is_lhs_string());
        EXPECT_TRUE(io.is_rhs_int64());

        auto v = io.get_rhs_int64();
        EXPECT_EQ(conta, v);

        if (io.input) {
          EXPECT_EQ(inp_vector[conta], io.lhs);
        } else {
          EXPECT_EQ(out_vector[conta], io.lhs);
        }
      }

      ++conta;
    } while (rd->next_stmt());

    EXPECT_EQ(conta, inp_vector.size());
    EXPECT_EQ(conta, out_vector.size());
  }
}
