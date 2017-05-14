cc_library(
    name = "scanner",
    hdrs = [
        "filesystem.h",
        "mlocker.h",
        "scanner.h",
    ],
    srcs = [
        "filesystem.cpp",
        "mlocker.cpp",
        "scanner.cpp",
    ],
)

cc_binary(
    name = "binder",
    srcs = ["binder.cpp"],
    deps = [
        ":scanner",
    ],
)

cc_binary(
    name = "rlimit",
    srcs = ["rlimit.c"],
)
