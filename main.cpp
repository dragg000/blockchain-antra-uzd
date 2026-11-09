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

// Priima tik skaitmenis, nuo 1 iki 1 000 000. std::stoull priimtu ir "-5" ar "12abc".
static bool parse_transaction_count(const std::string &text, uint64_t &count)
{
    if (text.empty() || text.size() > 7)
        return false;

    uint64_t value = 0;
    for (char c : text)
    {
        if (c < '0' || c > '9')
            return false;
        value = value * 10 + static_cast<uint64_t>(c - '0');
    }

    if (value == 0 || value > 1000000)
        return false;

    count = value;
    return true;
}

int main(int argc, char *argv[])
{
    const uint64_t user_seed = 1;
    const uint64_t transaction_seed = 2;
    const uint64_t selection_seed = 3;
    const uint32_t difficulty = 3;
    uint64_t transaction_count = 10000;
    if (argc > 1 && !parse_transaction_count(argv[1], transaction_count))
    {
        std::cout << "Netinkamas transakciju skaicius: " << argv[1] << "\n"
                  << "Naudojimas: ./blockchain [transakciju skaicius nuo 1 iki 1000000]\n";
        return 2;
    }

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

    std::cout << "\nKandidatai\n"
              << "  kandidatu vienam blokui: " << candidates_per_block << "\n"
              << "  pradinis biudzetas:      " << candidate_start_budget << " bandymu per eile\n"
              << "  visu kandidatu bandymai: " << result.candidate_attempts << "\n"
              << "  visu kandidatu laikas:   " << result.candidate_seconds << " s\n"
              << "  biudzetas dvigubintas:   " << result.budget_raises << " k.\n"
              << "  laimeti blokai:         ";
    for (size_t i = 0; i < result.wins.size(); i++)
    {
        std::cout << " nr " << i + 1 << ": " << result.wins[i] << (i + 1 < result.wins.size() ? "," : "\n");
    }
    std::cout << "\n";

    for (const RefusedTransaction &item : refused)
    {
        std::cout << "Neileista i fonda " << item.transaction.get_transaction_id()
                  << " (suma " << item.transaction.get_amount() << "): " << item.reason << "\n";
    }

    for (size_t i = 0; i < result.rejected.size(); i++)
    {
        const Transaction &transaction = result.rejected[i];
        std::cout << "Atmesta " << transaction.get_transaction_id()
                  << " (suma " << transaction.get_amount() << "): " << result.reject_reasons[i] << "\n";
    }

    return 0;
}
