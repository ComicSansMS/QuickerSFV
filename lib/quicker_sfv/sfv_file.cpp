#include <quicker_sfv/sfv_file.hpp>

#include <quicker_sfv/crc32.hpp>
#include <quicker_sfv/error.hpp>
#include <quicker_sfv/line_reader.hpp>

namespace quicker_sfv {

ChecksumFilePtr createSfvFile() {
    return ChecksumFilePtr(new SfvFile(), detail::ChecksumFilePtrDeleter{ [](ChecksumFile* p) { delete p; } });
}

SfvFile::SfvFile()
{}


SfvFile::~SfvFile() = default;

[[nodiscard]] std::u8string_view SfvFile::fileExtensions() const {
    return u8"*.sfv";
}

[[nodiscard]] std::u8string_view SfvFile::fileDescription() const {
    return u8"Sfv File";
}

[[nodiscard]] HasherPtr SfvFile::createHasher() const {
    return HasherPtr(new Crc32Hasher(), detail::HasherPtrDeleter{ [](Hasher* p) { delete p; } });
}

[[nodiscard]] Digest SfvFile::digestFromString(std::u8string_view str) const {
    return Crc32Hasher::digestFromString(str);
}

void SfvFile::readFromFile(FileInput& file_input) {
    LineReader reader(file_input);
    std::vector<Entry> new_entries;
    for (;;) {
        auto opt_line = reader.read_line();
        if (!opt_line) {
            if (reader.done()) {
                break;
            }
        }
        auto line = std::u8string_view{ *opt_line };
        if (line.empty()) { continue; }
        // skip comments
        if (line.starts_with(u8";")) { continue; }

        if (line.size() < 10) { throwException(Error::ParserError); }
        std::size_t const separator_idx = line.size() - 8;
        if ((line[separator_idx - 1] != u8' ')) { throwException(Error::ParserError); }
        std::u8string_view filepath_sv = line.substr(0, separator_idx - 1);
        new_entries.emplace_back(
            std::u8string(filepath_sv),
            Crc32Hasher::digestFromString(line.substr(separator_idx))
        );
    }
    m_entries = std::move(new_entries);
}

[[nodiscard]] std::span<const ChecksumFile::Entry> SfvFile::getEntries() const {
    return m_entries;
}

void SfvFile::serialize(FileOutput& file_output) const {
    for (auto const& e : m_entries) {
        std::u8string out_str;
        out_str.reserve(e.path.size() + 11);
        out_str.append(e.path);
        out_str.push_back(u8' ');
        out_str.append(e.digest.toString());
        out_str.push_back(u8'\n');
        file_output.write(std::span<std::byte const>(reinterpret_cast<std::byte const*>(out_str.data()), out_str.size()));
    }
}

void SfvFile::addEntry(std::u8string_view path, Digest digest) {
    if (!Crc32Hasher::checkType(digest)) { throwException(Error::InvalidArgument); }
    m_entries.emplace_back(std::u8string{ path }, digest);
}

void SfvFile::clear() {
    m_entries.clear();
}

}
