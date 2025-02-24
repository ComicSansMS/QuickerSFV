#ifndef INCLUDE_GUARD_QUICKER_SFV_LINE_READER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_LINE_READER_HPP

#include <quicker_sfv/file_io.hpp>

#include <optional>
#include <string>
#include <vector>

namespace quicker_sfv {

class LineReader {
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
    explicit LineReader(quicker_sfv::FileInput& file_input) noexcept;

    std::optional<std::u8string> read_line();

    bool done() const;

private:
    bool read_more();
};

}
#endif
