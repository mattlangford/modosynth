cc_library(
    name = "window",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.hh"]),
    visibility = ["//visibility:public"],
    deps = [
        "//engine",
        "@glfw//:glfw",
        "@eigen//:eigen",
    ]
)

cc_test(
    name = "window_test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    deps = [
        ":window",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
