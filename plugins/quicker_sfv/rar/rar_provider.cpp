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
#include <quicker_sfv/rar/rar_provider.hpp>

#include <quicker_sfv/detail/crc32.hpp>
#include <quicker_sfv/error.hpp>

#include <bit>
#include <optional>

namespace quicker_sfv::rar {

constexpr std::byte const rar_signature[] = { std::byte{0x52}, std::byte{0x61}, std::byte{0x72}, std::byte{0x21}, std::byte{0x1A}, std::byte{0x07} };

enum class FileType {
    Rar4,
    Rar5
};

FileType seekSignature(FileInput& fi) {
    auto find_signature_start = [](FileInput& fi) {
        std::byte b[1];
        do { fi.read(b); } while (b[0] != rar_signature[0]);
    };

    std::byte b[1];
    for (;;) {
        find_signature_start(fi);
        for (;;) {
            bool is_valid = true;
            for (size_t i = 1; i < 6; ++i) {
                fi.read(b);
                if (b[0] != rar_signature[i]) { is_valid = false; break; }
            }
            if (b[0] == rar_signature[0]) { continue; }
            if (!is_valid) { break; }
            fi.read(b);
            if (b[0] == std::byte{ 0 }) {
                return FileType::Rar4;
            } else if (b[0] == std::byte{ 0x1 }) {
                fi.read(b);
                if (b[0] == std::byte{ 0 }) {
                    return FileType::Rar5;
                } else if (b[0] == rar_signature[0]) {
                    continue;
                }
            } else if (b[0] == rar_signature[0]) {
                continue;
            }
        }
    }
}

struct VInt {
    uint64_t i;
    int8_t raw_size;
};

class CollectingFileInput : public FileInput {
private:
    FileInput* m_upstream;
    std::vector<std::byte> m_data;
public:
    CollectingFileInput(FileInput& upstream)
        :m_upstream(&upstream)
    { }

    CollectingFileInput& operator=(CollectingFileInput&&) = delete;

    ~CollectingFileInput() override = default;

    size_t read(std::span<std::byte> read_buffer) override {
        size_t ret = m_upstream->read(read_buffer);
        if ((ret > 0) && (ret != FileInput::RESULT_END_OF_FILE)) {
            m_data.insert(m_data.end(), read_buffer.begin(), read_buffer.begin() + ret);
        }
        return ret;
    }

    int64_t seek(int64_t offset, SeekStart seek_start) override {
        reset();
        return m_upstream->seek(offset, seek_start);
    }

    int64_t tell() override {
        return m_upstream->tell();
    }


    std::u8string_view current_file() const override {
        return m_upstream->current_file();
    }

    bool open(std::u8string_view new_file) override {
        return m_upstream->open(new_file);
    }

    uint64_t file_size() override {
        return m_upstream->file_size();
    }

    std::span<std::byte const> data() const {
        return m_data;
    }

    void reset() {
        m_data.clear();
    }
};

VInt parseVInt(FileInput& fi) {
    VInt ret{};
    std::byte b[1];
    bool is_valid = false;
    for (uint64_t i = 0; i < 10; ++i) {
        fi.read(b);
        ++ret.raw_size;
        ret.i |= static_cast<uint64_t>(std::bit_cast<uint8_t>(b[0] & std::byte{0x7f})) << (7 * i);
        if ((b[0] & std::byte{ 0x80 }) == std::byte{ 0 }) { is_valid = true; break; }
    }
    if (!is_valid) { throwException(Error::ParserError); }
    return ret;
}

struct RarFileHeader {
    static constexpr uint64_t const hasExtraArea = 0x01;
    static constexpr uint64_t const hasDataArea  = 0x02;
    uint32_t crc32;
    VInt header_size;
    VInt header_type;
    VInt header_flags;
    VInt archive_flags;
    std::optional<VInt> extra_area_size;
    std::optional<VInt> data_size;
};

RarFileHeader parseHeader(FileInput& fi) {
    RarFileHeader ret{};
    fi.read(std::span<std::byte>(reinterpret_cast<std::byte*>(&ret.crc32), 4));
    ret.header_size = parseVInt(fi);
    ret.header_type = parseVInt(fi);
    ret.header_flags = parseVInt(fi);
    std::vector<std::byte> header_data;
    header_data.resize(static_cast<size_t>(ret.header_size.i - ret.header_type.raw_size - ret.header_flags.raw_size));
    fi.read(header_data);
    if (ret.header_type.i & RarFileHeader::hasExtraArea) {
        // @todo
    }
    if (ret.header_type.i & RarFileHeader::hasDataArea) {
        // @todo
    }
    return ret;
}

class RarHasher : public Hasher {
private:
    detail::Crc32Hasher m_crcHasher;
public:
    explicit RarHasher(HasherOptions const& hasher_options)
        :m_crcHasher(hasher_options)
    {}
    ~RarHasher() override = default;
    RarHasher& operator=(RarHasher&&) = delete;
    void addData(std::span<std::byte const> data) override {
        return m_crcHasher.addData(data);
    }
    Digest finalize() override {
        return m_crcHasher.finalize();
    }
    void reset() override {
        m_crcHasher.reset();
    }
};

RarProvider::RarProvider() = default;
RarProvider::~RarProvider() = default;
ProviderCapabilities RarProvider::getCapabilities() const noexcept {
    return ProviderCapabilities::VerifyOnly;
}

std::u8string_view RarProvider::fileExtensions() const noexcept {
    return u8"*.rar";
}

std::u8string_view RarProvider::fileDescription() const noexcept {
    return u8"RAR Archive";
}

HasherPtr RarProvider::createHasher(HasherOptions const& hasher_options) const {
    return std::make_unique<RarHasher>(hasher_options);
}

Digest RarProvider::digestFromString(std::u8string_view str) const {
    return detail::Crc32Hasher::digestFromString(str);
}

ChecksumFile RarProvider::readFromFile(FileInput& file_input) const {
    CollectingFileInput fi(file_input);
    FileType ft = seekSignature(fi);
    if (ft != FileType::Rar5) {
        // Rar4 not supported for now
        throwException(Error::ParserError);
    }
    fi.reset();
    RarFileHeader header = parseHeader(fi);
    detail::Crc32Hasher hasher(HasherOptions{});
    hasher.reset();
    // hash header contents (skipping the crc itself, which is not part of the hash)
    hasher.addData(std::span<std::byte const>(fi.data().begin() + sizeof(header.crc32), fi.data().end()));
    Digest const header_digest = hasher.finalize();
    if (header_digest != detail::Crc32Hasher::digestFromRaw(header.crc32)) {
        throwException(Error::ParserError);
    }
    return {};
}

void RarProvider::writeNewFile(FileOutput&, ChecksumFile const&) const {
    throwException(Error::Failed);
}

struct RarPluginLoader : QuickerSFV_CppPluginLoader {
    quicker_sfv::ChecksumProviderPtr createChecksumProvider() const override {
        return std::make_unique<RarProvider>();
    }
};

}

extern "C"
void QuickerSFV_LoadPlugin_Cpp(QuickerSFV_CppPluginLoader** loader) {
    static quicker_sfv::rar::RarPluginLoader l;
    *loader = &l;
}

