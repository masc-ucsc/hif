load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest",
    tag = "release-1.11.0",
)

#git_repository(
#    name = "glog",
#    remote = "https://github.com/google/glog.git",
#    tag = "v0.4.0",
#)

git_repository(
    name = "com_google_benchmark",
    remote = "https://github.com/google/benchmark.git",
    tag = "v1.8.5",
)

#git_repository(
#    name = "com_github_gflags_gflags",
#    remote = "https://github.com/gflags/gflags.git",
#    tag = "v2.2.2",
#)

# optional library. Used only when -DUSE_ABSL_MAP set (faster)
http_archive(
  name = "com_google_absl",
  urls = ["https://github.com/abseil/abseil-cpp/archive/04610889a913d29037205ca72e9d7fd7acc925fe.zip"],
  strip_prefix = "abseil-cpp-04610889a913d29037205ca72e9d7fd7acc925fe",
)
