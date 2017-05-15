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
        ":elf_parser",
        ":mlocker",
    ],
)

cc_library(
    name = "elf_parser",
    hdrs = ["elf_parser.h"],
    srcs = ["elf_parser.cc"],
)

cc_test(
    name = "elf_parser_test",
    srcs = ["elf_parser_test.cc"],
    deps = [
        ":elf_parser",
        "//third_party:gtest_main",
    ],
    data = [
        "//src/testdata:hello_x64_dyn",
        "//src/testdata:hello_x64_static",
        "//src/testdata:hello_x86_dyn",
        "//src/testdata:hello_x86_static",
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
