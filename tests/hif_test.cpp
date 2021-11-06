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

  fmt::print("hello world\n");

  EXPECT_NE(10,100);
}

