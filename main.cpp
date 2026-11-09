#include "blockchain.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include "balance_state.hpp"
#include "mining.hpp"
#include "validation.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>

int main(int argc, char *argv[])
{
    const uint64_t user_seed = 1;
    const uint64_t transaction_seed = 2;
    const uint64_t selection_seed = 3;
    const uint32_t difficulty = 3;
    const uint64_t transaction_count = argc > 1 ? std::stoull(argv[1]) : 10000;

    std::vector<User> users = generate_users(user_seed);
    std::vector<Transaction> generated = generate_transactions(transaction_seed, users, transaction_count);
    std::vector<Transaction> pool;
    std::vector<RefusedTransaction> refused;
    std::set<Hash> pool_ids;
    std::mt19937_64 rng(selection_seed);

    BalanceState balances(users);
    Blockchain bakugan(difficulty);

    for (const Transaction &transaction : generated)
    {
        std::string error = add_transaction_to_pool(pool, pool_ids, transaction, balances, bakugan.get_transaction_ids());
        if (!error.empty())
        {
            refused.push_back({transaction, error});
        }
    }

    auto start = std::chrono::steady_clock::now();
    MiningResult result;
    try
    {
        result = run_mining(pool, balances, bakugan, rng, true);
    }
    catch (const std::runtime_error &error)
    {
        // Pvz. joks kandidatas nerado nonce per leistinas eiles. Jau priimti blokai lieka galioti,
        // o neuzbaigtas blokas nieko nepakeite.
        std::cout << "\nKasimas sustabdytas: " << error.what() << "\n"
                  << "  priimti blokai:        " << bakugan.height() << "\n"
                  << "  likusios transakcijos: " << pool.size() << "\n";
        return 1;
    }
    auto end = std::chrono::steady_clock::now();

    std::cout << "\nSuvestine\n"
              << "  vartotojai:             " << users.size() << "\n"
              << "  transakcijos:           " << transaction_count << "\n"
              << "  priimti blokai:         " << bakugan.height() << "\n"
              << "  itrauktos transakcijos: " << result.included << "\n"
              << "  atmestos pries fonda:   " << refused.size() << "\n"
              << "  atmestos atrenkant:     " << result.rejected.size() << "\n"
              << "  likusios transakcijos:  " << pool.size() << "\n"
              << "  pradine lesu suma:      " << balances.get_initial_total() << "\n"
              << "  galutine lesu suma:     " << balances.sum() << "\n"
              << "  bendras laikas:         " << std::chrono::duration<double>(end - start).count() << " s\n";

    for (const RefusedTransaction &item : refused)
    {
        std::cout << "Neileista i fonda " << item.transaction.get_transaction_id()
                  << " (suma " << item.transaction.get_amount() << "): " << item.reason << "\n";
    }

    for (const Transaction &transaction : result.rejected)
    {
        std::cout << "Atmesta " << transaction.get_transaction_id()
                  << " (suma " << transaction.get_amount() << "): "
                  << (balances.has_user(transaction.get_sender()) && balances.has_user(transaction.get_receiver())
                          ? "siuntejui nepakako lesu"
                          : "nezinomas dalyvis")
                  << "\n";
    }

    return 0;
}
