genrule(
    name="cmake",
    srcs=glob(["src/*", "CMakeLists.txt", "cmake/*", "doc/*", "soundio/*"]),
    outs=["libsoundio.a"],
    cmd = " \
        cmake \
            -DBUILD_DYNAMIC_LIBS=OFF \
            -DBUILD_TESTS=OFF \
            -DBUILD_EXAMPLE_PROGRAMS=OFF
            -DCMAKE_BUILD_TYPE=Release \
            `dirname $(location CMakeLists.txt)` && \
        make && \
        cp libsoundio.a $@ \
    "
)

cc_library(
    name = "soundio",
    srcs = ["libsoundio.a"],
    hdrs = glob(["soundio/*.h"]),
    includes = ["."],
    # TODO: Should probably do a platform switch here?
    linkopts= ["-framework CoreFoundation", "-framework CoreAudio", "-framework AudioToolbox", "-lpthread"],
)
