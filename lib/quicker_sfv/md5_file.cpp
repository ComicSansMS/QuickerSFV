#include <quicker_sfv/md5_file.hpp>

#include <memory>

namespace quicker_sfv {

ChecksumFilePtr createMD5File() {
    return ChecksumFilePtr(new MD5File, detail::ChecksumFilePtrDeleter{ [](ChecksumFile* p) { delete p; } });
}

/*
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
    */
}
