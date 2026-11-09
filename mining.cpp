#include "mining.hpp"
#include "validation.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

Selection select_transaction(const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids)
{
    Selection selection;
    BalanceState tmp = balances;
    std::vector<Transaction> remaining = pool;
    std::set<Hash> candidate_ids = known_ids;

    while (selection.selected.size() < 100 && remaining.empty() == false)
    {
        size_t position = rng() % remaining.size();
        Transaction transaction = remaining[position];
        remaining.erase(remaining.begin() + position);

        std::string error = validate_transaction(tmp, transaction, candidate_ids);

        if (!error.empty())
        {
            selection.rejected.push_back(transaction);
            continue;
        }

        if (tmp.apply(transaction) == false)
        {
            selection.rejected.push_back(transaction);
            continue;
        }

        selection.selected.push_back(transaction);

        candidate_ids.insert(transaction.get_transaction_id());
    }
    return selection;
}

std::vector<Selection> build_candidates(size_t candidate_count, const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng,const std::set<Hash> &known_ids)
{
    std::vector<Selection> candidate_selections;
    for(size_t i = 0; i < candidate_count; i++)
        candidate_selections.push_back(select_transaction(pool, balances, rng, known_ids));

    return candidate_selections;
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

MiningResult run_mining(
    std::vector<Transaction> &pool,
    BalanceState &balances,
    Blockchain &chain,
    std::mt19937_64 &rng,
    bool print_blocks)
{
    MiningResult result;

    while (!pool.empty())
    {
        Selection selection = select_transaction(
            pool,
            balances,
            rng,
            chain.get_transaction_ids());

        if (selection.selected.empty())
        {
            result.rejected.insert(
                result.rejected.end(),
                selection.rejected.begin(),
                selection.rejected.end());

            remove_from_pool(pool, selection.rejected);

            break;
        }

        uint64_t attempts = 0;
        double seconds = 0.0;

        Block block = chain.minePendingTransactions(
            selection.selected,
            attempts,
            seconds);

        // Validate and accept block, including balance updates
        if (!chain.add_block(block, balances))
        {
            throw std::runtime_error(
                "Grandine atmete iskasta bloka");
        }

        result.included += selection.selected.size();

        result.rejected.insert(
            result.rejected.end(),
            selection.rejected.begin(),
            selection.rejected.end());

        remove_from_pool(pool, selection.selected);
        remove_from_pool(pool, selection.rejected);

        if (print_blocks)
            print_block(chain, attempts, seconds);
    }

    return result;
}
