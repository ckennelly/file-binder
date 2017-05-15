package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "hello_x64_static",
    srcs = ["helloworld.c"],
    linkstatic = 1,
    copts = ["-m64"],
    linkopts = ["-static"],
    testonly = 1,
)

cc_binary(
    name = "hello_x64_dyn",
    srcs = ["helloworld.c"],
    linkstatic = 0,
    copts = ["-m64"],
    testonly = 1,
)

cc_binary(
    name = "hello_x86_static",
    srcs = ["helloworld.c"],
    linkstatic = 1,
    copts = ["-m32"],
    linkopts = ["-static", "-m32"],
    testonly = 1,
)

cc_binary(
    name = "hello_x86_dyn",
    srcs = ["helloworld.c"],
    linkstatic = 1,
    copts = ["-m32"],
    linkopts = ["-m32"],
    testonly = 1,
)
