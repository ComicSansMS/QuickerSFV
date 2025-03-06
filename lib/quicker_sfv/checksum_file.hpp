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
    /** An entry from a checksum file to be checked.
     * Each entry will appear as its own line in the list of checked files in the UI.
     */
    struct Entry {
        std::u8string path;     ///< Path to the entity to be checked.
        Digest digest;          ///< Checksum digest for the entity to be checked.
    };
private:
    std::vector<Entry> m_entries;
public:
    /** Retrieves all entries.
     */
    [[nodiscard]] std::span<const Entry> getEntries() const;
    /** Add a new entry.
     * New entries will be appended to the end of the list of entries.
     * @param[in] path Path of the entry.
     * @param[in] digest Checksum digest of the entry.
     *
     * @note addEntry() will permit at maximum 2^32 entries.
     * @throw Exception Error::Failed if the ChecksumFile already contains the
     *                  maximum number of entries.
     */
    void addEntry(std::u8string_view path, Digest digest);
    /** Sort all entries lexicographically by their paths.
     */
    void sortEntries();
    /** Clear the checksum file, leaving it with no entries.
     */
    void clear();
};

}

#endif
