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
#include <quicker_sfv/detail/md5.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/detail/string_conversion.hpp>

#include <openssl/md5.h>

#include <array>
#include <stdexcept>

#ifdef _MSC_VER
#   define SUPPRESS_DEPRECATED_WARNING() _Pragma("warning(suppress : 4996)") void()
#else
#   define SUPPRESS_DEPRECATED_WARNING() void()
#   if defined(__clang__)
#       pragma clang diagnostic ignored "-Wdeprecated-declarations"
#   elif defined(__GNUC__)
#       pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#   endif
#endif

namespace quicker_sfv::detail {
namespace {
struct MD5Digest {
    std::byte data[16];

    MD5Digest();

    static MD5Digest fromString(std::u8string_view str);

    std::u8string toString() const;

    friend bool operator==(MD5Digest const&, MD5Digest const&) = default;
};

static_assert(IsDigest<MD5Digest>, "MD5Digest is not a digest");

MD5Digest::MD5Digest()
    :data{}
{
}

MD5Digest MD5Digest::fromString(std::u8string_view str) {
    MD5Digest ret;
    if (str.size() != 32) { throwException(Error::ParserError); }
    for (int i = 0; i < 16; ++i) {
        char8_t const upper = str[i*2];
        char8_t const lower = str[i*2 + 1];
        ret.data[i] = string_conversion::hex_str_to_byte(upper, lower);
    }
    return ret;
}

std::u8string MD5Digest::toString() const {
    std::u8string ret;
    ret.reserve(17);
    for (std::byte const& b : data) {
        auto const n = string_conversion::byte_to_hex_str(b);
        ret.push_back(n.higher);
        ret.push_back(n.lower);
    }
    return ret;
}

} // anonymous namespace

struct MD5Hasher::Pimpl {
    MD5_CTX context;
};

MD5Hasher::MD5Hasher()
    :m_impl(std::make_unique<Pimpl>())
{
    reset();
}

MD5Hasher::~MD5Hasher() = default;

void MD5Hasher::addData(std::span<std::byte const> data)
{
    SUPPRESS_DEPRECATED_WARNING();
    int res = MD5_Update(&m_impl->context, data.data(), data.size());
    if (res != 1) { throwException(Error::HasherFailure); }
}

Digest MD5Hasher::finalize()
{
    static_assert(sizeof(MD5Digest::data) == MD5_DIGEST_LENGTH);
    MD5Digest ret;
    SUPPRESS_DEPRECATED_WARNING();
    int res = MD5_Final(reinterpret_cast<unsigned char*>(&ret.data), &m_impl->context);
    if (res != 1) { throwException(Error::HasherFailure); }
    return ret;
}

void MD5Hasher::reset() {
    SUPPRESS_DEPRECATED_WARNING();
    int res = MD5_Init(&m_impl->context);
    if (res != 1) { throwException(Error::HasherFailure); }
}

/* static */
Digest MD5Hasher::digestFromString(std::u8string_view str) {
    return MD5Digest::fromString(str);
}

}
