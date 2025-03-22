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
#ifndef INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_PROVIDER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_PROVIDER_HPP

#include <quicker_sfv/checksum_file.hpp>
#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/hasher.hpp>

#include <memory>
#include <string_view>

namespace quicker_sfv {

/** Provides facilities for reading, writing, and checking a checksum file format.
 */
class ChecksumProvider;

/** Smart pointer for Hasher.
 */
using HasherPtr = std::unique_ptr<Hasher>;
/** Smart pointer for ChecksumProvider.
 */
using ChecksumProviderPtr = std::unique_ptr<ChecksumProvider>;

/** Capabilities for ChecksumProvider.
 */
enum class ProviderCapabilities {
    Full,           ///< Supports all functionalities.
    VerifyOnly,     ///< Supports only verify, but not create.
                    ///  Calls to ChecksumProvider::writeNewFile() will always fail.
};

class ChecksumProvider {
public:
    ChecksumProvider& operator=(ChecksumProvider&&) = delete;

    /** Destructor.
     */
    virtual ~ChecksumProvider() = 0;
    /** Returns the provider's supported capabilities.
     */
    [[nodiscard]] virtual ProviderCapabilities getCapabilities() const noexcept = 0;
    /** The file extensions of all supported file formats.
     * The list is a semicolon-separated list of file extensions of the form `*.ext`.
     * The file extension is used by the UI to determine whether a provider is capable
     * of processing a file based on its file name.
     */
    [[nodiscard]] virtual std::u8string_view fileExtensions() const noexcept = 0;
    /** A short, user-readable description of the checksum file format.
     */
    [[nodiscard]] virtual std::u8string_view fileDescription() const noexcept = 0;
    /** Creates a Hasher suitable for computing checksum Digests for this format.
     * The returned Hasher is used to create hashes for each of the ChecksumFile
     * entries used with readFromFile() and writeNewFile().
     * @param[in] hasher_options Options for configuring the created Hasher.
     * @throws Exception Error::Failed If the hasher cannot be created.
     *                   Error::PluginError If a plugin failure occurs.
     */
    [[nodiscard]] virtual HasherPtr createHasher(HasherOptions const& hasher_options) const = 0;
    /** Parse a Digest from string.
     * The Digest format is determined by the Hasher returned by createHasher().
     * Digests produced by this function are guaranteed to be consistent with those
     * produced by the Hasher.
     * @param[in] str String to be parsed.
     * @throws Exception Error::ParserError if the string is invalid.
     *                   Error::PluginError If a plugin failure occurs.
     */
    [[nodiscard]] virtual Digest digestFromString(std::u8string_view str) const = 0;

    /** Reads a ChecksumFile from file.
     * The format of the file is determined by the ChecksumProvider.
     * @param[in] file_input A FileInput object providing access to the file data.
     * @return A ChecksumFile with the deserialized contents of file_input.
     * @throws Exception Error::ParserError if the file format is invalid.
     *                   Error::FileIO if an error occurs while reading the file.
     *                   Error::PluginError If a plugin failure occurs.
     */
    virtual ChecksumFile readFromFile(FileInput& file_input) const = 0;
    /** Writes a ChecksumFile out to a file.
     * The format of the file is determined by the ChecksumProvider.
     * @param[in] file_output A FileOutput object providing access to the file.
     * @param[in] f The ChecksumFile to be serialized.
     * @throws Exception Error::FileIO if an error occurs while writing the file.
     *                   Error::PluginError if a plugin failure occurs.
     *                   Error::Failed if the provider does not have the capabilities
     *                   for writing ChecksumFile. This must be indicated by returning
     *                   ProviderCapabilities::VerifyOnly from getCapabilities().
     */
    virtual void writeNewFile(FileOutput& file_output, ChecksumFile const& f) const = 0;
};

}

#endif
