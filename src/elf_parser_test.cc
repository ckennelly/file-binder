/**
 * File Binder - A File Loading Utility
 * (c) 2016-2017 Chris Kennelly <chris@ckennelly.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "elf_parser.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gtest/gtest.h>
#include <vector>
#include <string>

namespace file_binder {
namespace {

TEST(ElfParser, Dependencies) {
    const char* base_ptr = getenv("TEST_SRCDIR");
    const char* work_ptr = getenv("TEST_WORKSPACE");

    std::string base;
    if (base_ptr != nullptr) {
        base += base_ptr;
        base += "/";
    }
    if (work_ptr != nullptr) {
        base += work_ptr;
        base += "/";
    }

    struct TestCase {
        std::string filename;
        bool        static_linkage;

    };

    const std::vector<TestCase> binaries{
        {base + "src/testdata/hello_x64_dyn",    false},
        {base + "src/testdata/hello_x64_static", true},
        {base + "src/testdata/hello_x86_dyn",    false},
        {base + "src/testdata/hello_x86_static", true},
    };

    for (const auto& test_case : binaries) {
        const std::string& binary = test_case.filename;

        int fd;
        do {
            fd = open(binary.c_str(), O_RDONLY);
        } while (fd < 0 && errno == EINTR);
        ASSERT_GE(fd, 0) << "opening " << binary << " failed " << errno;

        ElfParser parser(fd);

        std::string interpreter;
        bool ret = parser.GetInterpreter(&interpreter);
        EXPECT_EQ(!test_case.static_linkage, ret);
        if (!test_case.static_linkage) {
            EXPECT_NE("", interpreter);
            // TODO:  Make this non-Linux specific...
            EXPECT_TRUE(interpreter.find("ld-linux") != std::string::npos)
                << interpreter;
            std::cout << "Found interpreter (" << interpreter
                      << ") for " << binary << std::endl;
        }

        std::vector<std::string> deps = parser.GetLibraryDependencies();
        if (test_case.static_linkage) {
            EXPECT_EQ(0, deps.size()) << binary;
        } else {
            EXPECT_NE(0, deps.size()) << binary;
        }

        std::cout << "Found dynamic library deps (";
        for (const auto& d : deps) {
            std::cout << d << ", ";
        }
        std::cout << ") for " << binary << std::endl;

        close(fd);
    }
}

}  // namespace
}  // namespace file_binder
