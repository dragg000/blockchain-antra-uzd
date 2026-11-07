#pragma once

#include "balance_state.hpp"
#include "blockchain.hpp"
#include <cstddef>
#include <random>
#include <vector>

struct Selection
{
    std::vector<Transaction> selected;
    std::vector<Transaction> rejected;
};

struct MiningResult
{
    std::vector<Transaction> rejected;
    size_t included = 0;
};

Selection select_transaction(const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids);

void remove_from_pool(std::vector<Transaction> &pool, const std::vector<Transaction> &to_remove);

MiningResult run_mining(std::vector<Transaction> &pool, BalanceState &balances, Blockchain &chain,
                        std::mt19937_64 &rng, bool print_blocks);
