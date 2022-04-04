#pragma once
#include <array>
extern "C" {
#include "lib/c25519/src/sha512.h"
}

namespace SHA512Half {

static constexpr size_t Len = SHA512_HASH_SIZE/2;
using Hash = std::array<uint8_t,Len>;

inline Hash Calc(std::string_view str) {
    sha512_state s;
    sha512half_init(&s);
    const uint8_t* data = (const uint8_t*)str.data();
    size_t off = 0;
    size_t rem = str.size();
    while (rem >= SHA512_BLOCK_SIZE) {
        sha512_block(&s, data+off);
        off += SHA512_BLOCK_SIZE;
        rem -= SHA512_BLOCK_SIZE;
    }
    sha512_final(&s, data, str.size());
    
    Hash r;
    sha512_get(&s, r.data(), 0, Len);
    return r;
}

inline std::string StringForHash(const Hash& hash) {
    char str[Len*2+1];
    size_t i = 0;
    for (uint8_t b : hash) {
        sprintf(str+i, "%02x", b);
        i += 2;
    }
    return str;
}

} // namespace SHA512Half
