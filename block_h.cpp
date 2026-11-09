#include "block_h.hpp"
#include "hash.hpp"
#include "merkle.hpp"
#include <limits>

// Antraste serializuojama kaip prev_hash|tx_root|timestamp|version|nonce|difficulty.
// Kasant keiciasi tik nonce, todel eilute padalyta i dali iki nonce ir dali po jo.
std::string Block::header_before_nonce() const
{
    return header.get_prev_hash() + "|" +
           header.get_tx_root() + "|" +
           std::to_string(header.get_timestamp()) + "|" +
           std::to_string(header.get_version()) + "|";
}

std::string Block::header_after_nonce() const
{
    return "|" + std::to_string(header.get_difficulty());
}

Hash Block::calculateHash() const
{
    return hashTextUtf8(header_before_nonce() + std::to_string(header.get_nonce()) + header_after_nonce());
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

    // Antrastes dalis iki nonce nesikeicia, todel ji sumaisoma viena karta.
    // Kiekvienam nonce kopijuojama si tarpine busena ir pridedama tik "nonce|difficulty".
    HashStream before_nonce;
    before_nonce.update(header_before_nonce());
    const std::string after_nonce = header_after_nonce();
    const uint32_t difficulty = header.get_difficulty();

    bool found = false;
    attempts = 0;
    uint64_t nonce = header.get_nonce();
    while (attempts < budget)
    {
        attempts++;

        HashStream stream = before_nonce;
        stream.update(std::to_string(nonce) + after_nonce);
        if (has_leading_zero_nibbles(stream.digest(), difficulty))
        {
            found = true;
            break;
        }
        nonce++;
    }
    header.set_nonce(nonce);

    elapsed_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return found;
}

Hash Block::calculateTxRoot() const
{
    std::vector<Hash> ids;
    ids.reserve(transactions.size());
    for (const Transaction &transaction : transactions)
    {
        ids.push_back(transaction.get_transaction_id());
    }
    return merkle_root(ids);
}
