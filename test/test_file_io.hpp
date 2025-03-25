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
#ifndef INCLUDE_GUARD_QUICKER_SFV_TESTING_TEST_FILE_IO_HPP
#define INCLUDE_GUARD_QUICKER_SFV_TESTING_TEST_FILE_IO_HPP

#include <quicker_sfv/error.hpp>
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
    std::u8string file_name = u8"testfile.bin";

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
            quicker_sfv::throwException(quicker_sfv::Error::FileIO);
        }
        std::memcpy(read_buffer.data(), contents.data() + read_idx, bytes_to_read);
        read_idx += bytes_to_read;
        return bytes_to_read;
    }

    int64_t seek(int64_t offset, SeekStart seek_start) override {
        size_t base_index = 0;
        if (seek_start == SeekStart::CurrentPosition) {
            base_index = read_idx;
        } else if (seek_start == SeekStart::FileEnd) {
            base_index = contents.size();
        }
        int64_t const new_index = base_index + offset;
        if ((new_index < 0) || (new_index > static_cast<int64_t>(contents.size()))) {
            quicker_sfv::throwException(quicker_sfv::Error::FileIO);
        }
        read_idx = static_cast<size_t>(new_index);
        return new_index;
    }

    int64_t tell() override {
        return read_idx;
    }

    std::u8string_view current_file() const override {
        return file_name;
    }

    bool open(std::u8string_view new_file) override {
        file_name = new_file;
        read_idx = 0;
        return true;
    }

    uint64_t file_size() override {
        return contents.size();
    }
};

struct TestOutput : public quicker_sfv::FileOutput {
    std::vector<char> contents;
    size_t write_idx = 0;
    size_t fault_after = 0;
    size_t write_calls = 0;

    void write(std::span<std::byte const> bytes_to_write) override {
        ++write_calls;
        if ((fault_after > 0) && (write_idx + bytes_to_write.size() >= fault_after)) {
            quicker_sfv::throwException(quicker_sfv::Error::FileIO);
        }
        contents.resize(contents.size() + bytes_to_write.size());
        std::memcpy(contents.data() + contents.size() - bytes_to_write.size(), bytes_to_write.data(), bytes_to_write.size());
        write_idx += bytes_to_write.size();
    }
};

#endif
