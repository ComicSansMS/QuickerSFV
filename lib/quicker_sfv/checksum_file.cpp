#include <quicker_sfv/checksum_file.hpp>

#include <algorithm>

namespace quicker_sfv {

[[nodiscard]] std::span<const ChecksumFile::Entry> ChecksumFile::getEntries() const {
    return m_entries;
}

void ChecksumFile::addEntry(std::u8string_view path, Digest digest) {
    m_entries.emplace_back(std::u8string(path), std::move(digest));
}

void ChecksumFile::sortEntries() {
    std::sort(begin(m_entries), end(m_entries),
        [](Entry const& lhs, Entry const& rhs) -> bool {
            return lhs.path < rhs.path;
        });
}

void ChecksumFile::clear() {
    m_entries.clear();
}


}
