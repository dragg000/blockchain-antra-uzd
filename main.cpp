#include "blockchain.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include "balance_state.hpp"
#include <random>
#include <set>
#include <algorithm>
#include <stdexcept>

struct Selection
{
    std::vector<Transaction> selected;
    std::vector<Transaction> rejected;
};

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

int main()
{
    const uint64_t user_seed = 1;
    const uint64_t transaction_seed = 2;
    const uint64_t selection_seed = 3;
    const uint64_t transaction_count = 10000;

    std::vector<User> users = generate_users(user_seed);
    std::vector<Transaction> pool = generate_transactions(transaction_seed, users, transaction_count);
    std::vector<Transaction> rejected;
    std::mt19937_64 rng(selection_seed);

    BalanceState balances(users);
    Blockchain bakugan(3);

    while(!pool.empty())
    {
        Selection selection = select_transaction(pool, balances, rng);
        if(selection.selected.empty())
        {
            rejected.insert(rejected.end(), selection.rejected.begin(), selection.rejected.end());
        remove_from_pool(pool, selection.rejected);
        continue;
        }
        uint64_t attempts = 0;
        double seconds = 0.0;
        Block block = bakugan.minePendingTransactions(selection.selected,
            attempts, seconds);

        BalanceState balances_copy = balances;
        for(const Transaction &transaction : block.get_transactions())
        {
            if(!balances_copy.apply(transaction))
            {
                throw std::runtime_error("Iškastas blokas turi netinkama transakcija");
            }
        }
        
        if(!bakugan.add_block(block))
        {
            throw std::runtime_error("Grandine atmete iskasta bloka");
        }

        balances = balances_copy;
        rejected.insert(rejected.end(), selection.rejected.begin(), selection.rejected.end());
    remove_from_pool(pool, selection.selected);
    remove_from_pool(pool, selection.rejected);
    }
    return 0;
}