load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_test(
    name = "hif_test",
    srcs = ["hif_test.cpp"],
    deps = [
      "//hif",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "hif_bench",
    srcs = ["hif_bench.cpp"],
    deps = [
      "//hif",
        "@google_benchmark//:benchmark",
    ],
)

cc_binary(
    name = "hif_cat",
    srcs = ["hif_cat.cpp"],
    deps = [
      "//hif",
    ],
)

cc_binary(
    name = "hif_rand_test",
    srcs = ["hif_rand_test.cpp"],
    deps = [
      "//hif",
    ],
)
