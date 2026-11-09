#include "user.hpp"
#include "hash.hpp"
#include "transaction.hpp"
#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>

//headeriai user.hpp ir transaction.hpp

std::vector<User> generate_users(uint64_t seed)
{
    const int user_count = 1000;
    const Bakugan min_balance = 100;
    const Bakugan max_balance = 1000000;

    std::mt19937_64 rng(seed);
    std::vector<User> users;
    users.reserve(user_count);
    std::set<Key> used_keys;

    for (size_t i = 0; i < user_count; ++i)
    {
        string name = "user" + std::to_string(i);
        Key public_key = hashTextUtf8(name);

        if (!used_keys.insert(public_key).second)
        {
            throw std::runtime_error("Duplicate public key for " + name);
        }

        uint64_t balance = min_balance + rng() % (max_balance - min_balance + 1);

        users.emplace_back(name, public_key, balance);
    }

    return users;
}

std::vector<Transaction> generate_transactions(uint64_t seed, const std::vector<User>& users, uint64_t transaction_count)
{
    uint64_t minimum_transaction = 1;
    std::vector<Transaction> transactions;
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

        const User& sender = users[sender_index];
        const User& receiver = users[receiver_index];

        Hash sender_key = sender.get_key();
        Hash receiver_key = receiver.get_key();
        // Bent 1, kad siuntejui su balansu iki 9 nebutu dalijama is nulio.
        uint64_t maximum_transaction = std::max<uint64_t>(1, sender.get_balance() / 10);
        uint64_t amount = minimum_transaction + rng() % maximum_transaction;

        Transaction transaction(sender_key, receiver_key, amount, "", sequence);
        transaction.set_transaction_id(hashTextUtf8(transaction.serialize()));

        transactions.push_back(transaction);
        sequence++;
    }

    return transactions;
}

