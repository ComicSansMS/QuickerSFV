#include <quicker_sfv/md5_provider.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/line_reader.hpp>
#include <quicker_sfv/string_utilities.hpp>

#include <quicker_sfv/detail/md5.hpp>

#include <memory>

namespace quicker_sfv {

ChecksumProviderPtr createMD5Provider() {
    return ChecksumProviderPtr(new MD5Provider);
}

MD5Provider::MD5Provider() = default;
MD5Provider::~MD5Provider() = default;

ProviderCapabilities MD5Provider::getCapabilities() const noexcept {
    return ProviderCapabilities::Full;
}

std::u8string_view MD5Provider::fileExtensions() const noexcept {
    return u8"*.md5";
}

std::u8string_view MD5Provider::fileDescription() const noexcept {
    return u8"MD5";
}

HasherPtr MD5Provider::createHasher(HasherOptions const&) const {
    return std::make_unique<detail::MD5Hasher>();
}

Digest MD5Provider::digestFromString(std::u8string_view str) const {
    return detail::MD5Hasher::digestFromString(str);
}

ChecksumFile MD5Provider::readFromFile(FileInput& file_input) const {
    LineReader reader(file_input);
    ChecksumFile ret;
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
        if (separator_idx == std::string::npos) {
            throwException(Error::ParserError);
        }
        if ((separator_idx == 0) || (line[separator_idx - 1] != u8' ')) {
            throwException(Error::ParserError);
        }
        std::u8string_view filepath_sv = trim(line.substr(separator_idx + 1));
        if (filepath_sv.contains('*')) {
            throwException(Error::ParserError);
        }
        std::u8string_view digest_sv = trim(line.substr(0, separator_idx - 1));
        ret.addEntry(filepath_sv, detail::MD5Hasher::digestFromString(digest_sv));
    }
    return ret;
}

void MD5Provider::writeNewFile(FileOutput& file_output, ChecksumFile const& f) const {
    for (auto const& e : f.getEntries()) {
        std::u8string out_str;
        out_str.reserve(e.path.size() + 36);
        out_str.append(e.digest.toString());
        out_str.append(u8" *");
        out_str.append(e.path);
        out_str.push_back(u8'\n');
        size_t ret = file_output.write(
            std::span<std::byte const>(reinterpret_cast<std::byte const*>(out_str.data()), out_str.size()));
        if (ret == 0) {
            throwException(Error::FileIO);
        }
    }
}

}
