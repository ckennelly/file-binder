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

#include <cstdio>

#include <string>
#include <vector>

#include "scanner.h"

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr,
            "Usage: %s <path-to-lock> [<path-to-lock> ...]\n\n"
            "%s scans the paths specified for files to lock into memory.\n",
            argv[0], argv[0]);
        return 1;
    }

    // TODO:  Support daemonization.

    std::vector<std::string> paths;
    for (int i = 1; i < argc; i++) {
        paths.emplace_back(argv[i]);
    }

    file_binder::Scanner s;
    s.SetPaths(std::move(paths));
    s.Run();

    return 0;
}
