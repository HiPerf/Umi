#include "database/pseudorandom_unique_id.hpp"

#include <algparam.h>
#include <argnames.h>
#include <osrng.h>


pseudorandom_unique_id::pseudorandom_unique_id() :
    _key(32),
    _iv(8),
    _counter(0)
{
    CryptoPP::AutoSeededRandomPool prng;
    prng.GenerateBlock(_key, _key.size());
    prng.GenerateBlock(_iv, _iv.size());

    const CryptoPP::AlgorithmParameters params = CryptoPP::MakeParameters(CryptoPP::Name::Rounds(), 8)
        (CryptoPP::Name::IV(), CryptoPP::ConstByteArrayParameter(_iv, 8));

    _enc.SetKey(_key, _key.size(), params);
}

uint64_t pseudorandom_unique_id::next()
{
    CryptoPP::byte data[8];

    uint64_t counter = _counter++ + std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    _enc.ProcessData(data, (CryptoPP::byte*)&counter, 8);

    return *(uint64_t*)data;
}
