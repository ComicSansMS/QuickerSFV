#include <quicker_sfv/digest.hpp>

namespace quicker_sfv {

Digest::Concept::~Concept() = default;

Digest::Digest()
    :m_digest(nullptr)
{}

Digest::~Digest() = default;

Digest::Digest(Digest const& rhs)
    :m_digest(rhs.m_digest->clone())
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
