#pragma once

#include "balance_state.hpp"
#include "blockchain.hpp"
#include <cstddef>
#include <random>
#include <vector>

struct Selection
{
    std::vector<Transaction> selected;
    std::vector<Transaction> rejected;
};

// Kiekvienas kandidatas gauna tiek bandymu per eile. Jei per visa eile niekas nerado nonce,
// biudzetas padvigubinamas. Po candidate_max_passes nesekmingu eiliu kasimas nutraukiamas.
constexpr uint64_t candidate_start_budget = 2000;
constexpr size_t candidate_max_passes = 10;
// Tiek kandidatu varzosi del kiekvieno bloko.
constexpr size_t candidates_per_block = 5;

// Vienas konkuruojantis kandidatas: jo atrinktos transakcijos, neiskastas blokas
// ir kiek jis jau bande.
struct Candidate
{
    Selection selection;
    Block block;
    uint64_t attempts = 0;
    double seconds = 0.0;
    bool found = false;
    size_t turns = 0; // kiek kartu kandidatas gavo eile kasti
};

struct MiningResult
{
    std::vector<Transaction> rejected;
    size_t included = 0;

    // Kandidatu suvestine per visa paleidima.
    std::vector<size_t> wins;        // wins[i] - kiek bloku laimejo kandidatas i + 1
    uint64_t candidate_attempts = 0; // visu kandidatu bandymai, iskaitant nelaimejusius
    double candidate_seconds = 0.0;  // visu kandidatu kasimo laikas
    size_t budget_raises = 0;        // kiek kartu teko dvigubinti biudzeta
};

Selection select_transaction(const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids);

std::vector<Selection> build_candidates(size_t candidate_count, const std::vector<Transaction> &pool, const BalanceState &balances, std::mt19937_64 &rng, const std::set<Hash> &known_ids);

// Kandidatai kasa paeiliui (1, 2, ... n), kiekvienas po `start_budget` bandymu.
// Pirmas rades tinkama nonce laimi, grazinamas jo indeksas. Jei per visa eile niekas nerado,
// biudzetas dvigubinamas ir tesiama nuo dar neisbandytu nonce. Po `max_passes` eiliu metama klaida.
// Kandidatai be transakciju praleidziami.
size_t mine_candidates(std::vector<Candidate> &candidates, uint64_t start_budget, size_t max_passes);

void remove_from_pool(std::vector<Transaction> &pool, const std::vector<Transaction> &to_remove);

// Kasimo ciklas. Kiekvienam blokui:
//   1. sudaromi candidates_per_block kandidatu ant to paties prev_hash ir tu paciu patvirtintu balansu;
//   2. kandidatai kasa paeiliui su bandymu biudzetu (mine_candidates);
//   3. laimi pirmas (eiles tvarka) rades tinkama nonce, jo bloka dar karta patikrina add_block;
//   4. is fondo pasalinamos tik laimetojo itrauktos ir atmestos transakcijos,
//      nelaimejusiu kandidatu darbas ismetamas, kitam blokui kandidatai sudaromi is naujo.
MiningResult run_mining(std::vector<Transaction> &pool, BalanceState &balances, Blockchain &chain,
                        std::mt19937_64 &rng, bool print_blocks);
