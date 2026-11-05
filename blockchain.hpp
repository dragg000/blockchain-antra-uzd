#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include <vector>
#include "block_h.hpp"

class Blockchain
{
private:
    std::vector<Block> blocks; // blocks[0] is genesis
    uint32_t difficulty;       // the chain sets this, not the candidate

    void create_genesis_block();

public:
    Blockchain(uint32_t difficulty); // creates genesis

    const Block &tip() const; // the newest block
    const Block &get_block(size_t height) const { return blocks.at(height); }
    size_t height() const;
    uint32_t get_difficulty() const;
    const vector<Block> get_blocks() { return blocks }

    bool add_block(const Block &block); // checks first, then appends

    // mines one block on top of the tip, without adding it to the chain
    Block minePendingTransactions(const std::vector<Transaction> &selected,
                                  uint64_t &attempts,
                                  double &elapsed_seconds) const;
};

#endif