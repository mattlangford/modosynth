cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        "@engine//ecs",
        "@engine//engine",
        "@engine//window",
        "//objects",
    ]
)

cc_binary(
    name = "test_main",
    srcs = ["test.cc"],
    deps = [
        "@engine//ecs",
        "@engine//engine",
        "//objects",
    ]
)

# cc_test(
#     name = "test",
#     srcs = glob(["test/*.cc"]),
#     size = "small",
#     linkstatic = 1, # important so GLFW can find it's functions
#     data = ["//objects:config", "//objects:textures"],
#     deps = [
#         "//engine",
#         "//synth",
#         "//objects",
#         "@gtest",
#         "@gtest//:gtest_main",
#     ]
# )
