#include "mining.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

Selection select_transaction(const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng)
{
    Selection selection;
    BalanceState tmp = balances;
    std::vector<Transaction> remaining = pool;

    while (selection.selected.size() < 100 && remaining.empty() == false)
    {
        size_t position = rng() % remaining.size();
        Transaction transaction = remaining[position];
        remaining.erase(remaining.begin() + position);
        if (tmp.apply(transaction) == false)
            selection.rejected.push_back(transaction);
        else
            selection.selected.push_back(transaction);
    }
    return selection;
}

void remove_from_pool(std::vector<Transaction> &pool, const std::vector<Transaction> &to_remove)
{
    std::set<Hash> ids;
    for (const Transaction &transaction : to_remove)
    {
        ids.insert(transaction.get_transaction_id());
    }

    pool.erase(std::remove_if(pool.begin(), pool.end(),
                              [&ids](const Transaction &transaction)
                              {
                                  return ids.count(transaction.get_transaction_id()) > 0;
                              }),
               pool.end());
}

static void print_block(const Blockchain &chain, uint64_t attempts, double seconds)
{
    const Block &block = chain.tip();
    const Block_h header = block.get_header();

    std::cout << "Blokas #" << chain.height() << "\n"
              << "  transakcijos:  " << block.get_transactions().size() << "\n"
              << "  prev_hash:     " << header.get_prev_hash() << "\n"
              << "  tx_root:       " << header.get_tx_root() << "\n"
              << "  hash:          " << block.calculateHash() << "\n"
              << "  difficulty:    " << header.get_difficulty() << "\n"
              << "  nonce:         " << header.get_nonce() << "\n"
              << "  bandymai:      " << attempts << "\n"
              << "  kasimo laikas: " << seconds << " s\n";
}

MiningResult run_mining(std::vector<Transaction> &pool, BalanceState &balances, Blockchain &chain,
                        std::mt19937_64 &rng, bool print_blocks)
{
    MiningResult result;

    while (!pool.empty())
    {
        Selection selection = select_transaction(pool, balances, rng);
        if (selection.selected.empty())
        {
            result.rejected.insert(result.rejected.end(), selection.rejected.begin(), selection.rejected.end());
            remove_from_pool(pool, selection.rejected);
            continue;
        }

        uint64_t attempts = 0;
        double seconds = 0.0;
        Block block = chain.minePendingTransactions(selection.selected, attempts, seconds);

        BalanceState balances_copy = balances;
        for (const Transaction &transaction : block.get_transactions())
        {
            if (!balances_copy.apply(transaction))
            {
                throw std::runtime_error("Iškastas blokas turi netinkama transakcija");
            }
        }

        if (!chain.add_block(block))
        {
            throw std::runtime_error("Grandine atmete iskasta bloka");
        }

        balances = balances_copy;
        result.included += selection.selected.size();
        result.rejected.insert(result.rejected.end(), selection.rejected.begin(), selection.rejected.end());
        remove_from_pool(pool, selection.selected);
        remove_from_pool(pool, selection.rejected);

        if (print_blocks)
        {
            print_block(chain, attempts, seconds);
        }
    }

    return result;
}
