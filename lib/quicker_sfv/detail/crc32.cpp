#include <quicker_sfv/detail/crc32.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/detail/string_conversion.hpp>

#include <fast_crc32.hpp>

namespace quicker_sfv {

namespace {
struct CrcDigest {
    uint32_t data;

    CrcDigest();

    explicit CrcDigest(uint32_t d);

    std::u8string toString() const;

    friend bool operator==(CrcDigest const&, CrcDigest const&) = default;
};

static_assert(IsDigest<CrcDigest>, "CrcDigest is not a digest");

CrcDigest::CrcDigest()
    :data(0)
{ }

CrcDigest::CrcDigest(uint32_t d)
    :data(d)
{ }

std::u8string CrcDigest::toString() const {
    std::u8string ret;
    ret.reserve(5);
    auto bytes = std::span<std::byte const>(reinterpret_cast<std::byte const*>(&data), 4);
    for (auto it = bytes.rbegin(), it_end = bytes.rend(); it != it_end; ++it) {
        auto const n = string_conversion::byte_to_hex_str(*it);
        ret.push_back(n.higher);
        ret.push_back(n.lower);
    }
    return ret;
}

} // anonymous namespace

Crc32Hasher::Crc32Hasher()
    :m_state(0), m_useAvx512(false)
{}

Crc32Hasher::Crc32Hasher(Crc32UseAvx512_T)
    :m_state(0), m_useAvx512(true)
{}

Crc32Hasher::~Crc32Hasher() = default;

void Crc32Hasher::addData(std::span<std::byte const> data) {
    m_state = crc::crc32(reinterpret_cast<char const*>(data.data()), data.size(), m_state, m_useAvx512);
}

Digest Crc32Hasher::finalize() {
    return CrcDigest{ m_state };
}

void Crc32Hasher::reset() {
    m_state = 0;
}

/* static */
Digest Crc32Hasher::digestFromString(std::u8string_view str) {
    if (str.size() != 8) { throwException(Error::ParserError); }
    auto conv = [](char8_t h, char8_t l) -> uint32_t {
        using string_conversion::hex_str_to_byte;
        return static_cast<uint32_t>(hex_str_to_byte(h, l));
    };
    uint32_t d =
        (conv(str[0], str[1]) << 24) |
        (conv(str[2], str[3]) << 16) |
        (conv(str[4], str[5]) << 8) |
        conv(str[6], str[7]);
    return CrcDigest{ d };
}

/* static */ bool Crc32Hasher::checkType(Digest const& d) {
    return d.checkType(typeid(CrcDigest));
}

}
