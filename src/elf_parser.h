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

#ifndef __FILE_BINDER__ELF_PARSER_H_
#define __FILE_BINDER__ELF_PARSER_H_

#include <elf.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace file_binder {

class ElfError : public std::runtime_error {
public:
    ElfError(const std::string& what);
    ~ElfError();
};

class ElfParser {
public:
    // ElfParser parses the contents of the ELF file.  It throws ElfError on
    // failure at any point of the parse.  The file descriptor must remain
    // valid for the lifetime of this class.
    explicit ElfParser(int fd);
    ~ElfParser();

    // Retrieves the interpreter for this ELF file.  It returns false if one
    // was not specified.
    bool GetInterpreter(std::string* interpreter);

    // Enumerates all of the dynamic library dependencies of this ELF file.
    std::vector<std::string> GetLibraryDependencies();
private:
    // Reads the program header at offset, widening as needed to an Elf64
    // formatted version.
    void ReadPHeader(size_t offset, Elf64_Phdr* hdr);

    // Reads the dynamic header at offset, widening as needed to an Elf64
    // formatted version.
    void ReadDHeader(size_t offset, Elf64_Dyn* hdr);

    int fd_;
    bool x64_;
    bool le_;

    // These values are in native byte order and have been zero-extended to the
    // 64-bit varieties.
    Elf64_Off phoff_;
    Elf64_Half phentsize_;
    Elf64_Half phnum_;

    Elf64_Off shoff_;
    Elf64_Half shentsize_;
    Elf64_Half shnum_;
    Elf64_Half shstrndx_;
};

}  // namespace file_binder

#endif  // __FILE_BINDER__ELF_PARSER_H_
