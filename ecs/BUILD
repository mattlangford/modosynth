cc_library(
    name = "ecs",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.hh"]),
    visibility = ["//visibility:public"],
    deps = []
)

cc_test(
    name = "ecs_test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    deps = [
        ":esc",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
