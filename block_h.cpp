#include "block_h.hpp"
#include "hash.hpp"
#include <bits/stdc++.h>

Hash Block::calculateHash() const
{
    const std::string data =
        header.get_prev_hash() + "|" +
        header.get_tx_root() + "|" +
        std::to_string(header.get_timestamp()) + "|" +
        std::to_string(header.get_version()) + "|" +
        std::to_string(header.get_nonce()) + "|" +
        std::to_string(header.get_difficulty());

    return hashTextUtf8(data);
}

bool Block::hasValidProofOfWork() const
{
    Hash hash = calculateHash();

    std::string target(header.get_difficulty(), '0');

    return hash.substr(0, target.size()) == target;
}

void Block::mine(uint64_t &attempts, double &elapsed_seconds)
{
    uint64_t count = 0;

    auto start = std::chrono::steady_clock::now();

    while (!hasValidProofOfWork())
    {
        header.set_nonce(header.get_nonce() + 1);
        count++;
    }
    auto end = std::chrono::steady_clock::now();

    attempts = count + 1;

    elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
}

Hash Block::calculateTxRoot() const
{

    std::vector<Hash> level;

    if (transactions.empty())
        return hashTextUtf8("");

    for (const Transaction &tx : transactions)
    {
        level.push_back(tx.get_transaction_id());
    }

    while (level.size() > 1)
    {

        std::vector<Hash> next_level;

        if (level.size() % 2 == 1)
            level.push_back(level.back());

        for (int i = 0; i < level.size(); i += 2)
        {

            string left = hexToBytes(level[i]);
            string right = hexToBytes(level[i + 1]);

            Hash parent = hashTextUtf8(left + right);

            next_level.push_back(parent);
        }
        level = next_level;
    }
    return level[0];
}

std::string hexToBytes(const std::string &hex)
{
    if (hex.size() % 2 != 0)
        throw std::invalid_argument("Invalid hex length");

    std::string bytes;

    for (size_t i = 0; i < hex.size(); i += 2)
    {
        unsigned int byte;

        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> byte;

        if (ss.fail())
            throw std::invalid_argument("Invalid hex character");

        bytes.push_back(static_cast<char>(byte));
    }

    return bytes;
}