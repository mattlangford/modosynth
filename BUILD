cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
        ":engine",
    ]
)

cc_binary(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        "@glfw//:glfw",
        ":bitmap",
    ]
)

cc_library(
    name = "engine",
    srcs = glob(["engine/*.cc"]),
    hdrs = glob(["engine/*.hh"]),
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
    ]
)
