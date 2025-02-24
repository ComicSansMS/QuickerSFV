#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_FILE_HPP

#include <quicker_sfv/checksum_file.hpp>

namespace quicker_sfv {

class MD5File : public ChecksumFile {
public:
    friend ChecksumFilePtr createMD5File();
private:
    MD5File();
public:
    ~MD5File() override;
    [[nodiscard]] std::u8string_view fileExtension() const override;
    [[nodiscard]] std::u8string_view fileDescription() const override;
    [[nodiscard]] HasherPtr createHasher() const override;
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override;
    void readFromFile(FileInput& file_input) override;
    [[nodiscard]] std::span<const Entry> getEntries() const override;
    void serialize(FileOutput& file_output) const override;
    void addEntry(std::u8string_view p, Digest md5) override;
};

ChecksumFilePtr createMD5File();

}

#endif
