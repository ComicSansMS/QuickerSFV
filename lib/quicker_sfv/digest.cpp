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
#include <quicker_sfv/digest.hpp>

namespace quicker_sfv {

Digest::Concept::~Concept() = default;

Digest::Digest()
    :m_digest(nullptr)
{}

Digest::~Digest() = default;

Digest::Digest(Digest const& rhs)
    :m_digest(rhs.m_digest ? rhs.m_digest->clone() : nullptr)
{}

Digest& Digest::operator=(Digest const& rhs) {
    if (this != &rhs) {
        m_digest = rhs.m_digest->clone();
    }
    return *this;
}

Digest::Digest(Digest&& rhs) = default;
Digest& Digest::operator=(Digest&& rhs) = default;

std::u8string Digest::toString() const {
    return m_digest ? m_digest->toString() : u8"";
}

bool operator==(Digest const& lhs, Digest const& rhs) {
    if (!lhs.m_digest) {
        return !rhs.m_digest;
    }
    return lhs.m_digest->equalTo(*rhs.m_digest);
}

bool operator!=(Digest const& lhs, Digest const& rhs) {
    return !(lhs == rhs);
}

}
