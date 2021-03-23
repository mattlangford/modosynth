filegroup(
    name="config",
    srcs = glob(["*.yml"]),
    visibility = ["//visibility:public"],
)

filegroup(
    name="textures",
    srcs = glob(["*.bmp"]),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "objects",
    srcs = glob(["*.cc", "blocks/*.cc"]),
    hdrs = glob(["*.hh", "blocks/*.hh"]),
    visibility = ["//visibility:public"],
    data = [":config", ":textures"],
    deps = [
        "//ecs",
        "//engine",
        "//synth",
        "@glfw//:glfw",
        "@eigen//:eigen",
        "@yaml-cpp",
    ]
)

cc_test(
    name = "objects_test",
    srcs = glob(["test/*.cc", "blocks/test/*.cc"]),
    size = "small",
    deps = [
        ":objects",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
