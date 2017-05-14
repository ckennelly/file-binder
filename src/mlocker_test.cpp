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

#include "mlocker.h"

#include <cstdlib>
#include <sys/sysmacros.h>

#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <regex>
#include <stdexcept>

namespace file_binder {
namespace {

uint64_t Dec(const std::string& s) {
    uint64_t ret;
    int ss = sscanf(s.c_str(), "%lu", &ret);
    if (ss != 1) {
        throw std::runtime_error("Invalid number");
    }
    return ret;
}

uint64_t Hex(const std::string& s) {
    uint64_t ret;
    int ss = sscanf(s.c_str(), "%lx", &ret);
    if (ss != 1) {
        throw std::runtime_error("Invalid number");
    }
    return ret;
}

// SmapsReader reads the contents of /proc/self/smaps and parses the entries.
class SmapsReader {
public:
    SmapsReader() {}

    // Parse is implemented in a standalone function, rather than the
    // constructor to allow us the use of gtest macros.
    void Parse() {
        std::vector<Entry> new_entries;

        // Build a regex to map find the file.  See
        // Documentation/filesystems/proc.txt in the kernel sources.
        std::regex re(
            // Address
            "^([0-9a-f]+)-([0-9a-f]+) "
            // Perms
            "[r-][w-][x-][ps] "
            // Offset
            "([0-9a-f]+) "
            // Dev major:minor
            "([0-9a-f]+):([0-9a-f]+) "
            // Inode
            "([0-9]+) *"
            // Filename
            "(.*)$");

        std::regex prop_re(
            // Property
            "^([a-zA-Z_]+): *"
            // Size
            "([0-9]+) kB$");

        std::ifstream smaps("/proc/self/smaps");

        // Prime the pump.
        std::string line;
        if (!std::getline(smaps, line)) {
            return;
        }

        // Our outer parsing loop maintains the invariant that line is always
        // the beginning of another entry, or blank.
        do {
            if (line.empty()) {
                break;
            }

            std::smatch match;
            ASSERT_TRUE(std::regex_match(line, match, re)) << line;
            ASSERT_EQ(8, match.size());

            Entry entry;
            entry.start_address =
                reinterpret_cast<void*>(Hex(match[1]));
            entry.end_address =
                reinterpret_cast<void*>(Hex(match[2]));
            entry.offset = Hex(match[3]);

            unsigned major = Hex(match[4]);
            unsigned minor = Hex(match[5]);
            entry.dev = makedev(major, minor);

            entry.inode = Dec(match[6]);
            entry.filename = match[7].str();

            // Iterate over contents.
            while (std::getline(smaps, line)) {
                std::smatch prop_match;
                if (!std::regex_match(line, prop_match, prop_re)) {
                    // Assume we are done with matching properties and are back
                    // to entries.
                    break;
                }
                ASSERT_EQ(3, prop_match.size());

                const std::string prop = prop_match[1];
                const uint64_t val = Dec(prop_match[2]);

                if (prop == "Size") {
                    entry.Size = val;
                } else if (prop == "Rss") {
                    entry.RSS = val;
                } else if (prop == "Locked") {
                    entry.Locked = val;
                } // TODO: Handle other properties.
            }

            new_entries.push_back(entry);
        } while(std::getline(smaps, line));

        using std::swap;
        swap(entries, new_entries);
    }

    struct Entry {
        void* start_address;
        void* end_address;
        // TODO:  Support permissions
        size_t offset;
        dev_t dev;
        ino_t inode;
        std::string filename;

        // Properties, in KB
        uint64_t Size;
        uint64_t RSS;
        uint64_t Locked;
    };

    std::vector<Entry> entries;
};

TEST(MLocker, Locked) {
    // Construct a temporary file, fill with some data.
    char name[] = "/tmp/mlocker.XXXXXXX";
    int fd = mkstemp(name);
    ASSERT_GE(fd, 0);

    const size_t page_size = sysconf(_SC_PAGESIZE);
    const size_t multiples = 4;
    std::unique_ptr<char[]> buf(new char[page_size]);
    memset(buf.get(), 'a', page_size);

    for (size_t i = 0; i < multiples; i++) {
        size_t offset = 0;
        while (offset < page_size) {
            ssize_t w = ::write(fd, buf.get(), page_size - offset);
            ASSERT_TRUE(w >= 0 || errno == EINTR);
            offset += w;
        }
    }

    struct stat s;
    int ret;
    do {
        ret = fstat(fd, &s);
    } while (ret < 0 && errno == EINTR);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(multiples * page_size, ret.st_size);

    // Mlock
    {
        MLocker mlocker;
        // If this fails, consult the output of `ulimit -l` default for
        // unprivileged users and adjust multiples accordingly.
        const auto token = mlocker.Lock(name);

        // Verify that name has been mlocked by scanning our own maps.
        SmapsReader p;
        p.Parse();

        bool found = false;
        for (const auto& entry : p.entries) {
            if (entry.dev != s.st_dev) {
                continue;
            }

            if (entry.inode != s.st_ino) {
                continue;
            }

            if (entry.filename != name) {
                continue;
            }

            // Found the entry, verify it is mlock'd.
            ASSERT_EQ(s.st_size / 1024, entry.Size);
            ASSERT_EQ(s.st_size / 1024, entry.RSS);
            ASSERT_EQ(s.st_size / 1024, entry.Locked);

            found = true;
            break;
        }

        EXPECT_TRUE(found);
    }

    // Cleanup.
    ::unlink(name);
    ::close(fd);
}

}  // namespace
}  // namespace file_binder
