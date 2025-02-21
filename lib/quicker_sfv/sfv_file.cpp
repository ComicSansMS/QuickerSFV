#include <quicker_sfv/sfv_file.hpp>

#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/utf.hpp>

#include <algorithm>
#include <optional>
#include <vector>

namespace {
inline constexpr size_t const READ_BUFFER_SIZE = 64 << 10;

class FileReader {
private:
    FileInput* m_fileIn;
    size_t m_bufferOffset;
    size_t m_fileOffset;
    bool m_eof;
    struct DoubleBuffer {
        std::vector<std::byte> front;
        std::vector<std::byte> back;
    } m_buffers;
public:
    explicit FileReader(FileInput& file_input) noexcept;

    std::optional<std::u8string> read_line();

    bool done() const;

private:
    bool read_more();
};

FileReader::FileReader(FileInput& file_input) noexcept
    :m_fileIn(&file_input), m_bufferOffset(0), m_fileOffset(0), m_eof(false),
     m_buffers{ .front = std::vector<std::byte>(READ_BUFFER_SIZE), .back = std::vector<std::byte>(READ_BUFFER_SIZE) }
{ }

bool FileReader::read_more() {
    if (m_bufferOffset < READ_BUFFER_SIZE) { return false; }
    if (m_eof) { return false; }
    m_bufferOffset -= READ_BUFFER_SIZE;
    std::swap(m_buffers.front, m_buffers.back);
    size_t const bytes_read = m_fileIn->read(m_buffers.back);
    if (bytes_read == FileInput::RESULT_END_OF_FILE) {
        m_eof = true;
        m_buffers.back.resize(0);
        return true;
    }
    m_fileOffset += bytes_read;
    if (bytes_read == 0) { return false; }
    if (bytes_read < READ_BUFFER_SIZE) {
        m_buffers.back.resize(bytes_read);
        m_eof = true;
    }
    return true;
}

// return conditions: file i/o error, eof, invalid file, invalid utf8, line, empty line
std::optional<std::u8string> FileReader::read_line() {
    if (done()) { return std::nullopt; }
    if (m_fileOffset == 0) {
        // initial read
        m_bufferOffset += READ_BUFFER_SIZE;
        if (!read_more()) { return std::nullopt; }
        if (!m_eof) {
            m_bufferOffset += READ_BUFFER_SIZE;
            if (!read_more()) { return std::nullopt; }
        } else {
            std::swap(m_buffers.front, m_buffers.back);
            m_buffers.back.clear();
        }
    }
    constexpr std::byte const newline = static_cast<std::byte>('\n');
    constexpr std::byte const carriage_return = static_cast<std::byte>('\r');
    auto it_begin = begin(m_buffers.front) + m_bufferOffset;
    auto it = std::find(it_begin, end(m_buffers.front), newline);
    if (it == end(m_buffers.front)) {
        // read spans buffers
        it = std::find(begin(m_buffers.back), end(m_buffers.back), newline);
        if ((it == end(m_buffers.back)) && (!m_eof)) { return std::nullopt; }
        std::span<std::byte> front_range(it_begin, end(m_buffers.front));
        std::span<std::byte> back_range(begin(m_buffers.back), it);
        std::vector<std::byte> buffer;
        buffer.append_range(front_range);
        buffer.append_range(back_range);
        if (!checkValidUtf8(buffer)) {
            return std::nullopt;
        }
        m_bufferOffset += buffer.size() + 1;
        if (!m_eof) {
            if (!read_more()) {
                return std::nullopt;
            }
        } else if (!m_buffers.back.empty()) {
            std::swap(m_buffers.front, m_buffers.back);
            m_buffers.back.clear();
            m_bufferOffset -= READ_BUFFER_SIZE;
        }
        if (!buffer.empty() && (buffer.back() == carriage_return)) { buffer.pop_back(); }
        return std::u8string(reinterpret_cast<char8_t const*>(buffer.data()), reinterpret_cast<char8_t const*>(buffer.data() + buffer.size()));
    } else {
        // line is fully contained within front buffer
        m_bufferOffset += std::distance(it_begin, it) + 1;
        std::span<std::byte> line_range{ it_begin, it };
        if (!checkValidUtf8(line_range)) {
            return std::nullopt;
        }
        if (!line_range.empty() && (line_range.back() == carriage_return)) { line_range = line_range.subspan(0, line_range.size() - 1); }
        return std::u8string(reinterpret_cast<char8_t const*>(line_range.data()),
                             reinterpret_cast<char8_t const*>(line_range.data() + line_range.size()));
    }
}

bool FileReader::done() const {
    return m_eof && m_buffers.back.empty() && (m_bufferOffset == m_buffers.front.size() + 1);
}
}


std::optional<SfvFile> SfvFile::readFromFile(FileInput& file_input) {
    SfvFile ret;
    FileReader reader(file_input);
    for (;;) {
        auto opt_line = reader.read_line();
        if (!opt_line) {
            if (reader.done()) {
                break;
            } else {
                return std::nullopt;
            }
        }
        auto line = std::u8string_view{ *opt_line };
        if (line.empty()) { continue; }
        // skip comments
        if (line.starts_with(u8";")) { continue; }
        std::size_t const separator_idx = line.find(u8'*');
        if (separator_idx == std::string::npos) { return std::nullopt; }
        std::u8string_view filepath_sv = line.substr(separator_idx + 1);
        ret.m_files.emplace_back(
            std::u8string(filepath_sv),
            MD5Digest::fromString(line.substr(0, separator_idx))
        );
    }
    return ret;
}

std::span<const SfvFile::Entry> SfvFile::getEntries() const {
    return m_files;
}

void SfvFile::serialize(FileOutput& file_output) const {
    for (auto const& f : m_files) {
        //auto out_str = std::format("{} *..\\{}\n", f.md5, reinterpret_cast<char const*>(f.path.c_str()));
        auto out_str = f.md5.toString();
        out_str.append(u8" *..\\");
        out_str.append(f.path);
        out_str.push_back(u8'\n');
        file_output.write(std::span<std::byte const>(reinterpret_cast<std::byte const*>(out_str.data()), out_str.size()));
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

