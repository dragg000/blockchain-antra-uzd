#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

std::string hashTextUtf8(const std::string& text);

// Ta pati maisos funkcija, tik duomenys paduodami dalimis (v0.3).
//
// Rezultatas visada sutampa su hashTextUtf8 visam tekstui is karto: naudojama ta pati
// bloko maisymo funkcija, tas pats pradinis vektorius ir tas pats uzpildymas.
// Nauda kasimui: nekintanti antrastes pradzia sumaisoma viena karta, o kiekvienam nonce
// kopijuojama tarpine busena ir skaiciuojama tik likusi dalis.
class HashStream
{
public:
    using Digest = std::array<std::uint8_t, 32>;

    HashStream();

    void update(const char *data, std::size_t size);
    void update(const std::string &text) { update(text.data(), text.size()); }

    // Abi funkcijos busenos nekeicia, todel ta pati tarpine busena tinka daug kartu.
    Digest digest() const;
    std::string hex() const;

private:
    std::array<std::uint32_t, 8> state;
    std::array<std::uint8_t, 64> buffer; // dar nesumaisyti baitai, visada maziau nei 64
    std::size_t buffered;
    std::uint64_t total_bytes;
};

// Ar maisos hex uzrasas prasideda `count` nuliu. Tikrinami neapdoroti baitai:
// vienas hex simbolis yra puse baito.
bool has_leading_zero_nibbles(const HashStream::Digest &digest, std::uint32_t count);

// 32 baitai -> 64 mazosios hex raides.
std::string digest_to_hex(const HashStream::Digest &digest);
