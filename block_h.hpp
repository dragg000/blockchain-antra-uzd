#ifndef BLOCK_H_HPP
#define BLOCK_H_HPP

#include <string>
#include <cstdint>
#include <vector>
#include "transaction.hpp"


class Block_h
{
private:
    Hash prev_hash;
    Hash tx_root;

    uint64_t timestamp;
    uint64_t nonce;

    uint32_t version;
    uint32_t difficulty;

public:
    Block_h(
        Hash prev_hash,
        Hash tx_root,
        uint64_t timestamp,
        uint32_t version,
        uint32_t difficulty,
        uint64_t nonce)
    {
        this->prev_hash = prev_hash;
        this->tx_root = tx_root;
        this->timestamp = timestamp;
        this->version = version;
        this->difficulty = difficulty;
        this->nonce = nonce;
    }

    void set_prev_hash(Hash prev_hash) { this->prev_hash = prev_hash; }
    void set_tx_root(Hash tx_root) { this->tx_root = tx_root; }
    void set_timestamp(uint64_t timestamp) { this->timestamp = timestamp; }
    void set_version(uint32_t version) { this->version = version; }
    void set_difficulty(uint32_t difficulty) { this->difficulty = difficulty; }
    void set_nonce(uint64_t nonce) { this->nonce = nonce; }

    Hash get_prev_hash() const { return prev_hash; }
    Hash get_tx_root() const { return tx_root; }
    uint64_t get_timestamp() const { return timestamp; }
    uint32_t get_version() const { return version; }
    uint32_t get_difficulty() const { return difficulty; }
    uint64_t get_nonce() const { return nonce; }
};

class Block
{
private:
    Block_h header;
    std::vector<Transaction> transactions;

public:
    Block(Block_h header, std::vector<Transaction> transactions)
        : header(header), transactions(transactions) {}

    Block_h get_header() const { return header; }
    std::vector<Transaction> get_transactions() const { return transactions; }
};

#endif