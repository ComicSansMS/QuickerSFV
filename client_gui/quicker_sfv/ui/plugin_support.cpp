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
#ifndef QUICKER_SFV_BUILD_SELF_CONTAINED
#include <quicker_sfv/ui/plugin_support.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/line_reader.hpp>

#include <bit>
#include <memory>
#include <utility>

namespace quicker_sfv::gui {
namespace {

struct PluginDigest {
    void* user_data = nullptr;
    void (*free_user_data)(void* user_data) = nullptr;
    void* (*clone)(void* user_data) = nullptr;
    size_t (*to_string)(void* user_data, char* out_str) = nullptr;
    int8_t (*compare)(void* user_data_lhs, void* user_data_rhs) = nullptr;

    PluginDigest() = default;
    explicit PluginDigest(void* user_data,
        void (*free_user_data)(void* user_data), 
        void* (*clone)(void* user_data),
        size_t (*to_string)(void* user_data, char* out_str),
        int8_t (*compare)(void* user_data_lhs, void* user_data_rhs))
        :user_data(user_data), free_user_data(free_user_data), clone(clone), to_string(to_string), compare(compare)
    {}
    PluginDigest(PluginDigest const& rhs)
        :PluginDigest()
    {
        if (rhs.user_data) {
            user_data = rhs.clone(rhs.user_data);
            if (!user_data) { throwException(Error::PluginError); }
            free_user_data = rhs.free_user_data;
            clone = rhs.clone;
            to_string = rhs.to_string;
            compare = rhs.compare;
        }
    }
    PluginDigest& operator=(PluginDigest const& rhs)
    {
        if (this != &rhs) {
            if (user_data) {
                free_user_data(user_data);
            }
            user_data = rhs.clone(rhs.user_data);
            if (!user_data) { throwException(Error::PluginError); }
            free_user_data = rhs.free_user_data;
            clone = rhs.clone;
            to_string = rhs.to_string;
            compare = rhs.compare;
        }
        return *this;
    }
    PluginDigest(PluginDigest&& rhs) noexcept
        :user_data(std::exchange(rhs.user_data, nullptr)),
         free_user_data(std::exchange(rhs.free_user_data, nullptr)),
         clone(std::exchange(rhs.clone, nullptr)),
         to_string(std::exchange(rhs.to_string, nullptr)),
         compare(std::exchange(rhs.compare, nullptr))
    {}
    PluginDigest& operator=(PluginDigest&& rhs) noexcept
    {
        if (this != &rhs) {
            if (user_data) {
                free_user_data(user_data);
            }
            user_data = std::exchange(rhs.user_data, nullptr);
            free_user_data = std::exchange(rhs.free_user_data, nullptr);
            clone = std::exchange(rhs.clone, nullptr);
            to_string = std::exchange(rhs.to_string, nullptr);
            compare = std::exchange(rhs.compare, nullptr);
        }
        return *this;
    }

    std::u8string toString() const {
        if (!user_data) { return {}; }
        size_t const required_size = to_string(user_data, nullptr);
        std::u8string ret;
        ret.resize(required_size);
        to_string(user_data, reinterpret_cast<char*>(ret.data()));
        ret.pop_back();
        return ret;
    }

    friend bool operator==(PluginDigest const& lhs, PluginDigest const& rhs) {
        if (!lhs.user_data) {
            return !rhs.user_data;
        }
        return lhs.compare(lhs.user_data, rhs.user_data) == 0;
    }
};

static_assert(IsDigest<PluginDigest>);

void fillDigest(QuickerSFV_DigestP out_digest,
                void* user_data,
                void (*free_user_data)(void* user_data),
                void* (*clone)(void* user_data),
                size_t (*to_string)(void* user_data, char* out_str),
                int8_t (*compare)(void* user_data_lhs, void* user_data_rhs))
{
    Digest* d = reinterpret_cast<Digest*>(out_digest);
    *d = PluginDigest(user_data, free_user_data, clone, to_string, compare);
}

struct PluginHasher : public quicker_sfv::Hasher {
    plugin_interface::IHasher* hasher;
    plugin_interface::IChecksumProvider* pif;

    explicit PluginHasher(plugin_interface::IHasher* hasher, plugin_interface::IChecksumProvider* pif)
        :hasher(hasher), pif(pif)
    {}

    ~PluginHasher() override {
        pif->DeleteHasher(reinterpret_cast<IQuickerSFV_Hasher*>(hasher));
    }
    PluginHasher& operator=(PluginHasher&&) = delete;
    void addData(std::span<std::byte const> data) override {
        QuickerSFV_Result const res = hasher->AddData(reinterpret_cast<char const*>(data.data()), data.size());
        if (res != QuickerSFV_Result_OK) { throwException(Error::PluginError); }
    }

    Digest finalize() override {
        Digest digest;
        QuickerSFV_Result const res = hasher->Finalize(reinterpret_cast<QuickerSFV_DigestP>(&digest));
        if (res != QuickerSFV_Result_OK) { throwException(Error::PluginError); }
        return digest;
    }
    
    void reset() override {
        QuickerSFV_Result const res = hasher->Reset();
        if (res != QuickerSFV_Result_OK) { throwException(Error::PluginError); }
    }
};

struct PluginChecksumProvider : public quicker_sfv::ChecksumProvider {
    plugin_interface::IChecksumProvider* pif;
    ProviderCapabilities capabilities;
    std::u8string file_extension;
    std::u8string file_description;

    explicit PluginChecksumProvider(plugin_interface::IChecksumProvider* pif)
        :pif(pif)
    {
        QuickerSFV_ProviderCapabilities caps;
        if (pif->GetProviderCapabilities(&caps) != QuickerSFV_Result_OK) {
            throwException(Error::PluginError);
        }
        switch (caps) {
        case QuickerSFV_ProviderCapabilities_Full:
            capabilities = ProviderCapabilities::Full;
            break;
        case QuickerSFV_ProviderCapabilities_VerifyOnly:
            capabilities = ProviderCapabilities::VerifyOnly;
            break;
        default: throwException(Error::PluginError);
        }

        size_t required_size_file_extension = 0;
        pif->FileExtension(nullptr, &required_size_file_extension);
        file_extension.resize(required_size_file_extension);
        size_t bytes_written = required_size_file_extension;
        if ((pif->FileExtension(reinterpret_cast<char*>(file_extension.data()), &bytes_written) != QuickerSFV_Result_OK) ||
            (bytes_written != required_size_file_extension))
        {
            throwException(Error::PluginError);
        }
        file_extension.pop_back();

        size_t required_size_file_description = 0;
        pif->FileDescription(nullptr, &required_size_file_description);
        file_description.resize(required_size_file_description);
        if ((pif->FileDescription(reinterpret_cast<char*>(file_description.data()), &bytes_written) != QuickerSFV_Result_OK) ||
            (bytes_written != required_size_file_description))
        {
            throwException(Error::PluginError);
        }
        file_description.pop_back();
    }

    PluginChecksumProvider& operator=(PluginChecksumProvider&&) = delete;

    ~PluginChecksumProvider() override {
        pif->Delete();
    }
    
    [[nodiscard]] ProviderCapabilities getCapabilities() const noexcept override {
        return capabilities;
    }
    
    [[nodiscard]] std::u8string_view fileExtensions() const noexcept override {
        return file_extension;
    }
    
    [[nodiscard]] std::u8string_view fileDescription() const noexcept override {
        return file_description;
    }
    
    [[nodiscard]] HasherPtr createHasher(HasherOptions const& hasher_options) const override {
        IQuickerSFV_Hasher* hasher;
        QuickerSFV_HasherOptions opts{
            .opt_size = sizeof(QuickerSFV_HasherOptions),
            .has_sse42 = hasher_options.has_sse42,
            .has_avx512 = hasher_options.has_avx512,
            .reserved = {}
        };
        if (pif->CreateHasher(&hasher, &opts) != QuickerSFV_Result_OK) {
            throwException(Error::PluginError);
        }
        plugin_interface::IHasher* h = std::bit_cast<plugin_interface::IHasher*>(hasher);
        return HasherPtr(new PluginHasher(h, pif));
    }
    
    [[nodiscard]] Digest digestFromString(std::u8string_view str) const override {
        Digest ret;
        if (pif->DigestFromString(reinterpret_cast<QuickerSFV_DigestP>(&ret), reinterpret_cast<char const*>(str.data()), str.size()) != QuickerSFV_Result_OK) {
            throwException(Error::PluginError);
        }
        return ret;
    }

    ChecksumFile readFromFile(FileInput& file_input) const override {
        ChecksumFile ret;
        struct ReadInput {
            PluginChecksumProvider const* provider;
            FileInput* file_input;
            ChecksumFile* checksum_file;
            LineReader line_reader;
            std::u8string line;
        } read_provider{ this, &file_input, &ret, LineReader(file_input), {} };
        QuickerSFV_Result const res = pif->ReadFromFile(reinterpret_cast<QuickerSFV_FileReadProviderP>(&read_provider),
            // read_file_binary()
            [](QuickerSFV_FileWriteProviderP read_provider, char* out_read_buffer, size_t read_buffer_size, size_t* out_bytes_read) -> QuickerSFV_CallbackResult {
                ReadInput* ri = reinterpret_cast<ReadInput*>(read_provider);
                try {
                    size_t const bytes_read = ri->file_input->read(std::span<std::byte>(reinterpret_cast<std::byte*>(out_read_buffer), read_buffer_size));
                    if (bytes_read == FileInput::RESULT_END_OF_FILE) {
                        *out_bytes_read = 0;
                        return QuickerSFV_CallbackResult_Ok;
                    }
                    *out_bytes_read = bytes_read;
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
                return QuickerSFV_CallbackResult_MoreData;
            },
            // seek_file_binary()
            [](QuickerSFV_FileReadProviderP read_provider, int64_t offset, QuickerSFV_SeekStart seek_start) -> QuickerSFV_CallbackResult {
                ReadInput* ri = reinterpret_cast<ReadInput*>(read_provider);
                quicker_sfv::FileInput::SeekStart s;
                if (seek_start == QuickerSFV_SeekStart_CurrentPosition) {
                    s = quicker_sfv::FileInput::SeekStart::CurrentPosition;
                } else if (seek_start == QuickerSFV_SeekStart_FileStart) {
                    s = quicker_sfv::FileInput::SeekStart::FileStart;
                } else if (seek_start == QuickerSFV_SeekStart_FileEnd) {
                    s = quicker_sfv::FileInput::SeekStart::FileEnd;
                } else {
                    return QuickerSFV_CallbackResult_InvalidArg;
                }
                try {
                    ri->file_input->seek(offset, s);
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
                return QuickerSFV_CallbackResult_Ok;
            },
            // tell_file_binary()
            [](QuickerSFV_FileReadProviderP read_provider, int64_t* out_position) -> QuickerSFV_CallbackResult {
                ReadInput* ri = reinterpret_cast<ReadInput*>(read_provider);
                try {
                    int64_t const res = ri->file_input->tell();
                    *out_position = res;
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
                return QuickerSFV_CallbackResult_Ok;
            },
            // read_line_text()
            [](QuickerSFV_FileWriteProviderP read_provider, char const** out_line, size_t* out_line_size) -> QuickerSFV_CallbackResult {
                ReadInput* ri = reinterpret_cast<ReadInput*>(read_provider);
                if (ri->line_reader.done()) { return QuickerSFV_CallbackResult_Ok; }
                try {
                    std::optional<std::u8string> opt_str = ri->line_reader.readLine();
                    if (!opt_str) { return QuickerSFV_CallbackResult_Failed; }
                    ri->line = std::move(*opt_str);
                    *out_line = reinterpret_cast<char const*>(ri->line.c_str());
                    *out_line_size = ri->line.size();
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
                return QuickerSFV_CallbackResult_MoreData;
            },
            // new_entry_callback()
            [](QuickerSFV_FileWriteProviderP read_provider, char const* filename, char const* digest_string) -> QuickerSFV_CallbackResult {
                ReadInput* ri = reinterpret_cast<ReadInput*>(read_provider);
                try {
                    ri->checksum_file->addEntry(
                        reinterpret_cast<char8_t const*>(filename),
                        ri->provider->digestFromString(reinterpret_cast<char8_t const*>(digest_string)));
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
                return QuickerSFV_CallbackResult_Ok;
            });
        if (res != QuickerSFV_Result_OK) {
            throwException(Error::PluginError);
        }
        return ret;
    }
    void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const override {
        auto sstr = f.getEntries().front().digest.toString();
        struct WriteProvider {
            FileOutput* fout;
            ChecksumFile const* checksum_file;
            std::span<ChecksumFile::Entry const>::iterator it;
            std::u8string digest_string;
        } write_provider{ &file_output, &f, f.getEntries().begin(), {} };
        QuickerSFV_Result const res = pif->WriteNewFile(reinterpret_cast<QuickerSFV_FileWriteProviderP>(&write_provider),
            // write()
            [](QuickerSFV_FileWriteProviderP write_provider, char const* buffer, size_t buffer_size) -> QuickerSFV_CallbackResult {
                WriteProvider* wp = reinterpret_cast<WriteProvider*>(write_provider);
                try {
                    wp->fout->write(std::span<std::byte const>(reinterpret_cast<std::byte const*>(buffer), buffer_size));
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
                return QuickerSFV_CallbackResult_Ok;
            },
            // next_entry()
            [](QuickerSFV_FileWriteProviderP write_provider, char const** out_filename, char const** out_digest) -> QuickerSFV_CallbackResult {
                WriteProvider* wp = reinterpret_cast<WriteProvider*>(write_provider);
                try {
                    if (wp->it == wp->checksum_file->getEntries().end()) {
                        *out_filename = nullptr;
                        *out_digest = nullptr;
                        return QuickerSFV_CallbackResult_Ok;
                    }
                    *out_filename = reinterpret_cast<char const*>(wp->it->path.c_str());
                    wp->digest_string = wp->it->digest.toString();
                    *out_digest = reinterpret_cast<char const*>(wp->digest_string.c_str());
                    ++wp->it;
                    return QuickerSFV_CallbackResult_MoreData;
                } catch (...) {
                    return QuickerSFV_CallbackResult_Failed;
                }
            });
        if (res != QuickerSFV_Result_OK) {
            throwException(Error::PluginError);
        }
    }
};

} // anonymous namespace

QuickerSFV_ChecksumProvider_Callbacks* pluginCallbacks() {
    static QuickerSFV_ChecksumProvider_Callbacks cbs{
        .fillDigest = fillDigest
    };
    return &cbs;
}

ChecksumProviderPtr loadPlugin(QuickerSFV_LoadPluginFunc plugin_load_function) {
    IQuickerSFV_ChecksumProvider* p = plugin_load_function(pluginCallbacks());
    if (!p) { throwException(Error::PluginError); }
    plugin_interface::IChecksumProvider* pif = std::bit_cast<plugin_interface::IChecksumProvider*>(p);
    return ChecksumProviderPtr(new PluginChecksumProvider(pif));
}

ChecksumProviderPtr loadPluginCpp(QuickerSFV_LoadPluginCppFunc plugin_cpp_load_function) {
    QuickerSFV_CppPluginLoader* l = nullptr;
    plugin_cpp_load_function(&l);
    if (!l) { throwException(Error::PluginError); }
    return l->createChecksumProvider();
}

}
#endif // QUICKER_SFV_BUILD_SELF_CONTAINED
