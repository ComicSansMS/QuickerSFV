#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_HPP

#include <quicker_sfv/md5_digest.hpp>

#include <memory>
#include <span>

class MD5Hasher {
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> m_impl;
public:
    MD5Hasher();

    ~MD5Hasher();

    void addData(std::span<char const> data);

    MD5Digest getDigest();
};

#endif
