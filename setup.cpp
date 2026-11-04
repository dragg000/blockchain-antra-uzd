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

    for (size_t i = 0; i < user_count; ++i)
    {
        string name = "user" + std::to_string(i);
        Hash public_key = hashTextUtf8(name);

        if (!used_keys.insert(public_key).second)
        {
            throw std::runtime_error("Duplicate public key for " + name);
        }

        uint64_t balance = min_balance + rng() % (max_balance - min_balance + 1);

        users.emplace_back(name, public_key, balance);
    }

    return users;
}

std::vector<Transaction> generate_transactions(uint64_t seed, std::vector<User>& users)
{
    uint64_t minimum_transaction = 1;
    std::vector<Transaction> transactions;
    const uint64_t transaction_count = 10000;
    transactions.reserve(transaction_count);

    std::mt19937_64 rng(seed);

    if (users.size() < 2)
    {
        throw std::runtime_error("Need at least two users to generate transactions");
    }

    uint64_t sequence = 0;
    for(size_t i = 0; i < transaction_count; i++)
    {
        size_t sender_index = rng() % users.size();
        size_t receiver_index = rng() % users.size();
        while (receiver_index == sender_index)
        {
            receiver_index = rng() % users.size();
        }

        User& sender = users[sender_index];
        User& receiver = users[receiver_index];

        Hash sender_key = sender.get_key();
        Hash receiver_key = receiver.get_key();
        uint64_t maximum_transaction = sender.get_balance() / 10;
        uint64_t amount = minimum_transaction + rng() % maximum_transaction;

        Transaction transaction(sender_key, receiver_key, amount, "", sequence);
        transaction.set_transaction_id(hashTextUtf8(transaction.serialize()));

        transactions.push_back(transaction);
        sequence++;
    }

    return transactions;
}

