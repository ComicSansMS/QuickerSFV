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
#ifndef INCLUDE_GUARD_QUICKER_SFV_LINE_READER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_LINE_READER_HPP

#include <quicker_sfv/file_io.hpp>

#include <optional>
#include <string>
#include <vector>

namespace quicker_sfv {

/** Helper class for processing input from text files line by line.
 * This class provides both file read buffering and line splitting facilities.
 */
class LineReader {
public:
    /** Size of the internal read buffer in bytes.
     * The class will allocate two buffers of size READ_BUFFER_SIZE to buffer data
     * from file reads. If a single line exceeds the line length of READ_BUFFER_SIZE
     * bytes, parsing the file might fail.
     */
    static constexpr size_t const READ_BUFFER_SIZE = 64 << 10;
private:
    quicker_sfv::FileInput* m_fileIn;
    size_t m_bufferOffset;
    size_t m_fileOffset;
    bool m_eof;
    struct DoubleBuffer {
        std::vector<std::byte> front;
        std::vector<std::byte> back;
    } m_buffers;
public:
    /** Constructor.
     * @param[in] file_input FileInput used for reading data from file.
     */
    explicit LineReader(quicker_sfv::FileInput& file_input) noexcept;

    /** Extracts the next line from the file.
     * Lines are separated by linebreaks. Recognized linebreaks are CRLF and LF.
     * The linebreak characters themselves will not be part of the returned string.
     * @return The next line from the file on success. An empty optional if there
     *         is no more data available in the file. In the latter case, done() will
     *         also return `true`.
     * @throw Exception Error::FileIO if an error occurs while reading from the file.
     *                  Error::ParserError if the end of the line could not be found
     *                  within the available read buffer or if the line is not a
     *                  valid UTF-8 string.
     */
    std::optional<std::u8string> readLine();

    /** Checks whether the end of file has been reached.
     * If this function returns `true`, all subsequent calls to readLine() will
     * return the empty optional.
     */
    bool done() const;

private:
    bool read_more();
};

}
#endif
