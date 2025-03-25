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
#ifndef INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_CHECKSUM_FILE_HPP

#include <quicker_sfv/digest.hpp>
#include <quicker_sfv/file_io.hpp>
#include <quicker_sfv/hasher.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace quicker_sfv {

/** Representation of the contents of a single checksum file.
 * A checksum file is e.g. a `.sfv` or a `.md5` file. These files contain a list
 * of relative file paths along with the checksums for those files.
 *
 * @todo Support for handling self contained file formats.
 */
class ChecksumFile {
public:
    /** A data portion is a continuous segment of data that contributes to a checksum.
     * A checksum is computed from one or more data portions, potentially split
     * across multiple files.
     */
    struct DataPortion {
        std::u8string path;     ///< Path to the file that contains the data.
        int64_t data_offset;    ///< Offset to the data from the start of the file.
        int64_t data_size;      ///< Size of the data in bytes; If this is -1, the
                                ///  entire remainder of the file starting from the
                                ///  offset will be checked.
    };
    /** An entry from a checksum file to be checked.
     * Each entry will appear as its own line in the list of checked files in the UI.
     */
    struct Entry {
        std::u8string display;  ///< String to be used for displaying the item in UI.
        Digest digest;          ///< Checksum digest for the entity to be checked.
        std::vector<DataPortion> data;
                                ///< All data contributing to the checksum digest.
    };
private:
    std::vector<Entry> m_entries;
public:
    /** Retrieves all entries.
     */
    [[nodiscard]] std::span<const Entry> getEntries() const;

    /** Adds a new entry.
     * New entries will be appended to the end of the list of entries.
     * @param[in] path Path of the entry.
     * @param[in] digest Checksum digest of the entry.
     *
     * @note addEntry() will permit at maximum 2^32 entries.
     * @throw Exception Error::Failed if the ChecksumFile already contains the
     *                  maximum number of entries.
     */
    void addEntry(std::u8string_view path, Digest digest);

    /** Adds a new entry with extended information.
     * Use of this function is required if file hashing does not just hash the
     * entirety of a single file. In contrast to the simpler overload of addEntry()
     * this function allows for the following:
     *  - The displayed file name in the UI may be different from the file that is
     *    opened on disk.
     *  - The data portion may be smaller than the entirety of the file.
     *  - More than one data portion, potentially from different files, may be used.
     * @param[in] digest Checksum digest of the entry.
     * @param[in] display String that will be printed for this entry in the UI.
     * @param[in] data All data portions passed to the Hasher for computing the
     *                 checksum digest.
     *
     * @note addEntry() will permit at maximum 2^32 entries.
     * @throw Exception Error::Failed if the ChecksumFile already contains the
     *                  maximum number of entries.
     */
    void addEntry(Digest digest, std::u8string_view display, std::vector<DataPortion> data);

    /** Sorts all entries lexicographically by their paths.
     */
    void sortEntries();

    /** Clears the checksum file, leaving it with no entries.
     */
    void clear();
};

}

#endif
