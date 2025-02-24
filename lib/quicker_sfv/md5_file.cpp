#include <quicker_sfv/md5_file.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/line_reader.hpp>
#include <quicker_sfv/md5.hpp>

#include <memory>

namespace quicker_sfv {

ChecksumFilePtr createMD5File() {
    return ChecksumFilePtr(new MD5File, detail::ChecksumFilePtrDeleter{ [](ChecksumFile* p) { delete p; } });
}

MD5File::MD5File()
{ }

MD5File::~MD5File()
{ }

[[nodiscard]] std::u8string_view MD5File::fileExtensions() const {
    return u8"*.md5";
}

[[nodiscard]] std::u8string_view MD5File::fileDescription() const {
    return u8"MD5";
}

[[nodiscard]] HasherPtr MD5File::createHasher() const {
    return HasherPtr(new MD5Hasher, detail::HasherPtrDeleter{ [](Hasher* p) { delete p; } });
}

[[nodiscard]] Digest MD5File::digestFromString(std::u8string_view str) const {
    return MD5Hasher::digestFromString(str);
}

void MD5File::readFromFile(FileInput& file_input) {
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
        std::size_t const separator_idx = line.find(u8'*');
        if (separator_idx == std::string::npos) { throwException(Error::ParserError); }
        if ((separator_idx == 0) || (line[separator_idx - 1] != u8' ')) { throwException(Error::ParserError); }
        std::u8string_view filepath_sv = line.substr(separator_idx + 1);
        new_entries.emplace_back(
            std::u8string(filepath_sv),
            MD5Hasher::digestFromString(line.substr(0, separator_idx - 1))
        );
    }
    m_entries = std::move(new_entries);
}

[[nodiscard]] std::span<const ChecksumFile::Entry> MD5File::getEntries() const {
    return m_entries;
}

void MD5File::serialize(FileOutput& file_output) const {
    for (auto const& e : m_entries) {
        std::u8string out_str;
        out_str.reserve(e.path.size() + 36);
        out_str.append(e.digest.toString());
        out_str.append(u8" *");
        out_str.append(e.path);
        out_str.push_back(u8'\n');
        file_output.write(std::span<std::byte const>(reinterpret_cast<std::byte const*>(out_str.data()), out_str.size()));
    }
}

void MD5File::addEntry(std::u8string_view p, Digest digest) {
    if (!MD5Hasher::checkType(digest)) { throwException(Error::InvalidArgument); }
    m_entries.emplace_back(std::u8string{ p }, std::move(digest));
}

void MD5File::clear() {
    m_entries.clear();
}

}
