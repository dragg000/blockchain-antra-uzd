#include "block_h.hpp"
#include "hash.hpp"

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
    std::string combined;

    for (const Transaction &tx : transactions)
    {
        combined += tx.get_transaction_id();
    }

    return hashTextUtf8(combined);
}