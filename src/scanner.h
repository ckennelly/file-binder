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

#ifndef __FILE_BINDER__SCANNER_H__
#define __FILE_BINDER__SCANNER_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "filesystem.h"
#include "mlocker.h"

namespace file_binder {

class Scanner {
public:
    Scanner();
    ~Scanner();

    void SetPaths(std::vector<std::string> paths);

    void Run();
private:
    void Walk(
        const std::string& path,
        const struct stat& buf);

    std::unique_ptr<Filesystem> filesystem_;
    std::unique_ptr<MLocker> mlocker_;

    std::vector<std::string> paths_;
    // Mapping of paths to mlock tokens.
    std::unordered_map<std::string, std::unique_ptr<MLocker::Token>> locks_;
};

}  // namespace file_binder

#endif  // __FILE_BINDER__SCANNER_H__
