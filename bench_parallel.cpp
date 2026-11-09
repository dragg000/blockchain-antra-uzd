// Nuoseklaus ir lygiagretaus kandidatu kasimo palyginimas vienodomis salygomis (tik v0.3).
//
// Abu budai matuojami pakaitomis toje pacioje programoje: tie patys duomenys, tos pacios antrastes,
// tas pats biudzetas. Laikas matuojamas tik aplink skaiciavima, isvedimas vyksta po jo.
//
//   ./run_bench_parallel [runs]

#include "balance_state.hpp"
#include "blockchain.hpp"
#include "mining.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static double seconds_since(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

static double mean_of(const std::vector<double> &times)
{
    double mean = 0.0;
    for (double time : times)
    {
        mean += time;
    }
    return mean / times.size();
}

static double deviation_of(const std::vector<double> &times)
{
    const double mean = mean_of(times);
    double variance = 0.0;
    for (double time : times)
    {
        variance += (time - mean) * (time - mean);
    }
    return times.size() > 1 ? std::sqrt(variance / (times.size() - 1)) : 0.0;
}

static void report(const std::string &name, const std::vector<double> &sequential,
                   const std::vector<double> &parallel, const std::string &note)
{
    std::cout << std::fixed << std::setprecision(6)
              << name << "\n"
              << "  kartai:       " << sequential.size() << "\n"
              << "  nuosekliai:   vidurkis " << mean_of(sequential) << " s, nuokrypis " << deviation_of(sequential) << " s\n"
              << "  lygiagreciai: vidurkis " << mean_of(parallel) << " s, nuokrypis " << deviation_of(parallel) << " s\n"
              << std::setprecision(2)
              << "  santykis:     " << mean_of(sequential) / mean_of(parallel) << " karto\n"
              << "  pastaba:      " << note << "\n";
}

// Penki kandidatai su skirtingomis transakcijomis ir nepasiekiamu sudetingumu.
static std::vector<Candidate> fixed_candidates(const std::vector<Transaction> &generated)
{
    std::vector<Candidate> candidates;
    for (size_t i = 0; i < 5; i++)
    {
        const std::vector<Transaction> transactions(generated.begin() + i * 100, generated.begin() + (i + 1) * 100);
        candidates.push_back(Candidate{Selection{transactions, {}},
                                       Block(Block_h(std::string(64, '0'), "", 1001, 2, 16, 0), transactions)});
    }
    return candidates;
}

struct FullRun
{
    double seconds = 0.0;
    Hash tip;
    uint64_t attempts = 0;
    double mining_wall = 0.0;
};

static FullRun full_run(const std::vector<User> &users, const std::vector<Transaction> &generated,
                        uint32_t difficulty, MiningMode mode)
{
    BalanceState balances(users);
    Blockchain chain(difficulty);
    std::vector<Transaction> pool = generated;
    std::mt19937_64 rng(3);

    FullRun run;
    auto start = std::chrono::steady_clock::now();
    MiningResult result = run_mining(pool, balances, chain, rng, false, mode);
    run.seconds = seconds_since(start);
    run.tip = chain.tip().calculateHash();
    run.attempts = result.candidate_attempts;
    run.mining_wall = result.mining_wall_seconds;
    return run;
}

int main(int argc, char *argv[])
{
    const size_t runs = argc > 1 ? std::stoul(argv[1]) : 7;

    std::vector<User> users = generate_users(1);
    const std::vector<Transaction> generated = generate_transactions(2, users, 10000);

    std::cout << "Matavimo salygos: 5 kandidatai, " << std::thread::hardware_concurrency()
              << " loginiai branduoliai, kartojimu " << runs << "\n\n";

    // 1. Fiksuotas darbas: kiekvienas is 5 kandidatu atlieka lygiai tiek pat bandymu abiem budais.
    {
        const uint64_t attempts_each = 200000;
        std::vector<double> sequential_times;
        std::vector<double> parallel_times;
        uint64_t sequential_attempts = 0;
        uint64_t parallel_attempts = 0;
        for (size_t run = 0; run < runs; run++)
        {
            for (MiningMode mode : {MiningMode::sequential, MiningMode::parallel})
            {
                std::vector<Candidate> candidates = fixed_candidates(generated);
                auto start = std::chrono::steady_clock::now();
                try
                {
                    // Sudetingumas 16 nepasiekiamas, todel po vienos eiles metama klaida.
                    if (mode == MiningMode::sequential)
                        mine_candidates(candidates, attempts_each, 1);
                    else
                        mine_candidates_parallel(candidates, attempts_each, 1);
                }
                catch (const std::runtime_error &)
                {
                }
                const double seconds = seconds_since(start);

                uint64_t attempts = 0;
                for (const Candidate &candidate : candidates)
                {
                    attempts += candidate.attempts;
                }
                if (mode == MiningMode::sequential)
                {
                    sequential_times.push_back(seconds);
                    sequential_attempts = attempts;
                }
                else
                {
                    parallel_times.push_back(seconds);
                    parallel_attempts = attempts;
                }
            }
        }
        report("1. Fiksuotas bandymu skaicius", sequential_times, parallel_times,
               "5 kandidatai po " + std::to_string(attempts_each) + " bandymu, is viso nuosekliai " +
                   std::to_string(sequential_attempts) + ", lygiagreciai " + std::to_string(parallel_attempts));
    }

    // 2 ir 3. Visas paleidimas su 10000 transakciju, sudetingumas 3 ir 4.
    for (uint32_t difficulty : {3u, 4u})
    {
        std::vector<double> sequential_times;
        std::vector<double> parallel_times;
        std::vector<double> sequential_mining;
        std::vector<double> parallel_mining;
        FullRun sequential;
        FullRun parallel;
        bool same_tip = true;
        for (size_t run = 0; run < runs; run++)
        {
            sequential = full_run(users, generated, difficulty, MiningMode::sequential);
            parallel = full_run(users, generated, difficulty, MiningMode::parallel);
            sequential_times.push_back(sequential.seconds);
            parallel_times.push_back(parallel.seconds);
            sequential_mining.push_back(sequential.mining_wall);
            parallel_mining.push_back(parallel.mining_wall);
            same_tip = same_tip && sequential.tip == parallel.tip;
        }
        const std::string number = difficulty == 3 ? "2" : "3";
        report(number + "a. Visas paleidimas, sudetingumas " + std::to_string(difficulty), sequential_times, parallel_times,
               std::string("paskutinio bloko hash ") + (same_tip ? "sutampa: " : "NESUTAMPA: ") + parallel.tip);
        report(number + "b. Tik kandidatu kasimas tame paciame paleidime", sequential_mining, parallel_mining,
               "bandymu nuosekliai " + std::to_string(sequential.attempts) + ", lygiagreciai " +
                   std::to_string(parallel.attempts) + " (sustabdyti kandidatai spejo atlikti dali darbo)");
    }

    return 0;
}
