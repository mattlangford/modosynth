WIN32_DEFINES = [
    "_GLFW_WIN32",
]

WIN32_HDRS = [
    "src/win32_joystick.h",
    "src/win32_platform.h",
    "src/wgl_context.h",
]

WIN32_SRCS = [
    "src/win32_init.c",
    "src/win32_joystick.c",
    "src/win32_monitor.c",
    "src/win32_thread.c",
    "src/win32_time.c",
    "src/win32_window.c",
    "src/wgl_context.c",
]

WIN32_LINKOPTS = [
    "-DEFAULTLIB:user32.lib",
    "-DEFAULTLIB:gdi32.lib",
    "-DEFAULTLIB:shell32.lib",
]

LINUX_DEFINES = [
    "_GLFW_HAS_XF86VM",
    "_GLFW_X11",
]

LINUX_HDRS = [
    "src/glx_context.h",
    "src/linux_joystick.h",
    "src/posix_thread.h",
    "src/posix_time.h",
    "src/x11_platform.h",
]

LINUX_SRCS = [
    "src/glx_context.c",
    "src/linux_joystick.c",
    "src/posix_thread.c",
    "src/posix_time.c",
    "src/x11_init.c",
    "src/x11_monitor.c",
    "src/x11_window.c",
]

LINUX_LINKOPTS = [
    "-lGL",
    "-lXrandr",
    "-lXext",
    "-lX11",
    "-ldl",
    "-lpthread",
]

DARWIN_DEFINES = [
    "_GLFW_COCOA",
    "_GLFW_NSGL",
    "_GLFW_NO_DLOAD_WINMM",
    "_GLFW_USE_OPENGL",
]

DARWIN_HDRS = [
    "src/cocoa_joystick.h",
    "src/cocoa_platform.h",
    "src/glx_context.h",
    "src/nsgl_context.h",
    "src/null_joystick.h",
    "src/null_platform.h",
    "src/posix_thread.h",
    "src/wl_platform.h",
]


DARWIN_SRCS = [
    "src/cocoa_time.c",
    "src/posix_thread.c",
    "src/cocoa_init.m",
    "src/cocoa_joystick.m",
    "src/cocoa_monitor.m",
    "src/cocoa_window.m",
    "src/nsgl_context.m",
]

DARWIN_LINKOPTS = [
    "-framework OpenGL",
    "-framework Cocoa",
    "-framework IOKit",
    "-framework CoreFoundation",
    "-framework CoreVideo",
]

# It seems like bazel doesn't like to mix objective-C files in with C files. We'll have to build it like this in order
# to get it linking correctly on macos
objc_library(
    name = "glfw_src_apple",
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
        "src/egl_context.h",
        "src/internal.h",
        "src/osmesa_context.h",
        "src/mappings.h",
        "src/xkb_unicode.h"] + DARWIN_HDRS,
    srcs = [
        "src/context.c",
        "src/egl_context.c",
        "src/init.c",
        "src/input.c",
        "src/osmesa_context.c",
        "src/monitor.c",
        "src/vulkan.c",
        "src/window.c",
        "src/xkb_unicode.c"] + DARWIN_SRCS,
    defines = DARWIN_DEFINES,
    # Seems like there are errors that require this
    copts = ["-fno-objc-arc"]
)

cc_library(
    name = "glfw_src",
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
        "src/egl_context.h",
        "src/internal.h",
        "src/osmesa_context.h",
        "src/mappings.h",
        "src/xkb_unicode.h",
    ] + select({
        "@bazel_tools//src/conditions:windows": WIN32_HDRS,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_HDRS,
    }),
    srcs = [
        "src/context.c",
        "src/egl_context.c",
        "src/init.c",
        "src/input.c",
        "src/osmesa_context.c",
        "src/monitor.c",
        "src/vulkan.c",
        "src/window.c",
        "src/xkb_unicode.c",
    ] + select({
        "@bazel_tools//src/conditions:windows": WIN32_SRCS,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_SRCS,
    }),
    defines = select({
        "@bazel_tools//src/conditions:windows": WIN32_DEFINES,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_DEFINES,
    }),
)

cc_library(
    name = "glfw",
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
    ],
    linkopts = select({
        "@bazel_tools//src/conditions:windows": WIN32_LINKOPTS,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_LINKOPTS,
        "@bazel_tools//src/conditions:darwin": DARWIN_LINKOPTS,
    }),
    deps = select({
        "@bazel_tools//src/conditions:windows": [":glfw_src"],
        "@bazel_tools//src/conditions:linux_x86_64": [":glfw_src"],
        "@bazel_tools//src/conditions:darwin": [":glfw_src_apple"],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    defines = ["GL_SILENCE_DEPRECATION"],
)
