cc_library(
    name = "objects",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.hh"]),
    data = glob(["*.yml", "*.bmp"]),
    visibility = ["//visibility:public"],
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
        "@yaml-cpp",
        ":blocks",
        "//engine",
        "//synth"
    ]
)

cc_library(
    name = "blocks",
    srcs = glob(["blocks/*.cc"]),
    hdrs = glob(["blocks/*.hh"]),
    deps = [
        "@eigen//:eigen",
        "//engine",
        "//synth"
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
