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
#ifndef INCLUDE_GUARD_QUICKER_SFV_FILE_IO_HPP
#define INCLUDE_GUARD_QUICKER_SFV_FILE_IO_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace quicker_sfv {

/** Interface for file output operations.
 * This will be provided by the client as the sole facility for write access to the
 * filesystem.
 */
class FileOutput {
public:
    virtual ~FileOutput() = 0;
    /** Writes the supplied span of bytes to a file.
     * @return On error, returns 0.
               If successful, returns the number of bytes written to the file,
               which has to be equal to the size of the input span.
     */
    virtual size_t write(std::span<std::byte const> bytes_to_write) = 0;
};

/** Interface for file input operations.
 * This will be provided by the client as the sole facility for read access to the
 * filesystem.
 */
class FileInput {
public:
    static constexpr size_t const RESULT_END_OF_FILE = std::numeric_limits<size_t>::max();
    virtual ~FileInput() = 0;
    /** Reads to the supplied span a number of bytes from the file equal to the size of the span.
     * @return On error, returns 0.
     *         If there is no additional input available because the end of file
     *         has been reached, returns RESULT_END_OF_FILE.
     *         If successful, returns the number of bytes read from the file.
     *         If this is less than the size of the input span, the end of file
     *         has been reached and subsequent calls to read will return
     *         with RESULT_END_OF_FILE.
     */
    virtual size_t read(std::span<std::byte> read_buffer) = 0;

    /** Possible start positions for a seek() operation.
     */
    enum class SeekStart {
        CurrentPosition,    ///< The current value of the file read pointer.
        FileStart,          ///< The start of the file.
        FileEnd,            ///< The end of the file.
    };

    /** Sets the value of the file read pointer.
     * The file pointer
     * @param offset The offset of the new file pointer position from seek_start.
     * @param seek_start Start position for the file pointer calculation.
     * @return On success, returns the new value of the file read pointer.
     *         On error, returns -1.
     */
    virtual int64_t seek(int64_t offset, SeekStart seek_start) = 0;

    /** Retrieves the current value of the file read pointer.
     * @return On success, the current position of the file pointer in bytes.
     *         On error, returns -1.
     */
    virtual int64_t tell() = 0;
};

}

#endif
