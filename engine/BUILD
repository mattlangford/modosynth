cc_library(
    name = "engine",
    srcs = glob(["*.cc", "objects/*.cc"]),
    hdrs = glob(["*.hh", "objects/*.hh"]),
    visibility = ["//visibility:public"],
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
    ]
)

cc_test(
    name = "engine_test",
    srcs = glob(["engine/test/*.cc"]),
    size = "small",
    deps = [
        ":engine",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
