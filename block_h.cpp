#include "block_h.hpp"
#include "hash.hpp"
#include <limits>
#include <sstream>
#include <stdexcept>

std::string hexToBytes(const std::string &hex);

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
    // Be biudzeto: tas pats ciklas, tik riba yra didziausias galimas skaicius.
    mine(std::numeric_limits<uint64_t>::max(), attempts, elapsed_seconds);
}

// Tikrina daugiausia `budget` nonce reiksmiu, pradedant nuo dabartines.
// Radus tinkama: grazina true, antrasteje lieka tinkamas nonce.
// Neradus: grazina false, antrasteje lieka pirmas dar netikrintas nonce,
// todel kitas kvietimas tesia paieska ir nieko netikrina du kartus.
// `attempts` visada lygus atliktu maisos skaiciavimu skaiciui.
bool Block::mine(uint64_t budget, uint64_t &attempts, double &elapsed_seconds)
{
    auto start = std::chrono::steady_clock::now();

    bool found = false;
    attempts = 0;
    while (attempts < budget)
    {
        attempts++;
        if (hasValidProofOfWork())
        {
            found = true;
            break;
        }
        header.set_nonce(header.get_nonce() + 1);
    }

    elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return found;
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

        for (size_t i = 0; i < level.size(); i += 2)
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