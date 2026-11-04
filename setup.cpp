#include "user.hpp"
#include "hash.hpp"
#include "transaction.hpp"
#include <random>
#include <set>
#include <stdexcept>

std::vector<User> generate_users(uint64_t seed)
{
    const int user_count = 1000;
    const uint64_t min_balance = 100;
    const uint64_t max_balance = 1000000;

    std::mt19937_64 rng(seed);
    std::vector<User> users;
    users.reserve(user_count);
    std::set<Hash> used_keys;

    for (int i = 0; i < user_count; ++i)
    {
        string name = "user" + std::to_string(i);
        Hash public_key = hashTextUtf8(name);

        if (!used_keys.insert(public_key).second)
        {
            throw std::runtime_error("Duplicate public key for " + name);
        }

        // rng() gives the same numbers on every platform for the same seed.
        // std::uniform_int_distribution doesn't, so it isn't used here.
        uint64_t balance = min_balance + rng() % (max_balance - min_balance + 1);

        users.emplace_back(name, public_key, balance);
    }

    return users;
}

std::vector<Transaction> generate_transactions(uint64_t seed)
{
    std::vector<Transaction> transactions;
    transactions.reserve(10000);

    std::vector<User> users = generate_users(seed);


}

