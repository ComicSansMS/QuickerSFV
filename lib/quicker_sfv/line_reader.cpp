#include <quicker_sfv/line_reader.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/utf.hpp>

namespace quicker_sfv {

inline constexpr size_t const READ_BUFFER_SIZE = 64 << 10;

LineReader::LineReader(quicker_sfv::FileInput& file_input) noexcept
    :m_fileIn(&file_input), m_bufferOffset(0), m_fileOffset(0), m_eof(false),
    m_buffers{ .front = std::vector<std::byte>(READ_BUFFER_SIZE), .back = std::vector<std::byte>(READ_BUFFER_SIZE) }
{
}

bool LineReader::read_more() {
    if (m_bufferOffset < READ_BUFFER_SIZE) { return false; }
    if (m_eof) { return false; }
    m_bufferOffset -= READ_BUFFER_SIZE;
    std::swap(m_buffers.front, m_buffers.back);
    size_t const bytes_read = m_fileIn->read(m_buffers.back);
    if (bytes_read == quicker_sfv::FileInput::RESULT_END_OF_FILE) {
        m_eof = true;
        m_buffers.back.resize(0);
        return true;
    }
    m_fileOffset += bytes_read;
    if (bytes_read == 0) { throwException(Error::FileIO); }
    if (bytes_read < READ_BUFFER_SIZE) {
        m_buffers.back.resize(bytes_read);
        m_eof = true;
    }
    return true;
}

// return conditions: file i/o error, eof, invalid file, invalid utf8, line, empty line
std::optional<std::u8string> LineReader::read_line() {
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
        if (!quicker_sfv::checkValidUtf8(buffer)) {
            throwException(Error::ParserError);
        }
        m_bufferOffset += buffer.size() + 1;
        if (!m_eof) {
            if (!read_more()) {
                throwException(Error::FileIO);
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
        if (!quicker_sfv::checkValidUtf8(line_range)) {
            return std::nullopt;
        }
        if (!line_range.empty() && (line_range.back() == carriage_return)) { line_range = line_range.subspan(0, line_range.size() - 1); }
        return std::u8string(reinterpret_cast<char8_t const*>(line_range.data()),
            reinterpret_cast<char8_t const*>(line_range.data() + line_range.size()));
    }
}

bool LineReader::done() const {
    return m_eof && m_buffers.back.empty() && (m_bufferOffset == m_buffers.front.size() + 1);
}

}
