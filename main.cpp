#include "blockchain.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include "balance_state.hpp"
#include "mining.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <string>

int main(int argc, char *argv[])
{
    const uint64_t user_seed = 1;
    const uint64_t transaction_seed = 2;
    const uint64_t selection_seed = 3;
    const uint32_t difficulty = 3;
    const uint64_t transaction_count = argc > 1 ? std::stoull(argv[1]) : 10000;

    std::vector<User> users = generate_users(user_seed);
    std::vector<Transaction> pool = generate_transactions(transaction_seed, users, transaction_count);
    std::mt19937_64 rng(selection_seed);

    BalanceState balances(users);
    Blockchain bakugan(difficulty);

    auto start = std::chrono::steady_clock::now();
    MiningResult result = run_mining(pool, balances, bakugan, rng, true);
    auto end = std::chrono::steady_clock::now();

    std::cout << "\nSuvestine\n"
              << "  vartotojai:             " << users.size() << "\n"
              << "  transakcijos:           " << transaction_count << "\n"
              << "  priimti blokai:         " << bakugan.height() << "\n"
              << "  itrauktos transakcijos: " << result.included << "\n"
              << "  atmestos transakcijos:  " << result.rejected.size() << "\n"
              << "  likusios transakcijos:  " << pool.size() << "\n"
              << "  pradine lesu suma:      " << balances.get_initial_total() << "\n"
              << "  galutine lesu suma:     " << balances.sum() << "\n"
              << "  bendras laikas:         " << std::chrono::duration<double>(end - start).count() << " s\n";

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
