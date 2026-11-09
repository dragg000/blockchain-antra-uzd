#include "mining.hpp"
#include "validation.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>

Selection select_transaction(const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids)
{
    Selection selection;
    BalanceState tmp = balances;
    std::vector<Transaction> remaining = pool;
    std::set<Hash> candidate_ids = known_ids;

    while (selection.selected.size() < 100 && remaining.empty() == false)
    {
        size_t position = rng() % remaining.size();
        Transaction transaction = remaining[position];
        remaining.erase(remaining.begin() + position);

        // Pirma formatas, dalyviai, ID ir pasikartojimai, tada balansas laikinoje kopijoje.
        std::string error = validate_transaction(tmp, transaction, candidate_ids);
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

void remove_from_pool(std::vector<Transaction> &pool, const std::vector<Transaction> &to_remove)
{
    std::set<Hash> ids;
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
              << "  kasimo laikas: " << winner.seconds << " s\n"
              << "  kandidatai:    nr  transakcijos  eiles  bandymai  laikas, s  rezultatas\n";

    for (size_t i = 0; i < candidates.size(); i++)
    {
        const Candidate &candidate = candidates[i];
        const char *outcome = "nerado";
        if (candidate.found)
            outcome = "laimejo";
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
    bool print_blocks)
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
        for (const Selection &selection : selections)
        {
            candidates.push_back(Candidate{selection, chain.prepare_block(selection.selected)});
        }

        size_t winner_index = mine_candidates(candidates, candidate_start_budget, candidate_max_passes);
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
