#ifndef INCLUDE_GUARD_QUICKER_SFV_CRC32_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CRC32_HPP

#include <quicker_sfv/hasher.hpp>

#include <cstdint>

namespace quicker_sfv {

class Crc32Hasher: public Hasher {
private:
    uint32_t m_state;
    bool m_useAvx512;
public:
    Crc32Hasher();
    ~Crc32Hasher() override;
    void addData(std::span<char const> data) override;
    Digest finalize() override;
    Digest digestFromString(std::u8string_view str) const override;
};

}

#endif
