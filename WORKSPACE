workspace(name = "dynasm_brainfuck")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

# LuaJIT for DynASM
git_repository(
    name = "luajit",
    remote = "https://github.com/LuaJIT/LuaJIT.git",
    tag = "v2.1.0-beta3",
    build_file = "//:luajit.BUILD",
)
