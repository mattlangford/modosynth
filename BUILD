cc_binary(
    name = "main",
    srcs = ["main.cc"],
    deps = [
        "@glfw//:glfw",
        "@eigen//:eigen",
        ":bitmap",
    ]
)

cc_binary(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        "@glfw//:glfw",
    ]
)

cc_library(
    name = "bitmap",
    srcs = ["bitmap.cc"],
    hdrs = ["bitmap.hh"],
    deps = []
)
