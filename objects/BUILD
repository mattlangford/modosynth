cc_library(
    name = "objects",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.hh"]),
    visibility = ["//visibility:public"],
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
        "//engine"
    ]
)

cc_test(
    name = "objects_test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    deps = [
        ":objects",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
