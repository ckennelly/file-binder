ADD_LIBRARY(scanner SHARED
    "filesystem.h" "filesystem.cpp"
    "mlocker.h" "mlocker.cpp"
    "scanner.cpp" "scanner.h")

ADD_EXECUTABLE(binder "binder.cpp")
ADD_DEPENDENCIES(binder scanner)
TARGET_LINK_LIBRARIES(binder scanner)

ADD_EXECUTABLE(rlimit "rlimit.c")

INCLUDE_DIRECTORIES(.)
