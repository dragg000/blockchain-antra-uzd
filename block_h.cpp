#include "block_h.hpp"
#include "hash.hpp"

Hash Block::calculateHash() const
{
    const std::string data =
        header.get_prev_hash() +
        header.get_tx_root() +
        std::to_string(header.get_timestamp()) +
        std::to_string(header.get_version()) +
        std::to_string(header.get_nonce()) +
        std::to_string(header.get_difficulty());

    return hashTextUtf8(data);
}

bool Block::hasValidProofOfWork() const
{
    Hash hash = calculateHash();

    std::string target(header.get_difficulty(), '0');

    return hash.substr(0, target.size()) == target;
}