#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include <vector>
#include "block_h.hpp"
#include "balance_state.hpp"
#include <set>

class Blockchain
{
private:
    std::vector<Block> blocks; // blocks[0] is genesis
    uint32_t difficulty;       // the chain sets this, not the candidate
    std::set<Hash> known_transaction_ids;

    void create_genesis_block();

public:
    Blockchain(uint32_t difficulty); // creates genesis

    const Block &tip() const; // the newest block
    const Block &get_block(size_t height) const { return blocks.at(height); }
    size_t height() const;
    uint32_t get_difficulty() const;
    const std::vector<Block> &get_blocks() const { return blocks; }
    const std::set<Hash> &get_transaction_ids() const { return known_transaction_ids; }

    bool add_block(const Block &block, BalanceState &balances);

    // mines one block on top of the tip, without adding it to the chain
    Block minePendingTransactions(const std::vector<Transaction> &selected,
                                  uint64_t &attempts,
                                  double &elapsed_seconds) const;
};

#endif