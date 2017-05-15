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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <elf.h>
#include <sys/types.h>
#include <unistd.h>

#include <unordered_set>

namespace file_binder {
namespace {

// Reads at most size bytes into buf.  Returns the number of bytes successfully
// read.
size_t TryReadBytesAtOffset(int fd, size_t offset, uint8_t* buf, size_t size) {
    int ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        throw ElfError("Unable to seek");
    }

    ssize_t chunk;
    do {
        chunk = read(fd, buf, size);
    } while (chunk < 0 && errno == EINTR);

    if (chunk < 0) {
        throw ElfError("Unable to read");
    }

    return static_cast<size_t>(chunk);
}

// Reads exactly size bytes into buf.  It throws an ElfError on failure.
void ReadBytesAtOffset(int fd, size_t offset, uint8_t* buf, size_t size) {
    int ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        throw ElfError("Unable to seek");
    }

    size_t consumed = 0;
    while (consumed < size) {
        ssize_t chunk = read(fd, buf + consumed, size - consumed);
        if (chunk < 0) {
            if (errno == EINTR) {
                continue;
            }

            throw ElfError("Unable to read");
        } else if (chunk == 0) {
            throw ElfError("Premature EOF");
        }

        consumed += static_cast<size_t>(chunk);
    }
}

bool IsLittleEndian() {
    const uint16_t x = 1;
    return *reinterpret_cast<const uint8_t*>(&x) == 1;
}

template<typename T>
T ByteSwap(T t) {
    static_assert(sizeof(t) == 0, "Not implemented");
}

template<>
uint16_t ByteSwap(uint16_t t) {
    return
        ((t & 0x00FF) << 8) |
        ((t & 0xFF00) >> 8);
}

template<>
uint32_t ByteSwap(uint32_t t) {
    return
        ((t & 0x000000FF) << 24) |
        ((t & 0x0000FF00) <<  8) |
        ((t & 0x00FF0000) >>  8) |
        ((t & 0xFF000000) >> 24);
}

template<>
int32_t ByteSwap(int32_t t) {
    return static_cast<int32_t>(ByteSwap(static_cast<uint32_t>(t)));
}

template<>
uint64_t ByteSwap(uint64_t t) {
    return
        ((t & 0x00000000000000FFULL) << 56) |
        ((t & 0x000000000000FF00ULL) << 40) |
        ((t & 0x0000000000FF0000ULL) << 24) |
        ((t & 0x00000000FF000000ULL) <<  8) |
        ((t & 0x000000FF00000000ULL) >>  8) |
        ((t & 0x0000FF0000000000ULL) >> 24) |
        ((t & 0x00FF000000000000ULL) >> 40) |
        ((t & 0xFF00000000000000ULL) >> 56);
}

template<>
int64_t ByteSwap(int64_t t) {
    return static_cast<int64_t>(ByteSwap(static_cast<uint64_t>(t)));
}

// Swap the byte order of an ELF header.
template<typename T>
void SwapHeader(T& hdr) {
    static_assert(
        std::is_same<T, Elf32_Ehdr>::value ||
        std::is_same<T, Elf64_Ehdr>::value,
        "Not an Ehdr");

    hdr.e_type      = ByteSwap(hdr.e_type);
    hdr.e_machine   = ByteSwap(hdr.e_machine);
    hdr.e_version   = ByteSwap(hdr.e_version);
    hdr.e_entry     = ByteSwap(hdr.e_entry);
    hdr.e_phoff     = ByteSwap(hdr.e_phoff);
    hdr.e_shoff     = ByteSwap(hdr.e_shoff);
    hdr.e_flags     = ByteSwap(hdr.e_flags);
    hdr.e_ehsize    = ByteSwap(hdr.e_ehsize);
    hdr.e_phentsize = ByteSwap(hdr.e_phentsize);
    hdr.e_phnum     = ByteSwap(hdr.e_phnum);
    hdr.e_shentsize = ByteSwap(hdr.e_shentsize);
    hdr.e_shnum     = ByteSwap(hdr.e_shnum);
    hdr.e_shstrndx  = ByteSwap(hdr.e_shstrndx);
}

template<typename T>
void SwapPHeader(T& hdr) {
    static_assert(
        std::is_same<T, Elf32_Phdr>::value ||
        std::is_same<T, Elf64_Phdr>::value,
        "Not an Phdr");

    hdr.p_type   = ByteSwap(hdr.p_type);
    hdr.p_flags  = ByteSwap(hdr.p_flags);
    hdr.p_offset = ByteSwap(hdr.p_offset);
    hdr.p_vaddr  = ByteSwap(hdr.p_vaddr);
    hdr.p_paddr  = ByteSwap(hdr.p_paddr);
    hdr.p_filesz = ByteSwap(hdr.p_filesz);
    hdr.p_memsz  = ByteSwap(hdr.p_memsz);
    hdr.p_align  = ByteSwap(hdr.p_align);
}

template<typename T>
void SwapDHeader(T& hdr) {
    static_assert(
        std::is_same<T, Elf32_Dyn>::value ||
        std::is_same<T, Elf64_Dyn>::value,
        "Not an Phdr");

    hdr.d_tag      = ByteSwap(hdr.d_tag);
    hdr.d_un.d_val = ByteSwap(hdr.d_un.d_val);

    static_assert(sizeof(hdr.d_un.d_val) == sizeof(hdr.d_un.d_ptr),
        "Union field size mismatch.");
}

Elf64_Phdr Widen(const Elf32_Phdr& t) {
    Elf64_Phdr ret;
    ret.p_type   = t.p_type;
    ret.p_offset = t.p_offset;
    ret.p_vaddr  = t.p_vaddr;
    ret.p_paddr  = t.p_paddr;
    ret.p_filesz = t.p_filesz;
    ret.p_memsz  = t.p_memsz;
    ret.p_flags  = t.p_flags;
    ret.p_align  = t.p_align;
    return ret;
}

Elf64_Dyn Widen(const Elf32_Dyn& t) {
    Elf64_Dyn ret;
    ret.d_tag = t.d_tag;
    ret.d_un.d_val = t.d_un.d_val;
    static_assert(sizeof(t.d_un.d_val) == sizeof(t.d_un.d_ptr),
        "Union field size mismatch.");
    return ret;
}

}  // namespace

ElfError::ElfError(const std::string& what) : std::runtime_error(what) {}
ElfError::~ElfError() {}

ElfParser::ElfParser(int fd) : fd_(fd) {
    union u_t {
        Elf32_Ehdr x86;
        Elf64_Ehdr x64;
        uint8_t bytes[sizeof(x64)];
    } u;

    static_assert(sizeof(u) == sizeof(u.x64), "Additional padding found");
    static_assert(offsetof(u_t, x86) == offsetof(u_t, x64), "Invalid offsets");

    // Verify the file is ELF and determine 32/64-bit.
    ReadBytesAtOffset(fd_, 0, u.bytes, EI_CLASS + 1);

    if (memcmp(u.bytes, ELFMAG, SELFMAG) != 0) {
        throw ElfError("Not an ELF file");
    }

    const uint8_t c = u.bytes[EI_CLASS];
    if (c == ELFCLASSNONE || c >= ELFCLASSNUM) {
        throw ElfError("Unknown class");
    }
    x64_ = c == ELFCLASS64;

    // Read the rest of the header.
    ReadBytesAtOffset(fd_, EI_CLASS + 1, u.bytes + EI_CLASS + 1,
        (x64_ ? sizeof(u.x64) : sizeof(u.x86)) - (EI_CLASS + 1));

    const uint8_t byte_order = u.bytes[EI_DATA];
    if (byte_order == ELFDATANONE || byte_order >= ELFDATANUM) {
        throw ElfError("Unknown byte order");
    }
    le_ = byte_order == ELFDATA2LSB;

    if (le_ != IsLittleEndian()) {
        if (x64_) {
            SwapHeader(u.x64);
        } else {
            SwapHeader(u.x86);
        }
    }

    // Copy relevant fields.
    if (x64_) {
        phoff_      = u.x64.e_phoff;
        phentsize_  = u.x64.e_phentsize;
        phnum_      = u.x64.e_phnum;
        shoff_      = u.x64.e_shoff;
        shentsize_  = u.x64.e_shentsize;
        shnum_      = u.x64.e_shnum;
        shstrndx_   = u.x64.e_shstrndx;
    } else {
        phoff_      = u.x86.e_phoff;
        phentsize_  = u.x86.e_phentsize;
        phnum_      = u.x86.e_phnum;
        shoff_      = u.x86.e_shoff;
        shentsize_  = u.x86.e_shentsize;
        shnum_      = u.x86.e_shnum;
        shstrndx_   = u.x86.e_shstrndx;
    }
}

ElfParser::~ElfParser() {}

void ElfParser::ReadPHeader(size_t offset, Elf64_Phdr* hdr) {
    union u_t {
        Elf32_Phdr x86;
        Elf64_Phdr x64;
        uint8_t bytes[sizeof(x64)];
    } u;

    static_assert(offsetof(u_t, x64) == offsetof(u_t, x86),
        "Invalid offsets");
    static_assert(sizeof(u) == sizeof(u.x64), "Invalid padding");

    const size_t size   = x64_ ? sizeof(u.x64) : sizeof(u.x86);
    assert(size == phentsize_);

    ReadBytesAtOffset(fd_, offset, u.bytes, size);

    if (le_ != IsLittleEndian()) {
        if (x64_) {
            SwapPHeader(u.x64);
        } else {
            SwapPHeader(u.x86);
        }
    }

    if (x64_) {
        *hdr = u.x64;
    } else {
        *hdr = Widen(u.x86);
    }
}

void ElfParser::ReadDHeader(size_t offset, Elf64_Dyn* hdr) {
    union u_t {
        Elf32_Dyn x86;
        Elf64_Dyn x64;
        uint8_t bytes[sizeof(x64)];
    } u;

    static_assert(offsetof(u_t, x64) == offsetof(u_t, x86),
        "Invalid offsets");
    static_assert(sizeof(u) == sizeof(u.x64), "Invalid padding");

    const size_t size   = x64_ ? sizeof(u.x64) : sizeof(u.x86);
    ReadBytesAtOffset(fd_, offset, u.bytes, size);

    if (le_ != IsLittleEndian()) {
        if (x64_) {
            SwapDHeader(u.x64);
        } else {
            SwapDHeader(u.x86);
        }
    }

    if (x64_) {
        *hdr = u.x64;
    } else {
        *hdr = Widen(u.x86);
    }
}

bool ElfParser::GetInterpreter(std::string* interpreter) {
    for (unsigned i = 0; i < phnum_; i++) {
        const size_t offset = phoff_ + i * phentsize_;
        Elf64_Phdr hdr;
        ReadPHeader(offset, &hdr);

        if (hdr.p_type != PT_INTERP) {
            continue;
        } else if (hdr.p_filesz < 1) {
            throw ElfError("Interpeter size 0 bytes");
        }

        interpreter->resize(hdr.p_filesz);
        ReadBytesAtOffset(
            fd_, hdr.p_offset, reinterpret_cast<uint8_t*>(&(*interpreter)[0]),
            hdr.p_filesz);

        if ((*interpreter)[hdr.p_filesz - 1] != '\0') {
            throw ElfError("Interpreter not null terminated");
        }
        interpreter->resize(hdr.p_filesz - 1);
        return true;
    }

    return false;
}

std::vector<std::string> ElfParser::GetLibraryDependencies() {
    std::vector<Elf64_Phdr> loads;
    std::vector<Elf64_Xword> needed_offsets;
    bool dynsym_found = false;
    Elf64_Addr dynsym = 0;

    for (unsigned i = 0; i < phnum_; i++) {
        const size_t offset = phoff_ + i * phentsize_;
        Elf64_Phdr hdr;
        ReadPHeader(offset, &hdr);

        if (hdr.p_type == PT_LOAD) {
            loads.push_back(hdr);
            continue;
        }

        if (hdr.p_type != PT_DYNAMIC) {
            continue;
        }

        const size_t dentsize = x64_ ? sizeof(Elf64_Dyn) : sizeof(Elf32_Dyn);
        for (size_t d = 0; d + dentsize <= hdr.p_filesz; d += dentsize) {
            Elf64_Dyn dyn;
            ReadDHeader(hdr.p_offset + d, &dyn);

            switch (dyn.d_tag) {
                case DT_NEEDED:
                    needed_offsets.push_back(dyn.d_un.d_val);
                    break;
                case DT_STRTAB:
                    dynsym_found = true;
                    dynsym = dyn.d_un.d_ptr;
                    break;
                default:
                    break;
            }
        }
    }

    std::unordered_set<std::string> libs;

    // TODO:  replace this assertion as it is dependent on the contents of the
    // ELF file being correctly formed.
    assert(needed_offsets.empty() || dynsym_found);
    if (dynsym_found) {
        size_t load_idx;
        for (load_idx = 0; load_idx < loads.size(); load_idx++) {
            const auto& phdr = loads[load_idx];
            if (phdr.p_vaddr <= dynsym &&
                    dynsym <= phdr.p_vaddr + phdr.p_memsz) {
                // Found a load that covers DT_STRTAB.
                break;
            }
        }

        if (load_idx == loads.size()) {
            throw ElfError("Unable to find appropriate LOAD");
        }
        const auto& load = loads[load_idx];
        assert(dynsym >= load.p_vaddr);
        const size_t strtab_offset = dynsym - load.p_vaddr + load.p_offset;
        // This is a coarse upperbound, but we should not cross into another
        // load boundary while reading the strtab section.
        const size_t strtab_limit  = load.p_memsz - (dynsym - load.p_vaddr);

        for (const auto& needed_offset : needed_offsets) {
            size_t offset = strtab_offset + needed_offset;
            // This is dependent on the ELF file being correctly formatted.
            assert(strtab_limit >= needed_offset);
            size_t limit  = strtab_limit  - needed_offset;

            // The string is null-terminated, so we read 64 byte pieces at a
            // time until we find a null (or until we hit the limit).
            std::string sym;
            size_t nullpos = std::string::npos;
            do {
                const size_t old_size = sym.size();
                size_t to_read = std::min(limit, size_t(64));
                sym.append(to_read, '\0');

                size_t bytes_read = TryReadBytesAtOffset(
                    fd_, offset, reinterpret_cast<uint8_t*>(&sym[old_size]),
                    to_read);
                if (bytes_read == 0) {
                    nullpos = old_size;
                    break;
                }

                offset += bytes_read;
                assert(limit >= bytes_read);
                limit  -= bytes_read;

                sym.resize(old_size + bytes_read);
            } while ((nullpos = sym.find('\0')) == std::string::npos);

            if (nullpos != std::string::npos) {
                sym.resize(nullpos);
            }

            if (!sym.empty()) {
                libs.insert(sym);
            }
        }
    }

    std::vector<std::string> ret;
    ret.insert(ret.begin(), libs.begin(), libs.end());
    return ret;
}

}  // namespace file_binder
