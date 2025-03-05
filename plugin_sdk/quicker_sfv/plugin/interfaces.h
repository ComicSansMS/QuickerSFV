#ifndef INCLUDE_GUARD_QUICKER_SFV_PLUGIN_SDK_INTERFACES_H
#define INCLUDE_GUARD_QUICKER_SFV_PLUGIN_SDK_INTERFACES_H

#include <stddef.h>
#include <stdint.h>

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
} QuickerSFV_CallbackResult;

typedef enum tag_QuickerSFV_ProviderCapabilities {
    QuickerSFV_ProviderCapabilities_Full     = 0,
    QuickerSFV_ProviderCapabilities_Reserved = 0xffffffff,
} QuickerSFV_ProviderCapabilities;

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
    QuickerSFV_Result (*AddData)(
        IQuickerSFV_Hasher* self,
        char const* data,
        size_t size
    );
    QuickerSFV_Result (*Finalize)(
        IQuickerSFV_Hasher* self,
        QuickerSFV_DigestP out_digest
    );
    QuickerSFV_Result (*Reset)(
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
    QuickerSFV_Result (*Delete)(
        IQuickerSFV_ChecksumProvider* self
    );
    QuickerSFV_Result (*GetProviderCapabilities)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_ProviderCapabilities* out_capabilities
    );
    QuickerSFV_Result (*FileExtension)(
        IQuickerSFV_ChecksumProvider* self,
        char* out_utf8_str,
        size_t* in_out_size
    );
    QuickerSFV_Result (*FileDescription)(
        IQuickerSFV_ChecksumProvider* self,
        char* out_utf8_str,
        size_t* in_out_size
    );
    QuickerSFV_Result (*CreateHasher)(
        IQuickerSFV_ChecksumProvider* self,
        IQuickerSFV_Hasher** out_ihasher,
        struct QuickerSFV_HasherOptions* opts
    );
    QuickerSFV_Result (*DeleteHasher)(
        IQuickerSFV_ChecksumProvider* self,
        IQuickerSFV_Hasher* ihasher
    );
    QuickerSFV_Result (*DigestFromString)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_DigestP out_digest,
        char const* string_data,
        size_t string_size
    );
    QuickerSFV_Result (*ReadFromFile)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_FileReadProviderP read_provider,
        QuickerSFV_CallbackResult (*read_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            char* out_read_buffer,
            size_t read_buffer_size,
            size_t* out_bytes_read
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
    QuickerSFV_Result (*WriteNewFile)(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_FileWriteProviderP write_provider,
        QuickerSFV_CallbackResult (*Write)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const* in_buffer,
            size_t in_buffer_size,
            size_t* out_bytes_written
        ),
        QuickerSFV_CallbackResult (*next_entry)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const** out_filename,
            void** out_digest_user_data
        )
    );
};


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace quicker_sfv::plugin_interface {

struct IFileOutput {
    virtual QuickerSFV_Result Write(
        char const* in_buffer,
        size_t in_buffer_size,
        size_t* out_bytes_written
    ) = 0;
};

struct IFileInput {
    virtual QuickerSFV_Result Read(
        char* out_buffer,
        size_t out_buffer_size,
        size_t* out_bytes_read
    ) = 0;
};

struct IHasher {
    virtual QuickerSFV_Result AddData(
        char const* data,
        size_t size
    ) = 0;
    virtual QuickerSFV_Result Finalize(
        QuickerSFV_DigestP out_digest
    ) = 0;
    virtual QuickerSFV_Result Reset() = 0;
};

struct IChecksumProvider {
    virtual QuickerSFV_Result Delete() = 0;
    virtual QuickerSFV_Result GetProviderCapabilities(
        QuickerSFV_ProviderCapabilities* out_capabilities
    ) = 0;
    virtual QuickerSFV_Result FileExtension(
        char* out_utf8_str,
        size_t* in_out_size
    ) = 0;
    virtual QuickerSFV_Result FileDescription(
        char* out_utf8_str,
        size_t* in_out_size
    ) = 0;
    virtual QuickerSFV_Result CreateHasher(
        IQuickerSFV_Hasher** out_ihasher,
        struct QuickerSFV_HasherOptions* opts
    ) = 0;
    virtual QuickerSFV_Result DeleteHasher(
        IQuickerSFV_Hasher* ihasher
    ) = 0;
    virtual QuickerSFV_Result DigestFromString(
        QuickerSFV_DigestP out_digest,
        char const* string_data,
        size_t string_size
    ) = 0;
    virtual QuickerSFV_Result ReadFromFile(
        QuickerSFV_FileReadProviderP read_provider,
        QuickerSFV_CallbackResult (*read_file_binary)(
            QuickerSFV_FileReadProviderP read_provider,
            char* out_read_buffer,
            size_t read_buffer_size,
            size_t* out_bytes_read
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
    virtual QuickerSFV_Result WriteNewFile(
        QuickerSFV_FileWriteProviderP write_provider,
        QuickerSFV_CallbackResult (*Write)(
            QuickerSFV_FileWriteProviderP write_provider,
            char const* in_buffer,
            size_t in_buffer_size,
            size_t* out_bytes_written
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
