/*
 *   QuickerSFV - A fast checksum verifier
 *   Copyright (C) 2025  Andreas Weis (quickersfv@andreas-weis.net)
 *
 *   This file is part of QuickerSFV.
 *
 *   QuickerSFV is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   QuickerSFV is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <quicker_sfv/line_reader.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/string_utilities.hpp>

#include <algorithm>
#include <cassert>

namespace quicker_sfv {

LineReader::LineReader(quicker_sfv::FileInput& file_input) noexcept
    :m_fileIn(&file_input), m_bufferOffset(0), m_fileOffset(0), m_eof(false),
    m_buffers{ .front = std::vector<std::byte>(READ_BUFFER_SIZE), .back = std::vector<std::byte>(READ_BUFFER_SIZE) }
{
}

bool LineReader::read_more() {
    assert(!m_eof && (m_bufferOffset >= READ_BUFFER_SIZE));
    m_bufferOffset -= READ_BUFFER_SIZE;
    std::swap(m_buffers.front, m_buffers.back);
    size_t const bytes_read = m_fileIn->read(m_buffers.back);
    if (bytes_read == quicker_sfv::FileInput::RESULT_END_OF_FILE) {
        m_eof = true;
        m_buffers.back.resize(0);
        return true;
    }
    m_fileOffset += bytes_read;
    if (bytes_read < READ_BUFFER_SIZE) {
        m_buffers.back.resize(bytes_read);
        m_eof = true;
    }
    return true;
}

// return conditions: file i/o error, eof, invalid file, invalid utf8, line, empty line
std::optional<std::u8string> LineReader::readLine() {
    if (done()) { return std::nullopt; }
    if (m_fileOffset == 0) {
        // initial read
        m_bufferOffset += READ_BUFFER_SIZE;
        read_more();
        if (!m_eof) {
            m_bufferOffset += READ_BUFFER_SIZE;
            read_more();
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
        if ((it == end(m_buffers.back)) && (!m_eof)) {
            // no newline in either buffer; assume invalid file
            throwException(Error::ParserError);
        }
        std::span<std::byte> front_range(it_begin, end(m_buffers.front));
        std::span<std::byte> back_range(begin(m_buffers.back), it);
        std::vector<std::byte> buffer;
        buffer.reserve(front_range.size() + back_range.size());
        buffer.insert(end(buffer), begin(front_range), end(front_range));
        buffer.insert(end(buffer), begin(back_range), end(back_range));
        m_bufferOffset += buffer.size() + 1;
        if (!m_eof) {
            read_more();
        } else if (!m_buffers.back.empty()) {
            std::swap(m_buffers.front, m_buffers.back);
            m_buffers.back.clear();
            m_bufferOffset -= READ_BUFFER_SIZE;
        }
        if (!buffer.empty() && (buffer.back() == carriage_return)) {
            buffer.pop_back();
        }
        if (!quicker_sfv::checkValidUtf8(buffer)) {
            throwException(Error::ParserError);
        }
        return std::u8string(reinterpret_cast<char8_t const*>(buffer.data()), reinterpret_cast<char8_t const*>(buffer.data() + buffer.size()));
    } else {
        // line is fully contained within front buffer
        m_bufferOffset += std::distance(it_begin, it) + 1;
        std::span<std::byte> line_range{ it_begin, it };
        if (!line_range.empty() && (line_range.back() == carriage_return)) { line_range = line_range.subspan(0, line_range.size() - 1); }
        if (!quicker_sfv::checkValidUtf8(line_range)) {
            throwException(Error::ParserError);
        }
        return std::u8string(reinterpret_cast<char8_t const*>(line_range.data()),
            reinterpret_cast<char8_t const*>(line_range.data() + line_range.size()));
    }
}

bool LineReader::done() const {
    return m_eof && m_buffers.back.empty() && (m_bufferOffset == m_buffers.front.size() + 1);
}

}
