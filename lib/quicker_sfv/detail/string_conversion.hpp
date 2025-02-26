#ifndef INCLUDE_GUARD_QUICKER_SFV_STRING_CONVERSION_HPP
#define INCLUDE_GUARD_QUICKER_SFV_STRING_CONVERSION_HPP

#include <cstddef>

namespace quicker_sfv::string_conversion {

struct Nibbles {
    char8_t higher;
    char8_t lower;
};

std::byte hex_str_to_byte(char8_t higher, char8_t lower);

std::byte hex_str_to_byte(Nibbles const& nibbles);

Nibbles byte_to_hex_str(std::byte b);


}
#endif
