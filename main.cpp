#include "blockchain.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include "balance_state.hpp"
#include <random>

struct Selection
{
    std::vector<Transaction> selected;
    std::vector<Transaction> set_aside;
};

Selection select_transaction(const std::vector<Transaction>& pool, const BalanceState& balances, std::mt19937_64& rng)
{
    Selection selection;
    BalanceState tmp = balances;
    std::vector<Transaction> remaining = pool;

    while(selection.selected.size() < 100 && remaining.empty() == false)
    {
        size_t position = rng() % remaining.size();
        Transaction transaction = remaining[position];
        remaining.erase(remaining.begin() + position);
        if(tmp.apply(transaction) == false)
            selection.set_aside.push_back(transaction);
        else
            selection.selected.push_back(transaction);
    }
    return selection;
}

int main()
{
    const uint64_t user_seed = 1;
    const uint64_t transaction_seed = 2;
    const uint64_t selection_seed = 3;
    const uint64_t transaction_count = 10000;


    std::vector<User> users = generate_users(user_seed);
    std::vector<Transaction> pool = generate_transactions(transaction_seed, users, transaction_count);
    std::mt19937_64 rng(selection_seed);

    BalanceState balances(users);
    Blockchain bakugan(3);
    
    Selection selection = select_transaction(pool, balances, rng);

    return 0;
}