#include "balance_state.hpp"
#include "blockchain.hpp"
#include "hash.hpp"
#include "mining.hpp"
#include "transaction.hpp"
#include "user.hpp"
#include <iostream>
#include <random>
#include <set>
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

static Transaction make_transaction(const Key &sender, const Key &receiver, Bakugan amount, uint64_t sequence)
{
    Transaction transaction(sender, receiver, amount, "", sequence);
    transaction.set_transaction_id(hashTextUtf8(transaction.serialize()));
    return transaction;
}

static std::vector<User> small_users()
{
    return {User("a", "A", 100), User("b", "B", 50), User("c", "C", 0)};
}

static void test_serialization()
{
    std::cout << "Serializacija\n";
    CHECK(Transaction("a12f", "b34e", 100, "", 5).serialize() == "a12f|b34e|100|5");
    CHECK(Transaction("a", "b", 1, "", 23).serialize() != Transaction("a", "b", 12, "", 3).serialize());
}

static void test_same_input_same_transaction_hashes()
{
    std::cout << "Tas pats seed duoda tas pacias transakciju maisas\n";
    std::vector<User> users_a = generate_users(1);
    std::vector<User> users_b = generate_users(1);
    std::vector<Transaction> first = generate_transactions(2, users_a, 500);
    std::vector<Transaction> second = generate_transactions(2, users_b, 500);

    bool same = first.size() == second.size();
    for (size_t i = 0; same && i < first.size(); i++)
    {
        same = first[i].get_transaction_id() == second[i].get_transaction_id();
    }
    CHECK(same);
}

static void test_fixed_header_same_block_hash()
{
    std::cout << "Fiksuota antraste duoda ta pati bloko hash\n";
    std::vector<Transaction> transactions = {make_transaction("A", "B", 1, 0)};
    Block first(Block_h(std::string(64, '0'), "", 1000, 1, 2, 7), transactions);
    Block second(Block_h(std::string(64, '0'), "", 1000, 1, 2, 7), transactions);
    Block other_nonce(Block_h(std::string(64, '0'), "", 1000, 1, 2, 8), transactions);

    CHECK(first.calculateHash() == second.calculateHash());
    CHECK(first.calculateHash() != other_nonce.calculateHash());
}

static void test_genesis()
{
    std::cout << "Genesis blokas\n";
    Blockchain chain(1);
    CHECK(chain.height() == 0);
    CHECK(chain.tip().get_header().get_prev_hash() == std::string(64, '0'));
    CHECK(chain.tip().get_transactions().empty());
}

static void test_hand_checked_case()
{
    std::cout << "Mazas rankomis patikrintas atvejis\n";
    // A 100 - 30 = 70, B 50 + 30 - 20 = 60, C 0 + 20 = 20. Suma 150.
    std::vector<User> users = small_users();
    std::vector<Transaction> pool = {make_transaction("A", "B", 30, 0), make_transaction("B", "C", 20, 1)};
    BalanceState balances(users);
    Blockchain chain(1);
    std::mt19937_64 rng(1);

    MiningResult result = run_mining(pool, balances, chain, rng, false);

    CHECK(chain.height() == 1);
    CHECK(result.included == 2);
    CHECK(result.rejected.empty());
    CHECK(pool.empty());
    CHECK(balances.get_balance("A") == 70);
    CHECK(balances.get_balance("B") == 60);
    CHECK(balances.get_balance("C") == 20);
    CHECK(balances.sum() == 150);
}

static void test_cannot_overspend()
{
    std::cout << "Negalima isleisti daugiau, nei turima\n";
    std::vector<User> users = small_users();

    BalanceState single(users);
    CHECK(!single.apply(make_transaction("A", "B", 101, 0)));
    CHECK(single.get_balance("A") == 100);
    CHECK(single.get_balance("B") == 50);
    CHECK(single.apply(make_transaction("A", "B", 100, 1)));
    CHECK(single.get_balance("A") == 0);
    CHECK(!single.apply(make_transaction("Z", "B", 1, 2)));
    CHECK(!single.apply(make_transaction("A", "Z", 0, 3)));
    CHECK(single.sum() == 150);

    // Uzduoties pavyzdys: A turi 100, T1 moka 70, T2 moka 60, tilpti gali tik viena.
    std::vector<Transaction> pool = {make_transaction("A", "B", 70, 0), make_transaction("A", "C", 60, 1)};
    BalanceState confirmed(users);
    bool exactly_one_every_time = true;
    bool balances_unchanged = true;
    for (uint64_t seed = 0; seed < 50; seed++)
    {
        std::mt19937_64 rng(seed);
        Selection selection = select_transaction(pool, confirmed, rng);
        exactly_one_every_time = exactly_one_every_time && selection.selected.size() == 1 && selection.rejected.size() == 1;
        balances_unchanged = balances_unchanged && confirmed.get_balance("A") == 100 && pool.size() == 2;
    }
    CHECK(exactly_one_every_time);
    CHECK(balances_unchanged);
}

static void test_last_partial_block()
{
    std::cout << "Paskutinis nepilnas blokas\n";
    std::vector<User> users = {User("a", "A", 1000000), User("b", "B", 0)};
    std::vector<Transaction> pool;
    for (uint64_t i = 0; i < 250; i++)
    {
        pool.push_back(make_transaction("A", "B", 1, i));
    }
    BalanceState balances(users);
    Blockchain chain(1);
    std::mt19937_64 rng(1);

    MiningResult result = run_mining(pool, balances, chain, rng, false);

    CHECK(chain.height() == 3);
    CHECK(chain.get_block(1).get_transactions().size() == 100);
    CHECK(chain.get_block(2).get_transactions().size() == 100);
    CHECK(chain.get_block(3).get_transactions().size() == 50);
    CHECK(result.included == 250);
    CHECK(balances.get_balance("B") == 250);
}

static void test_loop_stops_when_nothing_fits()
{
    std::cout << "Ciklas sustoja, kai nieko negalima itraukti\n";
    std::vector<User> users = small_users();
    std::vector<Transaction> pool = {make_transaction("A", "B", 1000, 0), make_transaction("Z", "B", 1, 1)};
    BalanceState balances(users);
    Blockchain chain(1);
    std::mt19937_64 rng(1);

    MiningResult result = run_mining(pool, balances, chain, rng, false);

    CHECK(chain.height() == 0);
    CHECK(result.included == 0);
    CHECK(result.rejected.size() == 2);
    CHECK(pool.empty());
    CHECK(balances.sum() == 150);
}

static void test_full_run_invariants()
{
    std::cout << "Visas paleidimas: nera dublikatu ir neigiamu balansu\n";
    const size_t count = 2000;
    std::vector<User> users = generate_users(1);
    std::vector<Transaction> pool = generate_transactions(2, users, count);
    BalanceState balances(users);
    const Bakugan start_total = balances.sum();
    Blockchain chain(2);
    std::mt19937_64 rng(3);

    MiningResult result = run_mining(pool, balances, chain, rng, false);

    CHECK(result.included + result.rejected.size() + pool.size() == count);
    CHECK(balances.sum() == start_total);

    // Balansai be zenklo, todel "neigiamas" balansas apsisuktu ir virsytu visa suma.
    bool no_wrapped_balance = true;
    for (const User &user : users)
    {
        no_wrapped_balance = no_wrapped_balance && balances.get_balance(user.get_key()) <= start_total;
    }
    CHECK(no_wrapped_balance);

    std::set<Hash> ids;
    size_t on_chain = 0;
    bool links_ok = true;
    bool pow_ok = true;
    for (size_t height = 1; height <= chain.height(); height++)
    {
        const Block &block = chain.get_block(height);
        links_ok = links_ok && block.get_header().get_prev_hash() == chain.get_block(height - 1).calculateHash();
        pow_ok = pow_ok && block.hasValidProofOfWork();
        for (const Transaction &transaction : block.get_transactions())
        {
            ids.insert(transaction.get_transaction_id());
            on_chain++;
        }
    }
    CHECK(on_chain == result.included);
    CHECK(ids.size() == on_chain);
    CHECK(links_ok);
    CHECK(pow_ok);
}

static void test_same_input_same_chain()
{
    std::cout << "Tas pats paleidimas duoda ta pati paskutini bloka\n";
    Hash tips[2];
    for (Hash &tip : tips)
    {
        std::vector<User> users = generate_users(1);
        std::vector<Transaction> pool = generate_transactions(2, users, 300);
        BalanceState balances(users);
        Blockchain chain(2);
        std::mt19937_64 rng(3);
        run_mining(pool, balances, chain, rng, false);
        tip = chain.tip().calculateHash();
    }
    CHECK(tips[0] == tips[1]);
}

static void test_bad_blocks_are_refused()
{
    std::cout << "Netinkami blokai atmetami\n";
    Blockchain chain(3);
    std::vector<Transaction> transactions = {make_transaction("A", "B", 1, 0)};
    uint64_t attempts = 0;
    double seconds = 0.0;
    Block mined = chain.minePendingTransactions(transactions, attempts, seconds);
    const Block_h header = mined.get_header();
    CHECK(mined.hasValidProofOfWork());

    // Tas pats nonce, bet pakeistas turinys: PoW nebetinka.
    std::vector<Transaction> changed = {make_transaction("A", "B", 2, 0)};
    Block changed_contents(header, changed);
    CHECK(!changed_contents.hasValidProofOfWork());
    CHECK(!chain.add_block(changed_contents));

    // Neteisinga nuoroda i ankstesni bloka.
    Block wrong_prev(Block_h(std::string(64, 'f'), "", header.get_timestamp(), header.get_version(),
                             header.get_difficulty(), 0),
                     transactions);
    wrong_prev.mine(attempts, seconds);
    CHECK(!chain.add_block(wrong_prev));

    // Neiskastas blokas.
    Block_h unmined_header(header.get_prev_hash(), "", header.get_timestamp(), header.get_version(),
                           header.get_difficulty(), header.get_nonce() + 1);
    Block unmined(unmined_header, transactions);
    CHECK(!unmined.hasValidProofOfWork());
    CHECK(!chain.add_block(unmined));

    CHECK(chain.height() == 0);
    CHECK(chain.add_block(mined));
    CHECK(chain.height() == 1);
}

int main()
{
    test_serialization();
    test_same_input_same_transaction_hashes();
    test_fixed_header_same_block_hash();
    test_genesis();
    test_hand_checked_case();
    test_cannot_overspend();
    test_last_partial_block();
    test_loop_stops_when_nothing_fits();
    test_full_run_invariants();
    test_same_input_same_chain();
    test_bad_blocks_are_refused();

    std::cout << "\nPatikros: " << checks << ", nepavyko: " << failures << "\n";
    return failures == 0 ? 0 : 1;
}
