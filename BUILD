cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        ":engine",
    ]
)

cc_library(
    name = "engine",
    srcs = glob(["engine/*.cc"]),
    hdrs = glob(["engine/*.hh"]),
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
        "@iterate//:iterate",
    ]
)

cc_test(
    name = "engine_test",
    srcs = glob(["engine/test/*.cc"]),
    deps = [
        ":engine",
        "@gtest//:gtest",
        "@gtest//:gtest_main",
    ]
)
