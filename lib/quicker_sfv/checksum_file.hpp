#ifndef INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_FILE_HPP

#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/hasher.hpp>

#include <memory>
#include <string_view>

namespace quicker_sfv {

class ChecksumFile;

namespace detail {
struct HasherPtrDeleter {
    void (*deleter)(Hasher* p);
    void operator()(Hasher* p) const;
};
struct ChecksumFilePtrDeleter {
    void (*deleter)(ChecksumFile* p);
    void operator()(ChecksumFile* p) const;
};
}

using HasherPtr = std::unique_ptr<Hasher, detail::HasherPtrDeleter>;

using ChecksumFilePtr = std::unique_ptr<ChecksumFile, detail::ChecksumFilePtrDeleter>;

using ChecksumFileCreate = ChecksumFilePtr const& (*)();

class ChecksumFile {
public:
    struct Entry {
        std::u8string path;
        Digest digest;
    };
public:
    virtual ~ChecksumFile() = 0;
    virtual [[nodiscard]] std::u8string_view fileExtensions() const = 0;
    virtual [[nodiscard]] std::u8string_view fileDescription() const = 0;
    virtual [[nodiscard]] HasherPtr createHasher() const = 0;
    virtual [[nodiscard]] Digest digestFromString(std::u8string_view str) const = 0;

    virtual void readFromFile(FileInput& file_input) = 0;

    virtual [[nodiscard]] std::span<const Entry> getEntries() const = 0;

    virtual void serialize(FileOutput& file_output) const = 0;

    virtual void addEntry(std::u8string_view path, Digest digest) = 0;
    virtual void clear() = 0;
};

}

#endif
