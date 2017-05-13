CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Warnings
SET(WARNINGS "-Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls -Winline -Wuninitialized -Werror -Wno-missing-field-initializers")
SET(CMAKE_CXX_FLAGS "${WARNINGS} -Wnon-virtual-dtor -g")

# C++11 support
INCLUDE(TestCXXAcceptsFlag)
CHECK_CXX_ACCEPTS_FLAG("-std=c++11" USE11FLAG)
CHECK_CXX_ACCEPTS_FLAG("-std=c++0x" USE0xFLAG)
IF(USE11FLAG)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSEIF(USE0xFLAG)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSE()
    MESSAGE(ERROR "C++0x/C++11 support is required.")
ENDIF()

# Build targets
ENABLE_TESTING()
ADD_SUBDIRECTORY(src)
