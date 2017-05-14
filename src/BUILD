cc_library(
    name = "scanner",
    hdrs = [
        "filesystem.h",
        "scanner.h",
    ],
    srcs = [
        "filesystem.cpp",
        "scanner.cpp",
    ],
    deps = [
        ":mlocker",
    ],
)

cc_library(
    name = "mlocker",
    hdrs = ["mlocker.h"],
    srcs = ["mlocker.cpp"],
)

cc_test(
    name = "mlocker_test",
    srcs = ["mlocker_test.cpp"],
    deps = [
        ":mlocker",
        "//third_party:gtest_main",
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
