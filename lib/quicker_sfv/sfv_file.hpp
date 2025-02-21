#ifndef INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP

#include <quicker_sfv/md5_digest.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class FileInput;

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

    static std::optional<SfvFile> readFromFile(std::u8string_view filename);

    static std::optional<SfvFile> readFromFile(std::u16string_view filename);

    static std::optional<SfvFile> readFromFile(wchar_t const* filename);

    static std::optional<SfvFile> readFromFile(FileInput& file_input);

    std::span<const Entry> getEntries() const;

    void serialize(std::u8string_view out_filename) const;

    void addEntry(std::u8string_view p, MD5Digest md5);
    void addEntry(std::u16string_view p, MD5Digest md5);
    void addEntry(wchar_t const* utf16_zero_terminated, MD5Digest md5);
};

#endif
