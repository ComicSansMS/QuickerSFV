#include <quicker_sfv/quicker_sfv.hpp>

#include <openssl/md5.h>

namespace quicker_sfv {

Version getVersion() {
    return Version{
        .major = 1,
        .minor = 0,
        .patch = 0
    };
}

void md5(void* data, size_t size, unsigned char* out)
{
    MD5_CTX md5c;
    MD5_Init(&md5c);
    MD5_Update(&md5c, data, size);
    MD5_Final(out, &md5c);
}


}
