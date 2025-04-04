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
#ifndef INCLUDE_GUARD_QUICKER_SFV_PLUGIN_SDK_INTERFACES_H
#define INCLUDE_GUARD_QUICKER_SFV_PLUGIN_SDK_INTERFACES_H

#include <stddef.h>
#include <stdint.h>

#ifdef _MSC_VER
#   define QUICKER_SFV_PLUGIN_CALL __stdcall
#else
#   define QUICKER_SFV_PLUGIN_CALL
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef enum tag_QuickerSFV_Result {
    QuickerSFV_Result_OK                    = 1,
    QuickerSFV_Result_Failed                = -1,
    QuickerSFV_Result_NotImplemented        = -5,
    QuickerSFV_Result_InsufficientMemory    = -10

} QuickerSFV_Result;

typedef enum tag_QuickerSFV_CallbackResult {
    QuickerSFV_CallbackResult_Ok            = 1,
    QuickerSFV_CallbackResult_MoreData      = 2,
    QuickerSFV_CallbackResult_Failed        = -1,
    QuickerSFV_CallbackResult_InvalidArg    = -2,
} QuickerSFV_CallbackResult;

typedef enum tag_QuickerSFV_ProviderCapabilities {
    QuickerSFV_ProviderCapabilities_Full       = 0,
    QuickerSFV_ProviderCapabilities_VerifyOnly = 1,
    QuickerSFV_ProviderCapabilities_Reserved   = INT32_MAX,
} QuickerSFV_ProviderCapabilities;

typedef enum tag_QuickerSFV_SeekStart {
    QuickerSFV_SeekStart_CurrentPosition = 1,
    QuickerSFV_SeekStart_FileStart       = 2,
    QuickerSFV_SeekStart_FileEnd         = 3
} QuickerSFV_SeekStart;

typedef struct tag_QuickerSFV_GUID {
    uint32_t b1;
    uint16_t b2;
    uint16_t b3;
    uint64_t b4;
} QuickerSFV_GUID;

typedef char* QuickerSFV_DigestP;
typedef char* QuickerSFV_ChecksumFileP;

struct QuickerSFV_HasherOptions {
    size_t opt_size;
    uint8_t has_sse42;
    uint8_t has_avx512;
    uint8_t reserved[6];
};

typedef char* QuickerSFV_FileReadProviderP;
typedef char* QuickerSFV_FileWriteProviderP;

struct IQuickerSFV_Hasher_Vtbl;

typedef struct tag_IQuickerSFV_Hasher {
    struct IQuickerSFV_Hasher_Vtbl* vptr;
} IQuickerSFV_Hasher;

struct IQuickerSFV_Hasher_Vtbl {
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *AddData)(
        IQuickerSFV_Hasher* self,
        char const* data,
        size_t size
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *Finalize)(
        IQuickerSFV_Hasher* self,
        QuickerSFV_DigestP out_digest
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *Reset)(
        IQuickerSFV_Hasher* self
    );
};


typedef struct tag_QuickerSFV_ChecksumProvider_Callbacks {
    void (*fillDigest)(QuickerSFV_DigestP out_digest,
        void* user_data,
        void (*free_user_data)(void* user_data),
        void* (*clone)(void* user_data),
        size_t (*to_string)(void* user_data, char* out_str),
        int8_t (*compare)(void* user_data_lhs, void* user_data_rhs)
    );
} QuickerSFV_ChecksumProvider_Callbacks;

struct IQuickerSFV_ChecksumProvider_Vtbl;

typedef struct tag_IQuickerSFV_ChecksumProvider {
    struct IQuickerSFV_ChecksumProvider_Vtbl* vptr;
} IQuickerSFV_ChecksumProvider;

struct IQuickerSFV_ChecksumProvider_Vtbl {
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *Delete)(
        IQuickerSFV_ChecksumProvider* self
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL*GetProviderCapabilities)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_ProviderCapabilities* out_capabilities
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *FileExtension)(
        IQuickerSFV_ChecksumProvider* self,
        char* out_utf8_str,
        size_t* in_out_size
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *FileDescription)(
        IQuickerSFV_ChecksumProvider* self,
        char* out_utf8_str,
        size_t* in_out_size
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *CreateHasher)(
        IQuickerSFV_ChecksumProvider* self,
        IQuickerSFV_Hasher** out_ihasher,
        struct QuickerSFV_HasherOptions* opts
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *DeleteHasher)(
        IQuickerSFV_ChecksumProvider* self,
        IQuickerSFV_Hasher* ihasher
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *DigestFromString)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_DigestP out_digest,
        char const* string_data,
        size_t string_size
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *ReadFromFile)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_FileReadProviderP read_provider,
        QuickerSFV_CallbackResult (*read_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            char* out_read_buffer,
            size_t read_buffer_size,
            size_t* out_bytes_read
        ),
        QuickerSFV_CallbackResult (*seek_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            int64_t offset,
            QuickerSFV_SeekStart seek_start
        ),
        QuickerSFV_CallbackResult (*tell_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            int64_t* out_position
        ),
        QuickerSFV_CallbackResult (*read_line_text)(
            QuickerSFV_FileReadProviderP read_provider,
            char const** out_line,
            size_t* out_line_size
        ),
        QuickerSFV_CallbackResult (*new_entry_callback)(
            QuickerSFV_FileReadProviderP read_provider,
            char const* filename,
            char const* digest_string
        )
    );
    QuickerSFV_Result (QUICKER_SFV_PLUGIN_CALL *WriteNewFile)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_FileWriteProviderP write_provider,
        QuickerSFV_CallbackResult (*Write)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const* in_buffer,
            size_t in_buffer_size
        ),
        QuickerSFV_CallbackResult (*next_entry)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const** out_filename,
            char const** out_digest
        )
    );
};


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace quicker_sfv::plugin_interface {

struct IHasher {
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL AddData(
        char const* data,
        size_t size
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL Finalize(
        QuickerSFV_DigestP out_digest
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL Reset() = 0;
};

struct IChecksumProvider {
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL Delete() = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL GetProviderCapabilities(
        QuickerSFV_ProviderCapabilities* out_capabilities
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL FileExtension(
        char* out_utf8_str,
        size_t* in_out_size
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL FileDescription(
        char* out_utf8_str,
        size_t* in_out_size
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL CreateHasher(
        IQuickerSFV_Hasher** out_ihasher,
        struct QuickerSFV_HasherOptions* opts
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL DeleteHasher(
        IQuickerSFV_Hasher* ihasher
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL DigestFromString(
        QuickerSFV_DigestP out_digest,
        char const* string_data,
        size_t string_size
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL ReadFromFile(
        QuickerSFV_FileReadProviderP read_provider,
        QuickerSFV_CallbackResult (*read_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            char* out_read_buffer,
            size_t read_buffer_size,
            size_t* out_bytes_read
        ),
        QuickerSFV_CallbackResult (*seek_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            int64_t offset,
            QuickerSFV_SeekStart seek_start
        ),
        QuickerSFV_CallbackResult (*tell_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            int64_t* out_position
        ),
        QuickerSFV_CallbackResult (*read_line_text)(
            QuickerSFV_FileReadProviderP read_provider,
            char const** out_line,
            size_t* out_line_size
        ),
        QuickerSFV_CallbackResult (*new_entry_callback)(
            QuickerSFV_FileReadProviderP read_provider,
            char const* filename,
            char const* digest_string
        )
    ) = 0;
    virtual QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL WriteNewFile(
        QuickerSFV_FileWriteProviderP write_provider,
        QuickerSFV_CallbackResult (*Write)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const* in_buffer,
            size_t in_buffer_size
        ),
        QuickerSFV_CallbackResult (*next_entry)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const** out_filename,
            char const** out_digest
        )
    ) = 0;
};

} /* namespace quicker_sfv_interface */
#endif

#endif
