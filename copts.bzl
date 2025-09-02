"""Centralized compiler options for bf-dynasm."""

BF_DEFAULT_COPTS = [
    "-Wall",
    "-Wextra",
    "-O2",
    "-g",
    "-std=c23",
] + select({
    "//:x64_darwin_config": ["-target", "x86_64-apple-macos10.12"],
    "//conditions:default": [],
})

BF_DEFAULT_LINKOPTS = select({
    "//:x64_darwin_config": ["-target", "x86_64-apple-macos10.12"],
    "//conditions:default": [],
})
