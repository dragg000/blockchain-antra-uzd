// v0.3 testai. Bendras v0.2 ir v0.3 rinkinys lieka nepakeistas faile tests/tests.cpp,
// o cia tikrinami tik v0.3 pakeitimai.

#include "balance_state.hpp"
#include "blockchain.hpp"
#include "hash.hpp"
#include "mining.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include "validation.hpp"
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

static int failures = 0;
static int checks = 0;

#define CHECK(condition)                                                          \
    do                                                                            \
    {                                                                             \
        checks++;                                                                 \
        if (!(condition))                                                         \
        {                                                                         \
            failures++;                                                           \
            std::cout << "  NEPAVYKO " << __LINE__ << " eil.: " #condition "\n";  \
        }                                                                         \
    } while (false)

static Key key(const std::string &name)
{
    return hashTextUtf8(name);
}

static Transaction make_transaction(const Key &sender, const Key &receiver, Bakugan amount, uint64_t sequence)
{
    Transaction transaction(sender, receiver, amount, "", sequence);
    transaction.set_transaction_id(hashTextUtf8(transaction.serialize()));
    return transaction;
}

static void test_budget_is_exact()
{
    std::cout << "Biudzetas tikslus: nei vieno papildomo ar pakartoto bandymo\n";
    const std::vector<Transaction> transactions = {make_transaction(key("A"), key("B"), 1, 0)};
    const Block_h header(std::string(64, '0'), "", 1001, 2, 3, 0);
    uint64_t attempts = 0;
    double seconds = 0.0;

    // Tinkamas nonce N randamas kasant be ribos.
    Block reference(header, transactions);
    reference.mine(attempts, seconds);
    const uint64_t valid_nonce = reference.get_header().get_nonce();
    CHECK(valid_nonce > 0);
    CHECK(attempts == valid_nonce + 1);

    // N bandymu patikrina nonce 0..N-1, todel tinkamo dar neranda ir sustoja lygiai ties juo.
    Block block(header, transactions);
    CHECK(!block.mine(valid_nonce, attempts, seconds));
    CHECK(attempts == valid_nonce);
    CHECK(block.get_header().get_nonce() == valid_nonce);

    // Kitas ejimas pradeda nuo N ir randa ji pirmu bandymu.
    CHECK(block.mine(1, attempts, seconds));
    CHECK(attempts == 1);
    CHECK(block.get_header().get_nonce() == valid_nonce);
    CHECK(block.calculateHash() == reference.calculateHash());

    // Nulinis biudzetas nieko netikrina ir nieko nekeicia.
    Block untouched(header, transactions);
    CHECK(!untouched.mine(0, attempts, seconds));
    CHECK(attempts == 0);
    CHECK(untouched.get_header().get_nonce() == 0);

    // Jau iskasto bloko kasimas is karto grazina true ir nonce nekeicia.
    CHECK(reference.mine(5, attempts, seconds));
    CHECK(attempts == 1);
    CHECK(reference.get_header().get_nonce() == valid_nonce);
}

struct RunResult
{
    Hash tip;
    size_t blocks = 0;
    size_t included = 0;
    size_t rejected = 0;
    Bakugan total = 0;
};

static RunResult run_chain(uint64_t count, uint32_t difficulty, uint64_t selection_seed)
{
    std::vector<User> users = generate_users(1);
    std::vector<Transaction> pool = generate_transactions(2, users, count);
    BalanceState balances(users);
    Blockchain chain(difficulty);
    std::mt19937_64 rng(selection_seed);
    MiningResult result = run_mining(pool, balances, chain, rng, false);

    RunResult run;
    run.tip = chain.tip().calculateHash();
    run.blocks = chain.height();
    run.included = result.included;
    run.rejected = result.rejected.size();
    run.total = balances.sum();
    return run;
}

// Sias reiksmes isspausdino uzfiksuota v0.2 versija (zyma v0.2) su tais paciais duomenimis.
struct Golden
{
    uint64_t count;
    uint32_t difficulty;
    uint64_t selection_seed;
    size_t blocks;
    size_t included;
    size_t rejected;
    const char *tip;
};

static const Golden golden_runs[] = {
    {250, 3, 3, 3, 250, 0, "000c331f06b384d9fcb273acb06b7c58ac6171f29cfe64e722b4683af52535bc"},
    {2000, 3, 3, 20, 2000, 0, "000d8c1b2b53efbae7127ecb3ee912523fe0718f9606f43d7b0427f27128b36b"},
    {10000, 3, 3, 100, 9999, 1, "000df5652989778a44f24b2ff303158779079dbdacf501e27764198929b0d64d"},
    {1000, 2, 7, 10, 1000, 0, "00a41b7c546f2fcd369e9d66fa0e6151bb2d1f9e31c2d6d9b472fe1befda2460"},
    {600, 4, 11, 6, 600, 0, "0000503eebd9ef1315a53a94ef59297dbf7c071799e9ac226e5a23a10c5fc0be"},
    {3000, 3, 99, 30, 3000, 0, "000fd4965fd82e2cf9b924eb8ef182437ade7f85384ad4ebfe546b35219e8405"},
    {5000, 3, 5, 50, 5000, 0, "000f01028c7c8cad3fb662b030efa8e49417b24e460bd719557dc9717bc3ac45"},
};

static void test_same_chain_as_v02()
{
    std::cout << "Ta pati grandine kaip v0.2 (7 duomenu rinkiniai)\n";
    for (const Golden &golden : golden_runs)
    {
        const RunResult run = run_chain(golden.count, golden.difficulty, golden.selection_seed);
        CHECK(run.tip == golden.tip);
        CHECK(run.blocks == golden.blocks);
        CHECK(run.included == golden.included);
        CHECK(run.rejected == golden.rejected);
        CHECK(run.total == 498753171);
    }
}

int main()
{
    test_budget_is_exact();
    test_same_chain_as_v02();

    std::cout << "\nv0.3 patikros: " << checks << ", nepavyko: " << failures << "\n";
    return failures == 0 ? 0 : 1;
}
