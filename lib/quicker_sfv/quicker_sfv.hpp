#ifndef INCLUDE_GUARD_QUICKER_SFV_QUICKER_SFV_HPP
#define INCLUDE_GUARD_QUICKER_SFV_QUICKER_SFV_HPP

#include <quicker_sfv/checksum_file.hpp>
#include <quicker_sfv/checksum_provider.hpp>
#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/error.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/hasher.hpp>
#include <quicker_sfv/md5_provider.hpp>
#include <quicker_sfv/sfv_provider.hpp>
#include <quicker_sfv/utf.hpp>
#include <quicker_sfv/version.hpp>

/** QuickerSFV library.
 */
namespace quicker_sfv {

bool supportsAvx512();

}

#endif
