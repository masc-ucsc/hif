load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_library(
    name = "hif",
    srcs = glob(
        ["*.cpp"],
        exclude = ["*test*.cpp", "*bench*.cpp"],
    ),
    hdrs = glob(["*.hpp"]),
    visibility = ["//visibility:public"],
    deps = [
      "@com_google_absl//absl/container:flat_hash_map",
    ],
)

