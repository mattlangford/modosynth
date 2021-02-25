load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

git_repository(
    name = "gtest",
    remote = "https://github.com/google/googletest",
    # If I use branch = "v1.10.x" I get warnings that say I should use these two lines instead
    commit = "703bd9caab50b139428cea1aaff9974ebee5742e",
    shallow_since = "1570114335 -0400",
)

git_repository(
    name = "iterate",
    remote = "https://github.com/mattlangford/iterate",
    commit = "97f29e102da0e14a7acdb83b4dc76ac949c8471f",
    shallow_since = "1611765464 -0500",
)

new_git_repository(
    name = "glfw",
    remote = "https://github.com/glfw/glfw.git",
    commit = "8d7e5cdb49a1a5247df612157ecffdd8e68923d2",
    build_file = "//third_party:glfw.BUILD",
    # If I don't include this it gives warnings
    shallow_since = "1551813720 +0100",
)

http_archive(
    name = "eigen",
    url = "https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz",
    sha256 = "7985975b787340124786f092b3a07d594b2e9cd53bbfe5f3d9b1daee7d55f56f",
    build_file = "//third_party:eigen.BUILD",
    strip_prefix="eigen-3.3.9"
)
