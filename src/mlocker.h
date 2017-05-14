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

#ifndef __FILE_BINDER__MLOCKER_H__
#define __FILE_BINDER__MLOCKER_H__

#include <memory>
#include <string>

namespace file_binder {

class MLocker {
public:
    class Token {
    public:
        Token(Token&&);
        Token& operator=(Token&&);

        virtual ~Token();
    protected:
        friend class MLocker;

        Token(const std::string& path);
    private:
        Token(const Token&) = delete;
        Token& operator=(const Token&) = delete;

        void* addr_;
        size_t size_;
    };

    MLocker();
    virtual ~MLocker();

    virtual std::unique_ptr<Token> Lock(const std::string& path) const;
};

}  // namespace file_binder

#endif  // __FILE_BINDER__MLOCKER_H__
