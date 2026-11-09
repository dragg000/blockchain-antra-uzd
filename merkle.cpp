#include "merkle.hpp"
#include <stdexcept>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

HashStream::Digest decode_id(const Hash &id)
{
    if (id.size() != 64)
        throw std::invalid_argument("Transakcijos ID turi buti 64 hex simboliai");

    HashStream::Digest bytes{};
    for (size_t i = 0; i < bytes.size(); i++)
    {
        const int high = hex_value(id[i * 2]);
        const int low = hex_value(id[i * 2 + 1]);
        if (high < 0 || low < 0)
            throw std::invalid_argument("Transakcijos ID turi netinkama simboli");

        bytes[i] = static_cast<std::uint8_t>(high * 16 + low);
    }
    return bytes;
}

static HashStream::Digest parent_of(const HashStream::Digest &left, const HashStream::Digest &right)
{
    HashStream stream;
    stream.update(reinterpret_cast<const char *>(left.data()), left.size());
    stream.update(reinterpret_cast<const char *>(right.data()), right.size());
    return stream.digest();
}

Hash merkle_root(const std::vector<Hash> &ids)
{
    if (ids.empty())
        return hashTextUtf8("");

    // Lapai dekoduojami viena karta, toliau visas medis skaiciuojamas baitais.
    std::vector<HashStream::Digest> level;
    level.reserve(ids.size() + 1);
    for (const Hash &id : ids)
    {
        level.push_back(decode_id(id));
    }

    while (level.size() > 1)
    {
        if (level.size() % 2 == 1)
            level.push_back(level.back());

        // Tevai rasomi i to paties vektoriaus pradzia: tevas i / 2 perraso tik jau panaudotus vaikus.
        for (size_t i = 0; i < level.size(); i += 2)
        {
            level[i / 2] = parent_of(level[i], level[i + 1]);
        }
        level.resize(level.size() / 2);
    }

    return digest_to_hex(level[0]);
}
