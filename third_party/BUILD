cc_library(
    name = "gtest",
    srcs = [
        "googletest/googlemock/src/gmock-all.cc",
        "googletest/googletest/src/gtest-all.cc",
    ],
    hdrs = glob([
        "googletest/**/*.h",
        "googletest/googletest/src/*.cc",
        "googletest/googlemock/src/*.cc",
    ]),
    includes = [
        "googletest/googlemock",
        "googletest/googlemock/include",
        "googletest/googletest",
        "googletest/googletest/include",
    ],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "gtest_main",
    srcs = ["googletest/googlemock/src/gmock_main.cc"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [":gtest"],
)
