#ifndef INCLUDE_GUARD_QUICKER_SFV_MD5_HPP
#define INCLUDE_GUARD_QUICKER_SFV_MD5_HPP

#include <quicker_sfv/hasher.hpp>

#include <memory>
#include <span>

namespace quicker_sfv {

class MD5Hasher: public Hasher {
private:
    struct Pimpl;
    std::unique_ptr<Pimpl> m_impl;
public:
    MD5Hasher();

    ~MD5Hasher() override;
    void addData(std::span<char const> data) override;
    Digest finalize() override;
    Digest digestFromString(std::u8string_view str) const;
};

}
#endif
