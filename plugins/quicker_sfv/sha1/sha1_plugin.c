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
#include <quicker_sfv/sha1/sha1_export.h>

#include <quicker_sfv/plugin/plugin_sdk.h>

#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/sha.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNREFERENCED_PARAMETER(p) (void)p

static inline unsigned char hex_char_to_nibble(char x, int8_t* success) {
    if (*success == 0) { return 255; }
    if ((x >= '0') && (x <= '9')) {
        *success = 1;
        return x - '0';
    } else if ((x >= 'a') && (x <= 'f')) {
        *success = 1;
        return x - 'a' + 10;
    } else if ((x >= 'A') && (x <= 'F')) {
        *success = 1;
        return x - 'A' + 10;
    }
    *success = 0;
    return 255;
}

static inline char nibble_to_hex_char(unsigned char c) {
    if (c < 10) {
        return c + '0';
    } else if (c < 16) {
        return c - 10 + 'a';
    }
    return '?';
}

static inline unsigned char lower_nibble(unsigned char b) {
    return (b & 0x0f);
}

static inline unsigned char higher_nibble(unsigned char b) {
    return ((b & 0xf0) >> 4);
}

static inline unsigned char hex_str_to_byte(char higher, char lower, int8_t* success) {
    return (hex_char_to_nibble(higher, success) << 4) | hex_char_to_nibble(lower, success);
}


static struct IQuickerSFV_Hasher_Vtbl g_HasherVtbl;
static struct IQuickerSFV_ChecksumProvider_Vtbl g_ChecksumProviderVtbl;

typedef struct tag_QuickerSFV_ChecksumProvider_Impl {
    IQuickerSFV_ChecksumProvider base_type;
    QuickerSFV_ChecksumProvider_Callbacks callbacks;
} QuickerSFV_ChecksumProvider_Impl;

typedef struct tag_QuickerSFV_Hasher_Impl {
    IQuickerSFV_Hasher base_type;
    QuickerSFV_ChecksumProvider_Impl* provider;
    SHA_CTX context;
    int8_t is_valid;
} QuickerSFV_Hasher_Impl;

// {FDD46AE9-55AB-4A19-967A-C7CA0AB841FA}
static QuickerSFV_GUID const DIGEST_GUID = { 0xfdd46ae9, 0x55ab, 0x4a19, 0x967ac7ca0ab841fa };

typedef struct tag_Digest_UserData {
    QuickerSFV_GUID guid;
    unsigned char digest[SHA_DIGEST_LENGTH];
} Digest_UserData;

static Digest_UserData* createDigestUserData(void) {
    Digest_UserData* user_data = malloc(sizeof(Digest_UserData));
    if (!user_data) { return NULL; }
    memcpy(&user_data->guid, &DIGEST_GUID, sizeof(QuickerSFV_GUID));
    memset(&user_data->digest, 0, SHA_DIGEST_LENGTH);
    return user_data;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_Hasher_AddData(IQuickerSFV_Hasher* self, 
                                                                            char const* data, size_t size) {
    QuickerSFV_Hasher_Impl* h = (QuickerSFV_Hasher_Impl*)self;
    assert(h->is_valid);
    SHA1_Update(&h->context, data, size);
    return QuickerSFV_Result_OK;
}

static void* IQuickerSFV_Hasher_Finalize_clone(void* user_data) {
    assert(memcmp(&((Digest_UserData*)user_data)->guid, &DIGEST_GUID, sizeof(DIGEST_GUID)) == 0);
    void* copy = malloc(sizeof(Digest_UserData));
    if (!copy) { return NULL; }
    memcpy(copy, user_data, sizeof(Digest_UserData));
    return copy;

}

static size_t IQuickerSFV_Hasher_Finalize_to_string(void* user_data, char* out_str) {
    assert(memcmp(&((Digest_UserData*)user_data)->guid, &DIGEST_GUID, sizeof(DIGEST_GUID)) == 0);
    if (out_str) {
        Digest_UserData* ud = user_data;
        for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
            out_str[i*2] = nibble_to_hex_char(higher_nibble(ud->digest[i]));
            out_str[i*2 + 1] = nibble_to_hex_char(lower_nibble(ud->digest[i]));
        }
        out_str[SHA_DIGEST_LENGTH * 2] = '\0';
    }
    return (SHA_DIGEST_LENGTH * 2) + 1;
}

static int8_t IQuickerSFV_Hasher_Finalize_compare(void* user_data_lhs, void* user_data_rhs) {
    assert(memcmp(&((Digest_UserData*)user_data_lhs)->guid, &DIGEST_GUID, sizeof(DIGEST_GUID)) == 0);
    if (memcmp(&((Digest_UserData*)user_data_rhs)->guid, &DIGEST_GUID, sizeof(DIGEST_GUID)) != 0) {
        return 1;
    }
    Digest_UserData* ud_lhs = (Digest_UserData*)user_data_lhs;
    Digest_UserData* ud_rhs = (Digest_UserData*)user_data_rhs;
    for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        if (ud_lhs->digest[i] != ud_rhs->digest[i]) { return 1; }
    }
    return 0;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_Hasher_Finalize(IQuickerSFV_Hasher* self,
                                                                             QuickerSFV_DigestP out_digest) {
    QuickerSFV_Hasher_Impl* h = (QuickerSFV_Hasher_Impl*)self;
    assert(h->is_valid);
    Digest_UserData* user_data = createDigestUserData();
    if (!user_data) { return QuickerSFV_Result_InsufficientMemory; }
    SHA1_Final(user_data->digest, &h->context);
    h->is_valid = 0;
    h->provider->callbacks.fillDigest(out_digest, user_data, free,
                                      IQuickerSFV_Hasher_Finalize_clone,
                                      IQuickerSFV_Hasher_Finalize_to_string,
                                      IQuickerSFV_Hasher_Finalize_compare);
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_Hasher_Reset(IQuickerSFV_Hasher* self) {
    QuickerSFV_Hasher_Impl* h = (QuickerSFV_Hasher_Impl*)self;
    SHA1_Init(&h->context);
    h->is_valid = 1;
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_Delete(IQuickerSFV_ChecksumProvider* self) {
    free(self);
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_GetProviderCapabilities(
        IQuickerSFV_ChecksumProvider* self,
        QuickerSFV_ProviderCapabilities* out_capabilities
    )
{
    UNREFERENCED_PARAMETER(self);
    *out_capabilities = QuickerSFV_ProviderCapabilities_Full;
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_FileExtension(
        IQuickerSFV_ChecksumProvider* self,
        char* out_utf8_str,
        size_t* in_out_size
    )
{
    UNREFERENCED_PARAMETER(self);
    char const file_extension_str[] = "*.sha1";
    size_t const file_extension_str_size = sizeof(file_extension_str);
    if (*in_out_size < file_extension_str_size) {
        *in_out_size = file_extension_str_size;
        return QuickerSFV_Result_InsufficientMemory;
    }
    memcpy(out_utf8_str, file_extension_str, file_extension_str_size);
    *in_out_size = file_extension_str_size;
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_FileDescription(
            IQuickerSFV_ChecksumProvider* self,
            char* out_utf8_str,
            size_t* in_out_size
    )
{
    UNREFERENCED_PARAMETER(self);
    char const file_desc_str[] = "SHA1";
    size_t const file_desc_str_size = sizeof(file_desc_str);
    if (*in_out_size < file_desc_str_size) {
        *in_out_size = file_desc_str_size;
        return QuickerSFV_Result_InsufficientMemory;
    }
    memcpy(out_utf8_str, file_desc_str, file_desc_str_size);
    *in_out_size = file_desc_str_size;
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_CreateHasher(
            IQuickerSFV_ChecksumProvider* self,
            IQuickerSFV_Hasher** out_ihasher,
            struct QuickerSFV_HasherOptions* opts
    )
{
    UNREFERENCED_PARAMETER(opts);
    QuickerSFV_ChecksumProvider_Impl* p = (QuickerSFV_ChecksumProvider_Impl*)self;
    QuickerSFV_Hasher_Impl* impl = malloc(sizeof(QuickerSFV_Hasher_Impl));
    if (!impl) { return QuickerSFV_Result_InsufficientMemory; }
    impl->base_type.vptr = &g_HasherVtbl;
    impl->provider = p;
    impl->is_valid = 0;
    *out_ihasher = &impl->base_type;
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_DeleteHasher(
            IQuickerSFV_ChecksumProvider* self,
            IQuickerSFV_Hasher* ihasher
    )
{
    UNREFERENCED_PARAMETER(self);
    free(ihasher);
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_DigestFromString(
            IQuickerSFV_ChecksumProvider* self,
            QuickerSFV_DigestP out_digest,
            char const* string_data,
            size_t string_size
    )
{
    QuickerSFV_ChecksumProvider_Impl* p = (QuickerSFV_ChecksumProvider_Impl*)self;
    if (string_size < SHA_DIGEST_LENGTH * 2) {
        return QuickerSFV_Result_Failed;
    }
    Digest_UserData* user_data = createDigestUserData();
    if (!user_data) { return QuickerSFV_Result_InsufficientMemory; }
    for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        int8_t success = 1;
        user_data->digest[i] = hex_str_to_byte(string_data[2*i], string_data[2*i + 1], &success);
        if (!success) { free(user_data); return QuickerSFV_Result_Failed; }
    }
    p->callbacks.fillDigest(out_digest, user_data, free,
                            IQuickerSFV_Hasher_Finalize_clone,
                            IQuickerSFV_Hasher_Finalize_to_string,
                            IQuickerSFV_Hasher_Finalize_compare);
    return QuickerSFV_Result_OK;
}

static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_ReadFromFile(
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
    )
{
    UNREFERENCED_PARAMETER(self);
    UNREFERENCED_PARAMETER(read_file_binary);
    UNREFERENCED_PARAMETER(seek_file_binary);
    UNREFERENCED_PARAMETER(tell_file_binary);
    char const* line;
    size_t line_size;
    QuickerSFV_CallbackResult res;
    char digest[SHA_DIGEST_LENGTH * 2 + 1];
    while ((res = read_line_text(read_provider, &line, &line_size)) == QuickerSFV_CallbackResult_MoreData) {
        size_t li = 0;
        if (line_size < 1) { continue; }
        if (line[0] == ';') { continue; }
        if (line_size < 2*SHA_DIGEST_LENGTH + 3 + li) { return QuickerSFV_Result_Failed; }
        for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
            // validate that the characters are in range for digest
            int8_t success = 1;
            hex_str_to_byte(line[li + 2*i], line[li + 2*i + 1], &success);
            if (!success) { return QuickerSFV_Result_Failed; }
            // copy characters to digest string
            digest[2*i] = line[li + 2*i];
            digest[2*i + 1] = line[li + 2*i + 1];
        }
        digest[2*SHA_DIGEST_LENGTH] = '\0';
        li += 2*SHA_DIGEST_LENGTH;
        if (line[li++] != ' ') { return QuickerSFV_Result_Failed; }
        if (line[li++] != '*') { return QuickerSFV_Result_Failed; }
        char* filename = malloc(line_size - li + 1);
        if (!filename) { return QuickerSFV_Result_InsufficientMemory; }
        memcpy(filename, &line[li], line_size - li);
        filename[line_size - li] = '\0';
        res = new_entry_callback(read_provider, filename, digest);
        free(filename);
        if (res != QuickerSFV_CallbackResult_Ok) { break; }
    }
    return (res == QuickerSFV_CallbackResult_Ok) ? QuickerSFV_Result_OK : QuickerSFV_Result_Failed;
}
static QuickerSFV_Result QUICKER_SFV_PLUGIN_CALL IQuickerSFV_ChecksumProvider_WriteNewFile(
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
    )
{
    UNREFERENCED_PARAMETER(self);
    char* write_buffer = NULL;
    size_t write_buffer_size = 0;
    char const* filename;
    char const* digest;
    QuickerSFV_CallbackResult res;
    while ((res = next_entry(write_provider, &filename, &digest)) == QuickerSFV_CallbackResult_MoreData) {
        size_t required_size = strlen(filename) + (SHA_DIGEST_LENGTH * 2) + 10;
        size_t bytes_formatted = 0;
        if (write_buffer_size < required_size) {
            if (write_buffer) { free(write_buffer); }
            write_buffer = malloc(required_size * 2);
            if (!write_buffer) { return QuickerSFV_Result_InsufficientMemory; }
            write_buffer_size = required_size * 2;
        }
#ifdef _MSC_VER
        bytes_formatted = sprintf_s(write_buffer, write_buffer_size, "%s *%s\r\n", digest, filename);
#else
        bytes_formatted = sprintf(write_buffer, "%s *%s\r\n", digest, filename);
#endif
        if (bytes_formatted <= 0) { return QuickerSFV_Result_Failed; }
        res = Write(write_provider, write_buffer, bytes_formatted);
        if (res != QuickerSFV_CallbackResult_Ok) { break; }
    }
    free(write_buffer);
    return (res == QuickerSFV_CallbackResult_Ok) ? QuickerSFV_Result_OK : QuickerSFV_Result_Failed;
}

static void init_vtables(void) {
    g_HasherVtbl.AddData = IQuickerSFV_Hasher_AddData;
    g_HasherVtbl.Finalize = IQuickerSFV_Hasher_Finalize;
    g_HasherVtbl.Reset = IQuickerSFV_Hasher_Reset;

    g_ChecksumProviderVtbl.Delete = IQuickerSFV_ChecksumProvider_Delete;
    g_ChecksumProviderVtbl.GetProviderCapabilities = IQuickerSFV_ChecksumProvider_GetProviderCapabilities;
    g_ChecksumProviderVtbl.FileExtension = IQuickerSFV_ChecksumProvider_FileExtension;
    g_ChecksumProviderVtbl.FileDescription = IQuickerSFV_ChecksumProvider_FileDescription;
    g_ChecksumProviderVtbl.CreateHasher = IQuickerSFV_ChecksumProvider_CreateHasher;
    g_ChecksumProviderVtbl.DeleteHasher = IQuickerSFV_ChecksumProvider_DeleteHasher;
    g_ChecksumProviderVtbl.DigestFromString = IQuickerSFV_ChecksumProvider_DigestFromString;
    g_ChecksumProviderVtbl.ReadFromFile = IQuickerSFV_ChecksumProvider_ReadFromFile;
    g_ChecksumProviderVtbl.WriteNewFile = IQuickerSFV_ChecksumProvider_WriteNewFile;
}

QUICKER_SFV_PLUGIN_SHA1_EXPORT IQuickerSFV_ChecksumProvider* QuickerSFV_LoadPlugin(QuickerSFV_ChecksumProvider_Callbacks* cbs) {
    init_vtables();
    QuickerSFV_ChecksumProvider_Impl* ret = malloc(sizeof(QuickerSFV_ChecksumProvider_Impl));
    if (!ret) { return NULL; }
    ret->base_type.vptr = &g_ChecksumProviderVtbl;
    ret->callbacks = *cbs;
    return &ret->base_type;
}
