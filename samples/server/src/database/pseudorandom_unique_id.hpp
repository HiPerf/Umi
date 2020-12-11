#pragma once

#include <chacha.h>


class pseudorandom_unique_id
{
public:
    pseudorandom_unique_id();

    uint64_t next();

private:
    CryptoPP::SecByteBlock _key;
    CryptoPP::SecByteBlock _iv;
    CryptoPP::ChaCha::Encryption _enc;
    uint64_t _counter;
};
