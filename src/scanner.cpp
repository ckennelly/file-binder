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

#include "scanner.h"

#include <cassert>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "elf_parser.h"

namespace file_binder {

Scanner::Scanner() : filesystem_(new Filesystem()), mlocker_(new MLocker()) {}
Scanner::~Scanner() {}

void Scanner::SetPaths(std::vector<std::string> paths) {
    pending_paths_ = std::move(paths);
}

void Scanner::Run() {
    using std::placeholders::_1;
    using std::placeholders::_2;
    const auto& callback = std::bind(&Scanner::Walk, this, _1, _2);

    // We allow our Walk function to build up additional paths for us to scan.
    // To avoid iterator invalidation issues, we move the contents of the
    // member variable into a stack variable and build up additional work items
    // in pending_paths_.
    std::vector<std::string> paths;
    while (true) {
        while (!pending_paths_.empty()) {
            paths.swap(pending_paths_);

            for (const auto& path : paths) {
                filesystem_->Walk(path, callback);
            }

            paths.clear();
        }

        paths.clear();
    }

    // TODO:  Configure inotify

    while (true) {
        sleep(3600);
    }
}

void Scanner::Walk(const std::string& path, const struct stat& buf) {
    if (!S_ISREG(buf.st_mode)) {
        // Ignore non-files.
        return;
    }

    // Scan ELF-type files for their runtime dependencies.
    {
        int fd;
        do {
            fd = open(path.c_str(), O_RDONLY);
        } while (fd < 0 && errno != EINTR);

        if (fd >= 0) {
            try {
                ElfParser elf(fd);

                std::string interpreter;
                bool has_interpreter = elf.GetInterpreter(&interpreter);
                if (has_interpreter) {
                    pending_paths_.emplace_back(std::move(interpreter));
                }

                std::vector<std::string> deps = elf.GetLibraryDependencies();
                pending_paths_.insert(
                    pending_paths_.begin(), deps.begin(), deps.end());
            } catch (ElfError& ex) {
                // Ignore.
            }

            close(fd);
        }
    }

    // TODO:  Insert dependencies into inotify watch.

    // Lock file into memory, hold a reference to it.
    locks_.emplace(path, mlocker_->Lock(path));
}

}  // namespace file_binder
