#include <quicker_sfv/md5.hpp>

#include <openssl/md5.h>

#include <array>
#include <stdexcept>

struct MD5Hasher::Pimpl {
    MD5_CTX context;
};

MD5Hasher::MD5Hasher()
    :m_impl(std::make_unique<Pimpl>())
{
    int res = MD5_Init(&m_impl->context);
    if (res != 1) { throw std::runtime_error("Error initializing md5"); }
}

MD5Hasher::~MD5Hasher() = default;

void MD5Hasher::addData(std::span<char const> data)
{
    int res = MD5_Update(&m_impl->context, data.data(), data.size());
    if (res != 1) { throw std::runtime_error("Error updating md5"); }
}

MD5Digest MD5Hasher::getDigest()
{
    static_assert(sizeof(MD5Digest::data) == MD5_DIGEST_LENGTH);
    //MD5_CTX context = m_impl->context;
    MD5Digest ret;
    int res = MD5_Final(reinterpret_cast<unsigned char*>(&ret.data), &m_impl->context);
    if (res != 1) { throw std::runtime_error("Error finalizing md5"); }
    return ret;
}

