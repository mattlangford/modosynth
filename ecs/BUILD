cc_library(
    name = "ecs",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.hh"]),
    visibility = ["//visibility:public"],
    deps = ["@yaml-cpp"],
)

cc_test(
    name = "ecs_test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    deps = [
        ":ecs",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
