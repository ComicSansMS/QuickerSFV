#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_FILE_HPP

#include <quicker_sfv/checksum_file.hpp>

#include <vector>

namespace quicker_sfv {

class MD5File : public ChecksumFile {
public:
    friend ChecksumFilePtr createMD5File();
private:
    std::vector<ChecksumFile::Entry> m_entries;
private:
    MD5File();
public:
    ~MD5File() override;
    [[nodiscard]] std::u8string_view fileExtensions() const override;
    [[nodiscard]] std::u8string_view fileDescription() const override;
    [[nodiscard]] HasherPtr createHasher() const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;
    void readFromFile(FileInput& file_input) override;
    [[nodiscard]] std::span<const Entry> getEntries() const override;
    void serialize(FileOutput& file_output) const override;
    void addEntry(std::u8string_view path, Digest digest) override;
    void clear() override;
};

ChecksumFilePtr createMD5File();

}

#endif
