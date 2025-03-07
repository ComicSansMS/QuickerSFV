#ifndef INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_HASHER_HPP

#include <quicker_sfv/digest.hpp>

#include <span>
#include <string>
#include <string_view>

namespace quicker_sfv {

/** Options for configuring Hasher.
 */
struct HasherOptions {
    bool has_sse42;     ///< CPU supports the SSE4.2 instruction set.
    bool has_avx512;    ///< CPU supports the AVX-512 instruction set.
};

/** Hasher interface.
 * A Hasher computes the checksum digests for the entries of a ChecksumFile.
 * The Hasher is provided by a ChecksumProvider.
 *
 * Clients may provide subsequent chunks of data to the Hasher with the addData()
 * function. All provided data will be added to the checksum of the Hasher.
 * Once all data for a single file has been provided, finalize() will provide the
 * checksum Digest of the hashed data.
 */
class Hasher {
public:
    virtual ~Hasher() = 0;
    Hasher& operator=(Hasher&&) = delete;
    /** Add additional data to the current checksum.
     * @param[in] data Data to be included in the current checksum.
     * @pre The Hasher is not in its finalized state.
     * @throw Exception Error::HasherFailure If the operation fails.
     */
    virtual void addData(std::span<std::byte const> data) = 0;
    /** Finalize the current checksum and retrieve the digest for all added data.
     * After finalizing the Hasher, the only valid operation is to reset() it back
     * to its initial state. Once finalized, no more data can be added to the Hasher.
     * A Hasher also can not be finalized twice to obtain the same Digest again.
     * @return The Digest of all data provided to addData() since the last reset().
     * @pre The Hasher is not in its finalized state.
     * @throw Exception Error::HasherFailure If the operation fails.
     */
    virtual Digest finalize() = 0;
    /** Reset the Hasher back to its initial state.
     * @throw Exception Error::HasherFailure If the operation fails.
     */
    virtual void reset() = 0;
};

}

#endif
