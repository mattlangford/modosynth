cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        "//engine",
        "//objects",
    ]
)

cc_binary(
    name = "tester",
    srcs = ["test.cc"],
    deps = [
        "//engine",
        "//objects",
    ]
)

cc_test(
    name = "test",
    srcs = glob(["test/*.cc"]),
    size = "small",
    linkstatic = 1, # important so GLFW can find it's functions
    data = ["//objects:config", "//objects:textures"],
    deps = [
        "//engine",
        "//synth",
        "//objects",
        "@gtest",
        "@gtest//:gtest_main",
    ]
)
