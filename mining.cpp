#include "mining.hpp"
#include "validation.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <utility>

Selection select_transaction(const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids)
{
    Selection selection;
    BalanceState tmp = balances;

    // Fondas nekopijuojamas: traukiami ir salinami tik transakciju indeksai.
    // Traukimo taisykle ta pati kaip v0.2 (atsitiktine vieta likusiu sarase, salinama islaikant tvarka),
    // todel tas pats seed atrenka tas pacias transakcijas.
    std::vector<size_t> remaining(pool.size());
    for (size_t i = 0; i < remaining.size(); i++)
    {
        remaining[i] = i;
    }

    // Patvirtintu ID aibe nekopijuojama: sio kandidato ID renkami atskirai.
    std::set<Hash> candidate_ids;

    while (selection.selected.size() < max_block_transactions && remaining.empty() == false)
    {
        size_t position = rng() % remaining.size();
        const Transaction &transaction = pool[remaining[position]];
        remaining.erase(remaining.begin() + position);

        // Pirma formatas, dalyviai, ID ir pasikartojimai, tada balansas laikinoje kopijoje.
        std::string error = validate_transaction(tmp, transaction, known_ids);
        if (error.empty() && candidate_ids.count(transaction.get_transaction_id()) > 0)
            error = "pasikartojantis ID";
        if (error.empty())
            error = tmp.check(transaction);

        if (!error.empty())
        {
            selection.rejected.push_back(transaction);
            selection.reject_reasons.push_back(error);
            continue;
        }

        tmp.apply(transaction);

        selection.selected.push_back(transaction);

        candidate_ids.insert(transaction.get_transaction_id());
    }
    return selection;
}

std::vector<Selection> build_candidates(size_t candidate_count, const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids)
{
    std::vector<Selection> candidate_selections;
    for(size_t i = 0; i < candidate_count; i++)
        candidate_selections.push_back(select_transaction(pool, balances, rng, known_ids));

    return candidate_selections;
}

size_t mine_candidates(std::vector<Candidate> &candidates, uint64_t start_budget, size_t max_passes)
{
    uint64_t budget = start_budget;

    for (size_t pass = 0; pass < max_passes; pass++)
    {
        for (size_t i = 0; i < candidates.size(); i++)
        {
            Candidate &candidate = candidates[i];
            if (candidate.selection.selected.empty())
                continue;

            uint64_t attempts = 0;
            double seconds = 0.0;
            bool found = candidate.block.mine(budget, attempts, seconds);

            candidate.attempts += attempts;
            candidate.seconds += seconds;
            candidate.turns++;

            if (found)
            {
                candidate.found = true;
                return i;
            }
        }

        // Per visa eile niekas nerado: kitoje eileje kiekvienas gauna dvigubai daugiau bandymu.
        budget *= 2;
    }

    throw std::runtime_error("Joks kandidatas nerado tinkamo nonce per leistinas eiles");
}

// Vieno kandidato darbas per viena eile, vykdomas atskiroje gijoje.
// `first_found` - maziausias numeris kandidato, kuris sioje eileje jau rado nonce.
static void mine_turn(Candidate &candidate, size_t index, uint64_t budget, std::atomic<size_t> &first_found)
{
    // Kasama nedideliais gabalais, kad tarp ju butu galima pasitikrinti, ar dar verta testi.
    const uint64_t chunk = 256;

    candidate.turns++;
    uint64_t left = budget;
    while (left > 0)
    {
        if (first_found.load() < index)
        {
            candidate.stopped = true;
            return;
        }

        uint64_t attempts = 0;
        double seconds = 0.0;
        const bool found = candidate.block.mine(std::min(chunk, left), attempts, seconds);

        candidate.attempts += attempts;
        candidate.seconds += seconds;
        left -= attempts;

        if (found)
        {
            candidate.found = true;

            // Irasomas mazesnis is dvieju numeriu, net jei kita gija rado tuo paciu metu.
            size_t current = first_found.load();
            while (index < current && !first_found.compare_exchange_weak(current, index))
            {
            }
            return;
        }
    }
}

// Uztikrina, kad visos paleistos gijos butu sulauktos ir tada, kai kuriant kita gija ivyksta klaida.
struct JoinOnExit
{
    std::vector<std::thread> &threads;

    ~JoinOnExit()
    {
        for (std::thread &thread : threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }
};

size_t mine_candidates_parallel(std::vector<Candidate> &candidates, uint64_t start_budget, size_t max_passes)
{
    const size_t nobody = candidates.size();
    uint64_t budget = start_budget;

    for (size_t pass = 0; pass < max_passes; pass++)
    {
        std::atomic<size_t> first_found(nobody);

        {
            std::vector<std::thread> threads;
            JoinOnExit join{threads};
            threads.reserve(candidates.size());

            for (size_t i = 0; i < candidates.size(); i++)
            {
                if (candidates[i].selection.selected.empty())
                    continue;

                threads.emplace_back(mine_turn, std::ref(candidates[i]), i, budget, std::ref(first_found));
            }
        } // cia sulaukiamos visos gijos

        // Laimetojas nustatomas tik pasibaigus visoms gijoms, pagrindineje gijoje.
        const size_t winner = first_found.load();
        if (winner != nobody)
            return winner;

        budget *= 2;
    }

    throw std::runtime_error("Joks kandidatas nerado tinkamo nonce per leistinas eiles");
}

void remove_from_pool(std::vector<Transaction> &pool, const std::vector<Transaction> &to_remove)
{
    if (to_remove.empty())
        return;

    std::unordered_set<Hash> ids;
    ids.reserve(to_remove.size() * 2);
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

static void print_block(const Blockchain &chain, const std::vector<Candidate> &candidates, size_t winner_index)
{
    const Block &block = chain.tip();
    const Block_h header = block.get_header();
    const Candidate &winner = candidates[winner_index];

    std::cout << "Blokas #" << chain.height() << "\n"
              << "  transakcijos:  " << block.get_transactions().size() << "\n"
              << "  prev_hash:     " << header.get_prev_hash() << "\n"
              << "  tx_root:       " << header.get_tx_root() << "\n"
              << "  hash:          " << block.calculateHash() << "\n"
              << "  difficulty:    " << header.get_difficulty() << "\n"
              << "  nonce:         " << header.get_nonce() << "\n"
              << "  bandymai:      " << winner.attempts << "\n"
              << "  kasimo laikas: " << std::fixed << std::setprecision(6) << winner.seconds
              << std::defaultfloat << std::setprecision(6) << " s\n"
              << "  kandidatai:    nr  transakcijos  eiles  bandymai  laikas, s  rezultatas\n";

    for (size_t i = 0; i < candidates.size(); i++)
    {
        const Candidate &candidate = candidates[i];
        const char *outcome = "nerado";
        if (i == winner_index)
            outcome = "laimejo";
        else if (candidate.found)
            outcome = "rado, bet pirmenybe mazesniam numeriui";
        else if (candidate.stopped)
            outcome = "sustabdytas";
        else if (candidate.turns == 0)
            outcome = "nekase";

        std::cout << "                 " << std::setw(2) << i + 1
                  << std::setw(14) << candidate.selection.selected.size()
                  << std::setw(7) << candidate.turns
                  << std::setw(10) << candidate.attempts
                  << std::setw(11) << std::fixed << std::setprecision(6) << candidate.seconds
                  << std::defaultfloat << std::setprecision(6)
                  << "  " << outcome << "\n";
    }
}

MiningResult run_mining(
    std::vector<Transaction> &pool,
    BalanceState &balances,
    Blockchain &chain,
    std::mt19937_64 &rng,
    bool print_blocks,
    MiningMode mode)
{
    MiningResult result;
    result.wins.assign(candidates_per_block, 0);

    while (!pool.empty())
    {
        std::vector<Selection> selections = build_candidates(
            candidates_per_block,
            pool,
            balances,
            rng,
            chain.get_transaction_ids());

        // Visi kandidatai pradeda nuo tos pacios busenos, todel jei vienas negali itraukti
        // nieko, negali ne vienas. Toks kandidatas perziurejo visa fonda ir viska atmete.
        bool nothing_fits = true;
        for (const Selection &selection : selections)
        {
            if (!selection.selected.empty())
                nothing_fits = false;
        }

        if (nothing_fits)
        {
            const Selection &selection = selections.front();

            result.rejected.insert(
                result.rejected.end(),
                selection.rejected.begin(),
                selection.rejected.end());
            result.reject_reasons.insert(
                result.reject_reasons.end(),
                selection.reject_reasons.begin(),
                selection.reject_reasons.end());

            remove_from_pool(pool, selection.rejected);

            break;
        }

        std::vector<Candidate> candidates;
        candidates.reserve(selections.size());
        for (Selection &selection : selections)
        {
            Block block = chain.prepare_block(selection.selected);
            candidates.push_back(Candidate{std::move(selection), std::move(block)});
        }

        const auto mining_start = std::chrono::steady_clock::now();
        const size_t winner_index =
            mode == MiningMode::parallel
                ? mine_candidates_parallel(candidates, candidate_start_budget, candidate_max_passes)
                : mine_candidates(candidates, candidate_start_budget, candidate_max_passes);
        result.mining_wall_seconds +=
            std::chrono::duration<double>(std::chrono::steady_clock::now() - mining_start).count();
        const Candidate &winner = candidates[winner_index];

        // Laimetojas tikrinamas is naujo: nuoroda, versija, tx_root, sudetingumas, PoW
        // ir visos transakcijos patvirtintu balansu kopijoje. Tik tada keiciasi busena.
        std::string reason;
        if (!chain.add_block(winner.block, balances, &reason))
        {
            throw std::runtime_error(
                "Grandine atmete iskasta bloka: " + reason);
        }

        result.included += winner.selection.selected.size();

        result.rejected.insert(
            result.rejected.end(),
            winner.selection.rejected.begin(),
            winner.selection.rejected.end());
        result.reject_reasons.insert(
            result.reject_reasons.end(),
            winner.selection.reject_reasons.begin(),
            winner.selection.reject_reasons.end());

        // Is fondo dingsta tik laimetojo transakcijos. Kitu kandidatu atrinktos
        // transakcijos lieka fonde ir gali patekti i kita bloka.
        remove_from_pool(pool, winner.selection.selected);
        remove_from_pool(pool, winner.selection.rejected);

        result.wins[winner_index]++;
        result.budget_raises += winner.turns - 1;
        for (const Candidate &candidate : candidates)
        {
            result.candidate_attempts += candidate.attempts;
            result.candidate_seconds += candidate.seconds;
        }

        if (print_blocks)
            print_block(chain, candidates, winner_index);
    }

    return result;
}
