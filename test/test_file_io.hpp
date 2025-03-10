#ifndef INCLUDE_GUARD_QUICKER_SFV_TESTING_TEST_FILE_IO_HPP
#define INCLUDE_GUARD_QUICKER_SFV_TESTING_TEST_FILE_IO_HPP

#include <quicker_sfv/file_io.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

struct TestInput : public quicker_sfv::FileInput {
    std::vector<char> contents;
    size_t read_idx = 0;
    size_t fault_after = 0;
    size_t read_calls = 0;

    TestInput& operator=(std::string_view t) {
        contents.assign(t.begin(), t.end());
        return *this;
    }

    size_t read(std::span<std::byte> read_buffer) override {
        ++read_calls;
        size_t const bytes_available = contents.size() - read_idx;
        if (bytes_available == 0) { return RESULT_END_OF_FILE; }
        size_t const bytes_to_read = std::min(bytes_available, read_buffer.size());
        if ((fault_after > 0) && (read_idx + bytes_to_read >= fault_after)) {
            return 0;
        }
        std::memcpy(read_buffer.data(), contents.data() + read_idx, bytes_to_read);
        read_idx += bytes_to_read;
        return bytes_to_read;
    }

    int64_t seek(int64_t offset, SeekStart seek_start) override {
        int64_t base_index = 0;
        if (seek_start == SeekStart::CurrentPosition) {
            base_index = read_idx;
        } else if (seek_start == SeekStart::FileEnd) {
            base_index = contents.size();
        }
        int64_t const new_index = base_index + offset;
        if ((new_index < 0) || (new_index > static_cast<int64_t>(contents.size()))) {
            return -1;
        }
        read_idx = new_index;
        return new_index;
    }

    int64_t tell() override {
        return read_idx;
    }
};

struct TestOutput : public quicker_sfv::FileOutput {
    std::vector<char> contents;
    size_t write_idx = 0;
    size_t fault_after = 0;
    size_t write_calls = 0;

    size_t write(std::span<std::byte const> bytes_to_write) override {
        ++write_calls;
        if ((fault_after > 0) && (write_idx + bytes_to_write.size() >= fault_after)) {
            return 0;
        }
        contents.resize(contents.size() + bytes_to_write.size());
        std::memcpy(contents.data() + contents.size() - bytes_to_write.size(), bytes_to_write.data(), bytes_to_write.size());
        write_idx += bytes_to_write.size();
        return bytes_to_write.size();
    }
};

#endif
