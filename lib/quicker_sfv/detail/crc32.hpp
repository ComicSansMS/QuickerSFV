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
    explicit Crc32Hasher(HasherOptions const& opt);
    ~Crc32Hasher() override;
    void addData(std::span<std::byte const> data) override;
    Digest finalize() override;
    void reset() override;
    static Digest digestFromString(std::u8string_view str);
    static Digest digestFromRaw(uint32_t d);
    static bool checkType(Digest const& d);
};

}

#endif
