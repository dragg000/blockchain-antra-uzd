#include "hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

static inline u32 rotl32(u32 x, int r) {
    return (x << r) | (x >> (32 - r));
}

static constexpr std::array<u32, 8> IV = {
    0x8F1B2C3Du, 0x4A7A5F1Cu, 0xD6E7F8A1u, 0x92C17D4Eu,
    0xB9E6A4D2u, 0x5C41F0B7u, 0x76D1AE2Cu, 0xE9C4B5D8u
};

static std::vector<u8> toBytes(const std::string& text) {
    return std::vector<u8>(text.begin(), text.end());
}

static std::vector<u8> padBytes(std::vector<u8> message) {
    const u64 originalBits = static_cast<u64>(message.size()) * 8ULL;
    message.push_back(0x80u);
    while ((message.size() % 64u) != 56u) {
        message.push_back(0x00u);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<u8>((originalBits >> shift) & 0xFFu));
    }
    return message;
}

static std::array<u32, 8> mixBlock(const std::array<u32, 8>& state, const std::array<u32, 16>& words) {
    std::array<u32, 8> tmp = state;
    for (int round = 0; round < 7; ++round) {
        for (int i = 0; i < 8; ++i) {
            u32 a = tmp[i];
            u32 b = words[(i + round * 3) % 16];
            u32 c = tmp[(i + 3) % 8];
            u32 d = words[(i * 5 + round) % 16];
            u32 x = (b ^ d) + c + (u32(round + 1) * 0x9E3779B9u);
            a ^= rotl32(x, 9 + (i % 5));
            a = rotl32(a + tmp[(i + 5) % 8] + words[(i + round * 2) % 16], 11 + (round % 4));
            tmp[(i + 1) % 8] ^= rotl32(a + words[(i * 7 + round) % 16], 7 + (round % 3));
            tmp[i] = a;
        }
    }

    std::array<u32, 8> out = state;
    for (int i = 0; i < 8; ++i) {
        out[i] ^= rotl32(tmp[(i + 2) % 8] + words[(i * 5 + 3) % 16] + 0xC2B2AE3Du, 13 + (i % 5));
        out[i] += tmp[i] ^ words[(i * 3 + 2) % 16];
    }
    return out;
}

std::vector<u8> hashData(const std::vector<u8>& input) {
    std::vector<u8> padded = padBytes(input);
    std::array<u32, 8> state = IV;

    for (std::size_t block = 0; block < padded.size(); block += 64) {
        std::array<u32, 16> words{};
        for (int i = 0; i < 16; ++i) {
            std::size_t idx = block + i * 4;
            words[i] = (u32(padded[idx]) |
                        (u32(padded[idx + 1]) << 8) |
                        (u32(padded[idx + 2]) << 16) |
                        (u32(padded[idx + 3]) << 24));
        }

        std::array<u32, 8> mixed = mixBlock(state, words);
        for (int i = 0; i < 8; ++i) {
            state[i] = mixed[i] ^ rotl32(state[(i + 4) % 8] + words[(i * 3 + 1) % 16] + 0xA5A5A5A5u, 17 + i);
        }
    }

    std::vector<u8> digest(32, 0);
    for (int i = 0; i < 8; ++i) {
        u32 word = state[i];
        digest[i * 4 + 0] = static_cast<u8>(word & 0xFFu);
        digest[i * 4 + 1] = static_cast<u8>((word >> 8) & 0xFFu);
        digest[i * 4 + 2] = static_cast<u8>((word >> 16) & 0xFFu);
        digest[i * 4 + 3] = static_cast<u8>((word >> 24) & 0xFFu);
    }
    return digest;
}

std::string bytesToHex(const std::vector<u8>& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (u8 b : bytes) {
        oss << std::setw(2) << static_cast<unsigned int>(b);
    }
    return oss.str();
}

std::string hashTextUtf8(const std::string& text) {
    return bytesToHex(hashData(toBytes(text)));
}

// ---------------------------------------------------------------------------
// v0.3: srautine sasaja. Viskas, kas auksciau, yra nepakeistas 1 uzduoties kodas.
// Zemiau esantis kodas naudoja ta pacia mixBlock funkcija ir ta pati IV, todel
// duoda tuos pacius rezultatus. Tai tikrina tests/tests_v03.cpp.
// ---------------------------------------------------------------------------

// Vienas 64 baitu blokas: tie patys veiksmai kaip hashData ciklo viduje.
static void absorbBlock(std::array<u32, 8>& state, const u8* block) {
    std::array<u32, 16> words{};
    for (int i = 0; i < 16; ++i) {
        const u8* p = block + i * 4;
        words[i] = (u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24));
    }

    std::array<u32, 8> mixed = mixBlock(state, words);
    for (int i = 0; i < 8; ++i) {
        state[i] = mixed[i] ^ rotl32(state[(i + 4) % 8] + words[(i * 3 + 1) % 16] + 0xA5A5A5A5u, 17 + i);
    }
}

HashStream::HashStream() : state(IV), buffer{}, buffered(0), total_bytes(0) {}

void HashStream::update(const char* data, std::size_t size) {
    total_bytes += size;
    const u8* bytes = reinterpret_cast<const u8*>(data);

    // Pirma uzpildomas anksciau pradetas blokas.
    if (buffered > 0) {
        while (size > 0 && buffered < 64) {
            buffer[buffered++] = *bytes++;
            --size;
        }
        if (buffered < 64) {
            return;
        }
        absorbBlock(state, buffer.data());
        buffered = 0;
    }

    while (size >= 64) {
        absorbBlock(state, bytes);
        bytes += 64;
        size -= 64;
    }

    while (size > 0) {
        buffer[buffered++] = *bytes++;
        --size;
    }
}

HashStream::Digest HashStream::digest() const {
    // Dirbama su kopijomis, kad objekta butu galima naudoti toliau.
    std::array<u32, 8> final_state = state;

    // Uzpildymas kaip padBytes: 0x80, nuliai iki 56 (mod 64), tada 8 baitai ilgio bitais.
    std::array<u8, 128> tail{};
    std::size_t length = buffered;
    for (std::size_t i = 0; i < buffered; ++i) {
        tail[i] = buffer[i];
    }
    tail[length++] = 0x80u;
    const std::size_t padded = (length <= 56) ? 64 : 128;
    const u64 bits = total_bytes * 8ULL;
    for (int i = 0; i < 8; ++i) {
        tail[padded - 8 + i] = static_cast<u8>((bits >> (56 - 8 * i)) & 0xFFu);
    }

    absorbBlock(final_state, tail.data());
    if (padded == 128) {
        absorbBlock(final_state, tail.data() + 64);
    }

    Digest out{};
    for (int i = 0; i < 8; ++i) {
        const u32 word = final_state[i];
        out[i * 4 + 0] = static_cast<u8>(word & 0xFFu);
        out[i * 4 + 1] = static_cast<u8>((word >> 8) & 0xFFu);
        out[i * 4 + 2] = static_cast<u8>((word >> 16) & 0xFFu);
        out[i * 4 + 3] = static_cast<u8>((word >> 24) & 0xFFu);
    }
    return out;
}

std::string digest_to_hex(const HashStream::Digest& digest) {
    static const char digits[] = "0123456789abcdef";
    std::string hex(64, '0');
    for (std::size_t i = 0; i < digest.size(); ++i) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 0x0Fu];
    }
    return hex;
}

std::string HashStream::hex() const {
    return digest_to_hex(digest());
}

bool has_leading_zero_nibbles(const HashStream::Digest& digest, std::uint32_t count) {
    if (count > 64) {
        return false;
    }
    for (std::uint32_t i = 0; i < count; ++i) {
        const u8 byte = digest[i / 2];
        const u8 nibble = (i % 2 == 0) ? static_cast<u8>(byte >> 4) : static_cast<u8>(byte & 0x0Fu);
        if (nibble != 0) {
            return false;
        }
    }
    return true;
}
