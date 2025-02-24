#ifndef INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_FILE_HPP

#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/hasher.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace quicker_sfv {

class ChecksumFile {
public:
    struct Entry {
        std::u8string path;
        Digest digest;
    };
    std::vector<Entry> m_entries;
public:
    [[nodiscard]] std::span<const Entry> getEntries() const;
    void addEntry(std::u8string_view path, Digest digest);
    void sortEntries();
    void clear();
};

}

#endif
