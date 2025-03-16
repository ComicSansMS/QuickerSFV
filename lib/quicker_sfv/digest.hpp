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
#ifndef INCLUDE_GUARD_QUICKER_SFV_DIGEST_HPP
#define INCLUDE_GUARD_QUICKER_SFV_DIGEST_HPP

#include <concepts>
#include <memory>
#include <string>

namespace quicker_sfv {

/** Specifies that a type can be used as a digest.
 */
template<typename T>
concept IsDigest = std::regular<T> && requires(T const d) {
    { d.toString() } -> std::same_as<std::u8string>;
};

/** Type-erased container for checksum digest.
 * A checksum digest is provided either by a Hasher or parsed from a string
 * using ChecksumProvider::digestFromString().
 * Digest is a polymorphic type that can store arbitrary digests, but behaves like a
 * value type itself, that is, it can be copied and assigned like any `int` value.
 * Two Digests will only compare equal if they are of the same underlying dynamic
 * type.
 */
class Digest {
private:
    class Concept {
    public:
        virtual ~Concept() = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
        virtual std::u8string toString() const = 0;
        virtual bool equalTo(Concept const& rhs) const = 0;
    };

    template<IsDigest T>
    class Model: public Concept {
    private:
        T m_;
    public:
        explicit Model(T const& m)
            :m_(m)
        {}

        explicit Model(T&& m)
            :m_(std::move(m))
        {}

        ~Model() override = default;

        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model>(m_);
        }

        std::u8string toString() const override {
            return m_.toString();
        }

        bool equalTo(Concept const& rhs) const override {
            Model const* mrhs = dynamic_cast<Model const*>(&rhs);
            if (!mrhs) { return false; }
            return m_ == (mrhs->m_);
        }
    };

    std::unique_ptr<Concept> m_digest;
public:
    /** Constructor.
     * A Digest can store objects of any type that fulfills the IsDigest concept.
     * @param[in] digest The object to be stored.
     */
    template<typename T> requires( !std::is_same_v<std::remove_cvref_t<T>, Digest> ) && IsDigest<T>
    Digest(T&& digest)
        :m_digest(std::make_unique<Model<std::remove_cvref_t<T>>>(std::forward<T>(digest)))
    {}

    /** Default constructor.
     * Constructs an empty Digest value.
     * Empty Digests are only equal to other empty Digests.
     */
    Digest();
    ~Digest();
    Digest(Digest const& rhs);
    Digest& operator=(Digest const& rhs);
    Digest(Digest&& rhs);
    Digest& operator=(Digest&& rhs);

    /** Retrieves a string representation of the current Digest.
     * @note String representations are not required to be unique. Two Digests with
     *       the same string representation do not necessarily compare equal. However,
     *       the string representation is deterministic. Two Digests that compare
     *       equal must return the same string representation.
     */
    [[nodiscard]] std::u8string toString() const;

    friend bool operator==(Digest const& lhs, Digest const& rhs);
    friend bool operator!=(Digest const& lhs, Digest const& rhs);
};

static_assert(std::regular<Digest>, "Digest is not regular");

}
#endif
