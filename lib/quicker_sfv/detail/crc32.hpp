#ifndef INCLUDE_GUARD_QUICKER_SFV_CRC32_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CRC32_HPP

#include <quicker_sfv/hasher.hpp>

#include <cstdint>

namespace quicker_sfv::detail {

/** CRC32 Hasher.
 * Hasher for the CRC32 checksum (CRC-32/ISO-HDLC) algorithm.
 */
class Crc32Hasher: public Hasher {
private:
    uint32_t m_state;
    bool m_useAvx512;
    bool m_useSse42;
public:
    explicit Crc32Hasher(HasherOptions const& opt);
    ~Crc32Hasher() override;
    void addData(std::span<std::byte const> data) override;
    Digest finalize() override;
    void reset() override;
    static Digest digestFromString(std::u8string_view str);
    static Digest digestFromRaw(uint32_t d);
};

}

#endif
