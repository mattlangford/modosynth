cc_library(
    name = "synth",
    srcs = glob(["*.cc"]),
    hdrs = glob(["*.hh"]),
    visibility = ["//visibility:public"],
    deps = [
        "@eigen//:eigen",
        "@libsoundio//:soundio",
    ]
)

cc_test(
    name = "synth_test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    deps = [
        ":synth",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
