#include <quicker_sfv/checksum_provider.hpp>

namespace quicker_sfv {
namespace detail {
void HasherPtrDeleter::operator()(Hasher* p) const {
    deleter(p);
}

void ChecksumProviderPtrDeleter::operator()(ChecksumProvider* p) const {
    deleter(p);
}
}

ChecksumProvider::~ChecksumProvider() = default;

}
