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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdexcept>

namespace file_binder {

MLocker::Token::Token(const std::string& path) : addr_(nullptr), size_(0) {
    // TODO:  Use RAII for this file descriptor.
    int fd;
    do {
        fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    } while (fd < 0 && errno == EINTR);

    if (fd < 0) {
        // TODO: Catch this.
        throw std::invalid_argument("Error opening: " + path);
    }

    struct stat buf;
    int ret;
    do {
        ret = fstat(fd, &buf);
    } while (ret != 0 && errno == EINTR);

    // TODO:  Handle ret < 0

    size_ = buf.st_size;
    addr_ = ::mmap(nullptr, size_, PROT_READ,
        MAP_SHARED | MAP_LOCKED | MAP_POPULATE, fd, 0);
    if (addr_ == MAP_FAILED) {
        ::close(fd);

        throw std::runtime_error("Unable to mmap");
    }

    ::close(fd);

    // MAP_LOCKED is not as strong as mlock.
    // TODO: Check this return value.
    ::mlock(addr_, size_);
}

MLocker::Token::~Token() {
    ::munmap(addr_, size_);
}

MLocker::Token::Token(Token&& rhs) :
        addr_(rhs.addr_), size_(rhs.size_) {
    rhs.addr_ = nullptr;
    rhs.size_ = 0;
}

MLocker::Token& MLocker::Token::operator=(Token&& rhs) {
    using std::swap;

    swap(addr_, rhs.addr_);
    swap(size_, rhs.size_);

    return *this;
}

MLocker::MLocker() {}
MLocker::~MLocker() {}

std::unique_ptr<MLocker::Token> MLocker::Lock(const std::string& path) const {
    return std::unique_ptr<Token>(new Token(path));
}

}   // namespace file_binder
