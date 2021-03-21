cc_library(
    name = "engine",
    srcs = glob(["*.cc", "renderer/*.cc"]),
    hdrs = glob(["*.hh", "renderer/*.hh"]),
    visibility = ["//visibility:public"],
    data = glob(["renderer/*.vert", "renderer/*.geom", "renderer/*.frag"]),
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
    ]
)

cc_test(
    name = "engine_test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    deps = [
        ":engine",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
