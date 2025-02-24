#include <quicker_sfv/checksum_file.hpp>

namespace quicker_sfv {
namespace detail {
void HasherPtrDeleter::operator()(Hasher* p) const {
    deleter(p);
}

void ChecksumFilePtrDeleter::operator()(ChecksumFile* p) const {
    deleter(p);
}
}

ChecksumFile::~ChecksumFile() = default;

}
