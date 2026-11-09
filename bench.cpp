// Spartos matavimas. Tas pats failas naudojamas v0.2 ir v0.3 versijoms palyginti,
// todel jis kviecia tik tas funkcijas, kurios yra abiejose versijose.
//
// Kiekvienas matavimas kartojamas `runs` kartu (numatyta 7), spausdinamas vidurkis ir
// standartinis nuokrypis. Laikas matuojamas tik aplink skaiciavima, isvedimas vyksta po jo.
//
//   ./run_bench [runs]

#include "balance_state.hpp"
#include "blockchain.hpp"
#include "mining.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include "validation.hpp"
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

static double seconds_since(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

static void report(const std::string &name, const std::vector<double> &times, const std::string &note)
{
    double mean = 0.0;
    double lowest = times[0];
    double highest = times[0];
    for (double time : times)
    {
        mean += time;
        lowest = std::min(lowest, time);
        highest = std::max(highest, time);
    }
    mean /= times.size();

    double variance = 0.0;
    for (double time : times)
    {
        variance += (time - mean) * (time - mean);
    }
    const double deviation = times.size() > 1 ? std::sqrt(variance / (times.size() - 1)) : 0.0;

    std::cout << std::fixed << std::setprecision(6)
              << name << "\n"
              << "  kartai:    " << times.size() << "\n"
              << "  vidurkis:  " << mean << " s\n"
              << "  nuokrypis: " << deviation << " s\n"
              << "  min / max: " << lowest << " / " << highest << " s\n"
              << "  pastaba:   " << note << "\n";
}

int main(int argc, char *argv[])
{
    const size_t runs = argc > 1 ? std::stoul(argv[1]) : 7;

    const uint64_t user_seed = 1;
    const uint64_t transaction_seed = 2;
    const uint64_t selection_seed = 3;
    const uint32_t difficulty = 3;
    const uint64_t transaction_count = 10000;

    std::vector<User> users = generate_users(user_seed);
    const std::vector<Transaction> generated = generate_transactions(transaction_seed, users, transaction_count);
    const std::vector<Transaction> first_hundred(generated.begin(), generated.begin() + 100);

    std::cout << "Matavimo salygos: " << users.size() << " vartotoju, " << generated.size()
              << " transakciju, seed " << user_seed << "/" << transaction_seed << "/" << selection_seed
              << ", sudetingumas " << difficulty << ", kartojimu " << runs << "\n\n";

    // 1. Kasimas: fiksuota antraste, fiksuotas bandymu skaicius. Sudetingumas 16 praktiskai
    //    nepasiekiamas, todel kiekviena karta atliekama lygiai tiek pat bandymu.
    {
        const uint64_t attempts_per_run = 200000;
        std::vector<double> times;
        uint64_t done = 0;
        for (size_t run = 0; run < runs; run++)
        {
            Block block(Block_h(std::string(64, '0'), "", 1001, 2, 16, 0), first_hundred);
            uint64_t attempts = 0;
            double seconds = 0.0;
            auto start = std::chrono::steady_clock::now();
            block.mine(attempts_per_run, attempts, seconds);
            times.push_back(seconds_since(start));
            done = attempts;
        }
        report("1. Kasimas (fiksuotas bandymu skaicius)", times,
               std::to_string(done) + " bandymu per karta, viena antraste, sudetingumas 16");
    }

    // 2. Merkle saknis 100 transakciju blokui.
    {
        const size_t repeats = 500;
        std::vector<double> times;
        Hash root;
        const Block block(Block_h(std::string(64, '0'), "", 1001, 2, difficulty, 0), first_hundred);
        for (size_t run = 0; run < runs; run++)
        {
            auto start = std::chrono::steady_clock::now();
            for (size_t i = 0; i < repeats; i++)
            {
                root = block.calculateTxRoot();
            }
            times.push_back(seconds_since(start));
        }
        report("2. Merkle saknis", times,
               std::to_string(repeats) + " skaiciavimu per karta, 100 transakciju, saknis " + root);
    }

    // 3. Transakciju tikrinimas pries fonda.
    {
        std::vector<double> times;
        size_t accepted = 0;
        for (size_t run = 0; run < runs; run++)
        {
            BalanceState balances(users);
            std::vector<Transaction> pool;
            std::set<Hash> pool_ids;
            const std::set<Hash> chain_ids;
            auto start = std::chrono::steady_clock::now();
            for (const Transaction &transaction : generated)
            {
                add_transaction_to_pool(pool, pool_ids, transaction, balances, chain_ids);
            }
            times.push_back(seconds_since(start));
            accepted = pool.size();
        }
        report("3. Tikrinimas pries fonda", times,
               std::to_string(generated.size()) + " transakciju per karta, priimta " + std::to_string(accepted));
    }

    // 4. Penkiu kandidatu sudarymas is pilno fondo.
    {
        const size_t repeats = 20;
        std::vector<double> times;
        size_t selected = 0;
        for (size_t run = 0; run < runs; run++)
        {
            BalanceState balances(users);
            Blockchain chain(difficulty);
            std::mt19937_64 rng(selection_seed);
            auto start = std::chrono::steady_clock::now();
            for (size_t i = 0; i < repeats; i++)
            {
                std::vector<Selection> selections =
                    build_candidates(5, generated, balances, rng, chain.get_transaction_ids());
                selected = selections[0].selected.size();
            }
            times.push_back(seconds_since(start));
        }
        report("4. Kandidatu sudarymas", times,
               std::to_string(repeats) + " kartu po 5 kandidatus is 10000 transakciju fondo, kandidate " +
                   std::to_string(selected) + " transakciju");
    }

    // 5. Visas paleidimas be konsoles isvedimo.
    {
        std::vector<double> times;
        Hash tip;
        size_t blocks = 0;
        size_t included = 0;
        Bakugan total = 0;
        for (size_t run = 0; run < runs; run++)
        {
            BalanceState balances(users);
            Blockchain chain(difficulty);
            std::vector<Transaction> pool = generated;
            std::mt19937_64 rng(selection_seed);
            auto start = std::chrono::steady_clock::now();
            MiningResult result = run_mining(pool, balances, chain, rng, false);
            times.push_back(seconds_since(start));
            tip = chain.tip().calculateHash();
            blocks = chain.height();
            included = result.included;
            total = balances.sum();
        }
        report("5. Visas paleidimas (be isvedimo)", times,
               std::to_string(blocks) + " bloku, " + std::to_string(included) + " transakciju, lesu suma " +
                   std::to_string(total));
        std::cout << "  paskutinio bloko hash: " << tip << "\n";
    }

    return 0;
}
