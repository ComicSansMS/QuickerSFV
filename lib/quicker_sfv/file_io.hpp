#ifndef INCLUDE_GUARD_QUICKER_SFV_FILE_IO_HPP
#define INCLUDE_GUARD_QUICKER_SFV_FILE_IO_HPP

#include <cstddef>
#include <limits>
#include <span>

namespace quicker_sfv {

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
};

}

#endif
