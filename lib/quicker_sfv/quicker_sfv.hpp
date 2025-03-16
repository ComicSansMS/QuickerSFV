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
#include <quicker_sfv/string_utilities.hpp>
#include <quicker_sfv/version.hpp>

/** QuickerSFV library.
 */
namespace quicker_sfv {

/** Checks whether the CPU supports the SSE4.2 instruction set.
 * @note This is always true for x64 CPUs.
 */
bool supportsSse42();

/** Checks whether the CPU supports the AVX512 instruction set.
 */
bool supportsAvx512();

}

#endif
