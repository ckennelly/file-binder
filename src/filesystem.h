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

#ifndef __FILE_BINDER__FILESYSTEM_H_
#define __FILE_BINDER__FILESYSTEM_H_

#include <sys/types.h>
#include <sys/stat.h>

#include <functional>

namespace file_binder {

class Filesystem {
public:
    Filesystem();
    virtual ~Filesystem();

    // Walks the filesystem tree at and below path, calling the callback for
    // each file/directory found.
    virtual void Walk(
        const std::string& path,
        std::function<void(const std::string&, const struct stat&)> callback);
};

}  // namespace file_binder

#endif  // __FILE_BINDER__FILESYSTEM_H_
