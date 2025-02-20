#ifndef INCLUDE_GUARD_QUICKER_SFV_FILE_IO_HPP
#define INCLUDE_GUARD_QUICKER_SFV_FILE_IO_HPP

#include <cstddef>
#include <span>

class FileOutput {
public:
    virtual ~FileOutput() = 0;
    virtual size_t write(std::span<std::byte const> bytes_to_write) = 0;
};

class FileInput {
public:
    virtual ~FileInput() = 0;
    virtual size_t read(std::span<std::byte> read_buffer) = 0;
};

#endif
