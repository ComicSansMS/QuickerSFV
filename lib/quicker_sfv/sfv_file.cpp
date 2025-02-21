#include <quicker_sfv/sfv_file.hpp>

#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/utf.hpp>

#include <algorithm>
#include <fstream>
#include <optional>
#include <vector>

namespace {
inline constexpr size_t const READ_BUFFER_SIZE = 4 << 10;

class FileReader {
private:
    FileInput* m_fileIn;
    std::vector<std::byte> m_buffer;
    size_t m_bufferOffset;
    size_t m_fileOffset;
    bool m_eof;
public:
    explicit FileReader(FileInput& file_input) noexcept;

    std::optional<std::u8string> read_line();

private:
    bool read_more();
};

FileReader::FileReader(FileInput& file_input) noexcept
    :m_fileIn(&file_input), m_buffer(READ_BUFFER_SIZE), m_bufferOffset(0), m_fileOffset(0), m_eof(false)
{ }

bool FileReader::read_more() {
    std::rotate(begin(m_buffer), begin(m_buffer) + m_bufferOffset, end(m_buffer));
    size_t const bytes_read = m_fileIn->read(std::span{ begin(m_buffer) + m_bufferOffset, end(m_buffer) });


}

// return conditions: file i/o error, eof, invalid file, invalid utf8, line, empty line
std::optional<std::u8string> FileReader::read_line() {
    if (m_fileOffset == m_bufferOffset) {
        if (m_eof) { return std::nullopt; }
        size_t const bytes_read = m_fileIn->read(m_buffer);
        m_fileOffset += bytes_read;
        if (bytes_read != m_buffer.size()) { m_eof = true; }
    }
    constexpr std::byte const newline = static_cast<std::byte>('\n');
    auto it_begin = begin(m_buffer) + m_bufferOffset;
    auto it = std::find(begin(m_buffer), end(m_buffer), newline);
    if (it == end(m_buffer)) {

    }
    m_bufferOffset += std::distance(it_begin, it);
    std::span<std::byte> line_range{ it_begin, it };
    if (!checkValidUtf8(line_range)) {
        return std::nullopt;
    }
    return std::u8string( reinterpret_cast<char8_t const*>(line_range.data()),
                          reinterpret_cast<char8_t const*>(line_range.data() + line_range.size()) );
}
}

struct Ex {};

SfvFile::SfvFile(std::u8string_view filename) {
    std::ifstream fin(reinterpret_cast<char const*>(std::u8string{ filename }.c_str()));
    std::string l;
    while (std::getline(fin, l)) {
        std::string_view line = l;
        // skip comments
        if (line.starts_with(";")) { continue; }
        std::size_t const separator_idx = line.find('*');
        if (separator_idx == std::string::npos) { throw Ex{}; /* @todo error */ }
        std::string_view filepath_sv = line.substr(separator_idx + 1);
        if (!checkValidUtf8(filepath_sv)) {
            // @todo: file parse error
        }
        m_files.emplace_back(
            std::u8string{ assumeUtf8(filepath_sv) },
            MD5Digest::fromString(line.substr(0, separator_idx))
        );
    }
}

SfvFile::SfvFile(std::u16string_view filename)
    :SfvFile(convertToUtf8(filename))
{}

SfvFile::SfvFile(wchar_t const* filename)
    :SfvFile(reinterpret_cast<char16_t const*>(filename))
{}

std::span<const SfvFile::Entry> SfvFile::getEntries() const {
    return m_files;
}

void SfvFile::serialize(std::u8string_view out_filename) const {
    std::ofstream fout(reinterpret_cast<char const*>(std::u8string{ out_filename }.c_str()));
    for (auto const& f : m_files) {
        fout << std::format("{} *..\\{}\n", f.md5, reinterpret_cast<char const*>(f.path.c_str()));
    }
}

void SfvFile::addEntry(std::u8string_view p, MD5Digest md5) {
    m_files.emplace_back(std::u8string{ p }, md5);
}

void SfvFile::addEntry(std::u16string_view p, MD5Digest md5) {
    addEntry(convertToUtf8(p), md5);
}

void SfvFile::addEntry(wchar_t const* utf16_zero_terminated, MD5Digest md5) {
    addEntry(std::u16string_view(reinterpret_cast<char16_t const*>(utf16_zero_terminated)), md5);
}

