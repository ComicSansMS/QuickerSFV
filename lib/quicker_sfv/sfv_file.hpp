#ifndef INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP

#include <quicker_sfv/checksum_file.hpp>
#include <quicker_sfv/digest.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace quicker_sfv {

class SfvFile: public ChecksumFile {
public:
    friend ChecksumFilePtr createSfvFile();
private:
    std::vector<Entry> m_entries;
private:
    SfvFile();
public:
    ~SfvFile() override;
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

ChecksumFilePtr createSfvFile();

}
#endif
