package(default_visibility = ["//visibility:public"])

filegroup(
    name = "dynasm",
    srcs = glob(["dynasm/*"]),
)

cc_library(
    name = "dynasm_headers",
    hdrs = glob(["dynasm/*.h"]),
    includes = ["dynasm"],
)
