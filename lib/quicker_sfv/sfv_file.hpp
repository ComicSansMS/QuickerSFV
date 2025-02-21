#ifndef INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP

#include <quicker_sfv/md5_digest.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

class SfvFile {
public:
    struct Entry {
        std::u8string path;
        MD5Digest md5;
    };
private:
    std::vector<Entry> m_files;
public:
    SfvFile() = default;

    explicit SfvFile(std::u8string_view filename);

    explicit SfvFile(std::u16string_view filename);

    explicit SfvFile(wchar_t const* filename);

    std::span<const Entry> getEntries() const;

    void serialize(std::u8string_view out_filename) const;

    void addEntry(std::u8string_view p, MD5Digest md5);
    void addEntry(std::u16string_view p, MD5Digest md5);
    void addEntry(wchar_t const* utf16_zero_terminated, MD5Digest md5);
};

#endif
