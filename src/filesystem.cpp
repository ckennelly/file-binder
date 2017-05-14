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

#include "filesystem.h"

#include <ftw.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mutex>

namespace file_binder {
namespace {

// callback_mx protects callback
std::mutex callback_mx;
std::function<void(const std::string&, const struct stat&)> callback_;

int walker(const char *fpath, const struct stat *sb, int typeflag,
           struct FTW *ftwbuf) {
    (void) typeflag;
    (void) ftwbuf;

    std::string path(fpath);
    callback_(path, *sb);
    return 0;
}

}  // namespace

Filesystem::Filesystem() {}
Filesystem::~Filesystem() {}

void Filesystem::Walk(
        const std::string& path,
        std::function<void(const std::string&, const struct ::stat&)> callback) {
    struct stat buf;
    int ret = stat(path.c_str(), &buf);
    if (ret < 0) {
        // TODO:  Handle this error.
        return;
    }

    if (!S_ISDIR(buf.st_mode)) {
        // Execute callback directly.
        callback(path, buf);
        return;
    }

    // TODO:  Replace nftw with a more reentrant implementation.  As we have to
    // interact with global state for nftw, we hold callback_mx while nftw is
    // running.
    std::unique_lock<std::mutex> l(callback_mx);
    callback_ = callback;
    nftw(path.c_str(), walker, 16, FTW_PHYS);
}

}  // namespace file_binder
