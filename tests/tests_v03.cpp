// v0.3 testai. Bendras v0.2 ir v0.3 rinkinys lieka nepakeistas faile tests/tests.cpp,
// o cia tikrinami tik v0.3 pakeitimai.

#include "balance_state.hpp"
#include "blockchain.hpp"
#include "hash.hpp"
#include "merkle.hpp"
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

static void test_hash_stream_matches_hash()
{
    std::cout << "Srautine maisa sutampa su hashTextUtf8\n";

    // Zinoma reiksme is 1 uzduoties README.
    CHECK(hashTextUtf8("hello world") == "dcff59e3b321da126e0bc4d63b61bc3f74ee1ca81bd4c3607915403067988abe");
    HashStream known;
    known.update("hello world");
    CHECK(known.hex() == "dcff59e3b321da126e0bc4d63b61bc3f74ee1ca81bd4c3607915403067988abe");

    // Visi ilgiai nuo 0 iki 300 baitu: apima uzpildymo ribas ties 55, 56, 63, 64, 119, 120, 127, 128.
    std::mt19937_64 rng(2026);
    bool all_lengths = true;
    bool byte_by_byte = true;
    for (size_t length = 0; length <= 300; length++)
    {
        std::string text(length, '\0');
        for (char &c : text)
        {
            c = static_cast<char>(rng() % 256);
        }
        const Hash expected = hashTextUtf8(text);

        HashStream whole;
        whole.update(text);
        all_lengths = all_lengths && whole.hex() == expected;

        HashStream single;
        for (char c : text)
        {
            single.update(&c, 1);
        }
        byte_by_byte = byte_by_byte && single.hex() == expected;
    }
    CHECK(all_lengths);
    CHECK(byte_by_byte);

    // Kiekviena dalijimo vieta 200 baitu tekste, taip pat dalijimas i tris dalis.
    std::string text(200, '\0');
    for (char &c : text)
    {
        c = static_cast<char>(rng() % 256);
    }
    const Hash expected = hashTextUtf8(text);
    bool every_split = true;
    for (size_t split = 0; split <= text.size(); split++)
    {
        HashStream stream;
        stream.update(text.data(), split);
        stream.update(text.data() + split, text.size() - split);
        every_split = every_split && stream.hex() == expected;

        HashStream three;
        const size_t half = split / 2;
        three.update(text.data(), half);
        three.update(text.data() + half, split - half);
        three.update(text.data() + split, text.size() - split);
        every_split = every_split && three.hex() == expected;
    }
    CHECK(every_split);

    // digest() busenos nekeicia: ta pati tarpine busena tinka skirtingoms pabaigoms.
    HashStream prefix;
    prefix.update("bendra pradzia, ilgesne nei vienas 64 baitu blokas, kad dalis jau butu sumaisyta|");
    const std::string before = prefix.hex();
    HashStream first = prefix;
    first.update("1|3");
    HashStream second = prefix;
    second.update("2|3");
    CHECK(prefix.hex() == before);
    CHECK(first.hex() == hashTextUtf8("bendra pradzia, ilgesne nei vienas 64 baitu blokas, kad dalis jau butu sumaisyta|1|3"));
    CHECK(second.hex() == hashTextUtf8("bendra pradzia, ilgesne nei vienas 64 baitu blokas, kad dalis jau butu sumaisyta|2|3"));
    CHECK(first.hex() != second.hex());

    // Tuscia ivestis.
    CHECK(HashStream().hex() == hashTextUtf8(""));

    // Pradiniu nuliu tikrinimas baitais atitinka hex teksto tikrinima.
    bool zero_check_matches = true;
    for (uint64_t i = 0; i < 20000; i++)
    {
        HashStream stream;
        stream.update(std::to_string(i));
        const std::string hex = stream.hex();
        for (uint32_t count = 0; count <= 4; count++)
        {
            const bool by_text = hex.substr(0, count) == std::string(count, '0');
            zero_check_matches = zero_check_matches && has_leading_zero_nibbles(stream.digest(), count) == by_text;
        }
    }
    CHECK(zero_check_matches);
    HashStream::Digest zeros{};
    CHECK(has_leading_zero_nibbles(zeros, 64));
    CHECK(!has_leading_zero_nibbles(zeros, 65));
    HashStream::Digest half_byte{};
    half_byte[0] = 0x0F;
    CHECK(has_leading_zero_nibbles(half_byte, 1));
    CHECK(!has_leading_zero_nibbles(half_byte, 2));
    CHECK(digest_to_hex(half_byte) == "0f" + std::string(62, '0'));
}

static void test_fast_mining_matches_slow_check()
{
    std::cout << "Greitas kasimas randa ta pati nonce kaip letas tikrinimas\n";
    bool same_nonce = true;
    bool valid_by_slow_check = true;
    bool same_in_pieces = true;
    for (uint32_t difficulty = 0; difficulty <= 3; difficulty++)
    {
        for (uint64_t variant = 0; variant < 6; variant++)
        {
            const std::vector<Transaction> transactions = {make_transaction(key("A"), key("B"), 1 + variant, variant)};
            const Block_h header(std::string(64, '0'), "", 1001 + variant, 2, difficulty, 0);

            // Letas kelias: kiekvienam nonce sudaromas naujas blokas ir tikrinamas per calculateHash.
            uint64_t expected = 0;
            while (!Block(Block_h(header.get_prev_hash(), "", header.get_timestamp(), header.get_version(),
                                  header.get_difficulty(), expected),
                          transactions)
                        .hasValidProofOfWork())
            {
                expected++;
            }

            uint64_t attempts = 0;
            double seconds = 0.0;
            Block mined(header, transactions);
            mined.mine(attempts, seconds);
            same_nonce = same_nonce && mined.get_header().get_nonce() == expected && attempts == expected + 1;
            valid_by_slow_check = valid_by_slow_check && mined.hasValidProofOfWork() &&
                                  mined.calculateHash().substr(0, difficulty) == std::string(difficulty, '0');

            // Kasant mazais gabalais po 7 bandymus gaunamas tas pats nonce ir ta pati bandymu suma.
            Block pieces(header, transactions);
            uint64_t total = 0;
            bool found = false;
            while (!found)
            {
                found = pieces.mine(7, attempts, seconds);
                total += attempts;
            }
            same_in_pieces = same_in_pieces && pieces.get_header().get_nonce() == expected && total == expected + 1;
        }
    }
    CHECK(same_nonce);
    CHECK(valid_by_slow_check);
    CHECK(same_in_pieces);
}

// Leta atskaitos realizacija, parasyta taip, kaip v0.2: kiekvienas mazgas verciamas i hex ir atgal.
static std::string slow_hex_to_bytes(const std::string &hex)
{
    const std::string digits = "0123456789abcdef";
    std::string bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
        bytes.push_back(static_cast<char>(digits.find(hex[i]) * 16 + digits.find(hex[i + 1])));
    }
    return bytes;
}

static Hash slow_merkle_root(std::vector<Hash> level)
{
    if (level.empty())
        return hashTextUtf8("");
    while (level.size() > 1)
    {
        if (level.size() % 2 == 1)
            level.push_back(level.back());
        std::vector<Hash> next;
        for (size_t i = 0; i < level.size(); i += 2)
        {
            next.push_back(hashTextUtf8(slow_hex_to_bytes(level[i]) + slow_hex_to_bytes(level[i + 1])));
        }
        level = next;
    }
    return level[0];
}

template <typename Function>
static bool throws_invalid_argument(Function function)
{
    try
    {
        function();
    }
    catch (const std::invalid_argument &)
    {
        return true;
    }
    return false;
}

static void test_merkle_module()
{
    std::cout << "Merkle modulis: ta pati saknis kaip leta realizacija, grieztas ID formatas\n";
    std::vector<User> users = generate_users(1);
    const std::vector<Transaction> generated = generate_transactions(2, users, 130);
    std::vector<Hash> ids;
    for (const Transaction &transaction : generated)
    {
        ids.push_back(transaction.get_transaction_id());
    }

    // Visi dydziai nuo 0 iki 130 lapu.
    bool every_size = true;
    for (size_t size = 0; size <= ids.size(); size++)
    {
        const std::vector<Hash> part(ids.begin(), ids.begin() + size);
        every_size = every_size && merkle_root(part) == slow_merkle_root(part);
    }
    CHECK(every_size);

    // Pirmu 100 sugeneruotu transakciju saknis, kuria isspausdino v0.2 (results/v0.2/bench.txt).
    const std::vector<Hash> hundred(ids.begin(), ids.begin() + 100);
    CHECK(merkle_root(hundred) == "75374ba62a49cfa24d2506b07cd6ae4a06b9e21b72eea8355d12fd55367dce0d");
    const std::vector<Transaction> first_hundred(generated.begin(), generated.begin() + 100);
    CHECK(Block(Block_h(std::string(64, '0'), "", 1001, 2, 3, 0), first_hundred).calculateTxRoot() == merkle_root(hundred));

    CHECK(merkle_root({}) == hashTextUtf8(""));
    CHECK(merkle_root({ids[0]}) == ids[0]);

    // Dekodavimas ir kodavimas atgal.
    CHECK(digest_to_hex(decode_id(ids[0])) == ids[0]);
    CHECK(decode_id("00ff" + std::string(60, '0'))[1] == 0xFF);

    // Netinkamo formato ID: v0.2 priimdavo bet koki lygini ilgi, o "1g" tyliai perskaitydavo kaip "1".
    const std::vector<std::string> bad_ids = {
        "",
        "abc",
        "ab",
        std::string(63, 'a'),
        std::string(65, 'a'),
        std::string(66, 'a'),
        std::string(63, 'a') + "g",
        "g" + std::string(63, 'a'),
        "1g" + std::string(62, 'a'),
        std::string(63, 'a') + "A",
        std::string(63, 'a') + " ",
        std::string(32, 'a') + "|" + std::string(31, 'a'),
    };
    bool all_refused = true;
    bool blocks_refused = true;
    for (const std::string &bad : bad_ids)
    {
        all_refused = all_refused && throws_invalid_argument([&] { decode_id(bad); });
        all_refused = all_refused && throws_invalid_argument([&] { merkle_root({bad}); });
        all_refused = all_refused && throws_invalid_argument([&] { merkle_root({ids[0], ids[1], bad}); });

        // Tokio bloko neimanoma net sukurti.
        Transaction transaction = generated[0];
        transaction.set_transaction_id(bad);
        blocks_refused = blocks_refused && throws_invalid_argument([&] {
            Block(Block_h(std::string(64, '0'), "", 1001, 2, 3, 0), {generated[1], transaction});
        });
    }
    CHECK(all_refused);
    CHECK(blocks_refused);
}

static std::vector<User> small_users()
{
    return {User("a", key("A"), 100), User("b", key("B"), 50), User("c", key("C"), 0)};
}

// Iskasa bloka ant grandines virsunes su nurodytais antrastes pakeitimais.
static Block mined_block(const Blockchain &chain, const std::vector<Transaction> &transactions)
{
    uint64_t attempts = 0;
    double seconds = 0.0;
    return chain.minePendingTransactions(transactions, attempts, seconds);
}

static Block mined_with_header(const Block_h &header, const std::vector<Transaction> &transactions)
{
    uint64_t attempts = 0;
    double seconds = 0.0;
    Block block(header, transactions);
    block.mine(attempts, seconds);
    return block;
}

static void test_block_rejection_reasons()
{
    std::cout << "Bloko atmetimo priezastys\n";
    const Key A = key("A");
    const Key B = key("B");
    const Key C = key("C");
    std::vector<User> users = small_users();
    BalanceState balances(users);
    Blockchain chain(2);
    const std::vector<Transaction> transactions = {make_transaction(A, B, 1, 0)};
    const Block good = mined_block(chain, transactions);
    const Block_h header = good.get_header();

    CHECK(chain.check_block(good, balances) == "");

    const Block wrong_prev = mined_with_header(
        Block_h(std::string(64, 'f'), "", header.get_timestamp(), 2, 2, 0), transactions);
    CHECK(chain.check_block(wrong_prev, balances) == "neteisingas prev_hash");

    const Block old_version = mined_with_header(
        Block_h(header.get_prev_hash(), "", header.get_timestamp(), 1, 2, 0), transactions);
    CHECK(chain.check_block(old_version, balances) == "nepalaikoma versija");

    const Block empty = mined_with_header(Block_h(header.get_prev_hash(), "", header.get_timestamp(), 2, 2, 0), {});
    CHECK(chain.check_block(empty, balances) == "blokas be transakciju");

    std::vector<Transaction> too_many;
    for (uint64_t i = 0; i < max_block_transactions + 1; i++)
    {
        too_many.push_back(make_transaction(B, C, 1, 1000 + i));
    }
    CHECK(chain.check_block(mined_block(chain, too_many), balances) == "per daug transakciju");

    const Block easier = mined_with_header(
        Block_h(header.get_prev_hash(), "", header.get_timestamp(), 2, 1, 0), transactions);
    CHECK(easier.hasValidProofOfWork());
    CHECK(chain.check_block(easier, balances) == "neteisingas sudetingumas");

    // Sunkesnis nei reikalaujama taip pat netinka: sudetinguma nustato grandine.
    const Block harder = mined_with_header(
        Block_h(header.get_prev_hash(), "", header.get_timestamp(), 2, 3, 0), transactions);
    CHECK(chain.check_block(harder, balances) == "neteisingas sudetingumas");

    // Neiskastas blokas: ieskomas nonce, kuris PoW netenkina.
    uint64_t bad_nonce = 0;
    while (Block(Block_h(header.get_prev_hash(), "", header.get_timestamp(), 2, 2, bad_nonce), transactions)
               .hasValidProofOfWork())
    {
        bad_nonce++;
    }
    const Block unmined(Block_h(header.get_prev_hash(), "", header.get_timestamp(), 2, 2, bad_nonce), transactions);
    CHECK(chain.check_block(unmined, balances) == "netinkamas PoW");

    // Pakeistas turinys su ta pacia antraste: tx_root perskaiciuojama, todel PoW nebetinka.
    const Block changed(header, {make_transaction(A, B, 2, 0)});
    CHECK(chain.check_block(changed, balances) == "netinkamas PoW");

    // Transakciju klaidos nurodo vieta bloke.
    CHECK(chain.check_block(mined_block(chain, {make_transaction(A, B, 70, 1), make_transaction(A, C, 60, 2)}), balances) ==
          "transakcija 2: siuntejui nepakako lesu");
    CHECK(chain.check_block(mined_block(chain, {make_transaction(A, B, 1, 3), make_transaction(A, B, 1, 3)}), balances) ==
          "transakcija 2: pasikartojantis ID");
    CHECK(chain.check_block(mined_block(chain, {make_transaction(A, key("Z"), 1, 4)}), balances) ==
          "transakcija 1: nezinomas dalyvis");
    Transaction tampered = make_transaction(A, B, 1, 5);
    tampered.set_amount(50);
    CHECK(chain.check_block(mined_block(chain, {tampered}), balances) == "transakcija 1: neteisingas ID");
    CHECK(chain.check_block(mined_block(chain, {make_transaction(A, A, 1, 6)}), balances) ==
          "transakcija 1: siuntejas ir gavejas sutampa");

    // check_block nieko nekeicia.
    CHECK(chain.height() == 0);
    CHECK(balances.get_balance(A) == 100 && balances.get_balance(B) == 50);
    CHECK(chain.get_transaction_ids().empty());

    // add_block grazina ta pacia priezasti ir atmetes nieko nekeicia.
    std::string reason = "?";
    CHECK(!chain.add_block(wrong_prev, balances, &reason));
    CHECK(reason == "neteisingas prev_hash");
    CHECK(chain.height() == 0 && balances.get_balance(A) == 100);

    CHECK(chain.add_block(good, balances, &reason));
    CHECK(reason == "");
    CHECK(chain.height() == 1 && balances.get_balance(A) == 99 && balances.get_balance(B) == 51);

    // Tas pats blokas dar karta: jis nebesiremia virsune.
    CHECK(!chain.add_block(good, balances, &reason));
    CHECK(reason == "neteisingas prev_hash");

    // Jau patvirtintos transakcijos pakartojimas naujame bloke.
    CHECK(chain.check_block(mined_block(chain, transactions), balances) == "transakcija 1: pasikartojantis ID");
    CHECK(chain.height() == 1 && balances.get_balance(A) == 99);
}

static void test_selection_reasons()
{
    std::cout << "Atrankos atmetimo priezastys issaugomos\n";
    const Key A = key("A");
    const Key B = key("B");
    const Key C = key("C");
    std::vector<User> users = small_users();
    BalanceState balances(users);
    const std::set<Hash> no_ids;

    // C neturi lesu, Z nezinomas, o A -> B telpa.
    Transaction tampered = make_transaction(A, B, 5, 3);
    tampered.set_amount(6);
    std::vector<Transaction> pool = {make_transaction(C, A, 5, 0), make_transaction(A, key("Z"), 5, 1),
                                     make_transaction(A, B, 5, 2), tampered};
    std::mt19937_64 rng(1);
    const Selection selection = select_transaction(pool, balances, rng, no_ids);
    CHECK(selection.selected.size() == 1);
    CHECK(selection.rejected.size() == 3);
    CHECK(selection.reject_reasons.size() == selection.rejected.size());
    bool reasons_match = selection.reject_reasons.size() == selection.rejected.size();
    for (size_t i = 0; reasons_match && i < selection.rejected.size(); i++)
    {
        const Hash id = selection.rejected[i].get_transaction_id();
        std::string expected = "neteisingas ID";
        if (id == pool[0].get_transaction_id() && selection.rejected[i].get_amount() == 5 &&
            selection.rejected[i].get_sender() == C)
            expected = "siuntejui nepakako lesu";
        else if (id == pool[1].get_transaction_id())
            expected = "nezinomas dalyvis";
        reasons_match = selection.reject_reasons[i] == expected;
    }
    CHECK(reasons_match);

    // Visas ciklas: priezastys patenka i rezultata ta pacia tvarka kaip atmestos transakcijos.
    Blockchain chain(1);
    MiningResult result = run_mining(pool, balances, chain, rng, false);
    CHECK(result.included == 1);
    CHECK(result.rejected.size() == 3);
    CHECK(result.reject_reasons.size() == result.rejected.size());
    CHECK(pool.empty());

    // Kai nieko negalima itraukti, priezastys taip pat issaugomos.
    std::vector<Transaction> hopeless = {make_transaction(C, A, 5, 10), make_transaction(C, B, 7, 11)};
    MiningResult nothing = run_mining(hopeless, balances, chain, rng, false);
    CHECK(nothing.included == 0);
    CHECK(nothing.reject_reasons.size() == 2);
    CHECK(nothing.reject_reasons.size() == 2 && nothing.reject_reasons[0] == "siuntejui nepakako lesu" &&
          nothing.reject_reasons[1] == "siuntejui nepakako lesu");

    // BalanceState::check atskiria visas priezastis ir nieko nekeicia.
    CHECK(balances.check(make_transaction(B, C, 1, 20)) == "");
    CHECK(balances.check(make_transaction(B, B, 1, 21)) == "siuntejas ir gavejas sutampa");
    CHECK(balances.check(make_transaction(B, C, 0, 22)) == "netinkama suma");
    CHECK(balances.check(make_transaction(B, C, 100000, 23)) == "siuntejui nepakako lesu");
    CHECK(balances.check(make_transaction(key("Z"), C, 1, 24)) == "nezinomas dalyvis");
    const Bakugan max = std::numeric_limits<Bakugan>::max();
    std::vector<User> rich = {User("a", A, 10), User("b", B, max - 5)};
    BalanceState near_limit(rich);
    CHECK(near_limit.check(make_transaction(A, B, 10, 25)) == "gavejo balanso perpildymas");
    CHECK(near_limit.check(make_transaction(A, B, 5, 26)) == "");
    CHECK(near_limit.get_balance(A) == 10 && near_limit.get_balance(B) == max - 5);
}

static void test_input_limits()
{
    std::cout << "Ribos: sudetingumas ir mazi balansai\n";

    // Sudetingumo 65 patenkinti neimanoma, todel tokia grandine nesukuriama.
    CHECK(throws_invalid_argument([] { Blockchain chain(max_difficulty + 1); }));
    CHECK(!throws_invalid_argument([] { Blockchain chain(max_difficulty); }));
    CHECK(Blockchain(0).get_difficulty() == 0);

    // v0.2 cia dalijo is nulio: siuntejo balansas mazesnis nei 10.
    const std::vector<User> poor = {User("a", key("A"), 0), User("b", key("B"), 3), User("c", key("C"), 9)};
    const std::vector<Transaction> transactions = generate_transactions(5, poor, 200);
    CHECK(transactions.size() == 200);
    bool all_amount_one = true;
    for (const Transaction &transaction : transactions)
    {
        all_amount_one = all_amount_one && transaction.get_amount() == 1;
    }
    CHECK(all_amount_one);

    // Ir toks rinkinys apdorojamas iki galo, o lesu suma nesikeicia.
    BalanceState balances(poor);
    Blockchain chain(1);
    std::vector<Transaction> pool = transactions;
    std::mt19937_64 rng(1);
    MiningResult result = run_mining(pool, balances, chain, rng, false);
    CHECK(pool.empty());
    CHECK(result.included + result.rejected.size() == 200);
    CHECK(balances.sum() == 12);
}

static std::vector<Candidate> make_candidates(const Blockchain &chain, const std::vector<Transaction> &pool,
                                              const BalanceState &balances, uint64_t seed)
{
    std::mt19937_64 rng(seed);
    std::vector<Candidate> candidates;
    for (const Selection &selection : build_candidates(5, pool, balances, rng, chain.get_transaction_ids()))
    {
        candidates.push_back(Candidate{selection, chain.prepare_block(selection.selected)});
    }
    return candidates;
}

// Kandidatas su pasirinktu sudetingumu: 1 randa greitai, 16 praktiskai niekada.
static Candidate crafted_candidate(uint32_t difficulty, uint64_t variant)
{
    const std::vector<Transaction> transactions = {make_transaction(key("A"), key("B"), 1 + variant, variant)};
    return Candidate{Selection{transactions, {}},
                     Block(Block_h(std::string(64, '0'), "", 1001, 2, difficulty, 0), transactions)};
}

static void test_parallel_matches_sequential()
{
    std::cout << "Lygiagretus kasimas: tas pats laimetojas ir nonce kaip nuosekliame\n";
    std::vector<User> users = generate_users(1);
    const std::vector<Transaction> pool = generate_transactions(2, users, 600);
    BalanceState balances(users);

    bool same_winner = true;
    bool same_nonce = true;
    bool earlier_did_same_work = true;
    bool one_winner_valid = true;
    size_t multi_pass_cases = 0;
    for (uint32_t difficulty = 1; difficulty <= 3; difficulty++)
    {
        Blockchain chain(difficulty);
        for (uint64_t seed = 0; seed < 12; seed++)
        {
            // Mazas pradinis biudzetas, kad daznai prireiktu keliu eiliu.
            const uint64_t start_budget = difficulty == 3 ? 300 : 5;

            std::vector<Candidate> sequential = make_candidates(chain, pool, balances, seed);
            const size_t expected = mine_candidates(sequential, start_budget, 20);
            if (sequential[expected].turns > 1)
                multi_pass_cases++;

            // Tas pats darbas kelis kartus: rezultatas neturi priklausyti nuo giju greicio.
            for (int repeat = 0; repeat < 5; repeat++)
            {
                std::vector<Candidate> parallel = make_candidates(chain, pool, balances, seed);
                const size_t winner = mine_candidates_parallel(parallel, start_budget, 20);

                same_winner = same_winner && winner == expected;
                same_nonce = same_nonce &&
                             parallel[winner].block.get_header().get_nonce() ==
                                 sequential[expected].block.get_header().get_nonce() &&
                             parallel[winner].block.calculateHash() == sequential[expected].block.calculateHash();
                one_winner_valid = one_winner_valid && parallel[winner].found &&
                                   parallel[winner].block.hasValidProofOfWork() && !parallel[winner].stopped;

                // Mazesnio numerio kandidatai atliko lygiai ta pati darba kaip nuosekliame kasime.
                for (size_t i = 0; i < winner && i < expected; i++)
                {
                    earlier_did_same_work = earlier_did_same_work && !parallel[i].found && !parallel[i].stopped &&
                                            parallel[i].attempts == sequential[i].attempts &&
                                            parallel[i].turns == sequential[i].turns &&
                                            parallel[i].block.get_header().get_nonce() ==
                                                sequential[i].block.get_header().get_nonce();
                }
            }
        }
    }
    CHECK(same_winner);
    CHECK(same_nonce);
    CHECK(one_winner_valid);
    CHECK(earlier_did_same_work);
    CHECK(multi_pass_cases > 0);
}

static void test_parallel_stops_and_priority()
{
    std::cout << "Lygiagretus kasimas: kiti sustabdomi, pirmenybe mazesniam numeriui\n";

    // 1 kandidatas randa is karto, kiti niekada. Be sustabdymo jie atliktu po 5 mln. bandymu.
    {
        const uint64_t budget = 5000000;
        std::vector<Candidate> candidates;
        candidates.push_back(crafted_candidate(1, 0));
        for (uint64_t i = 1; i < 5; i++)
        {
            candidates.push_back(crafted_candidate(16, i));
        }
        const size_t winner = mine_candidates_parallel(candidates, budget, 1);
        CHECK(winner == 0);
        CHECK(candidates[0].found && !candidates[0].stopped);
        bool others_stopped = true;
        for (size_t i = 1; i < candidates.size(); i++)
        {
            others_stopped = others_stopped && candidates[i].stopped && !candidates[i].found &&
                             candidates[i].attempts < budget;
        }
        CHECK(others_stopped);
    }

    // Laimi 2 kandidatas: 1-asis privalo baigti visa savo biudzeta, nes rades turetu pirmenybe.
    {
        std::vector<Candidate> candidates;
        candidates.push_back(crafted_candidate(16, 0));
        candidates.push_back(crafted_candidate(1, 1));
        candidates.push_back(crafted_candidate(16, 2));
        const size_t winner = mine_candidates_parallel(candidates, 3000, 1);
        CHECK(winner == 1);
        CHECK(candidates[0].attempts == 3000 && !candidates[0].stopped && !candidates[0].found);
        CHECK(candidates[0].block.get_header().get_nonce() == 3000);
        CHECK(!candidates[2].found);
    }

    // Abu randa per ta pacia eile, antrasis greiciau. Laimeti vis tiek turi pirmasis.
    {
        // Ieskoma pora, kurioje pirmam kandidatui reikia daugiau bandymu nei antram.
        uint64_t slow_variant = 0;
        uint64_t fast_variant = 0;
        uint64_t slow_attempts = 0;
        uint64_t fast_attempts = 0;
        for (uint64_t variant = 0; variant < 200 && (slow_attempts < 400 || fast_attempts == 0); variant++)
        {
            Candidate probe = crafted_candidate(2, variant);
            uint64_t attempts = 0;
            double seconds = 0.0;
            probe.block.mine(attempts, seconds);
            if (attempts >= 400 && slow_attempts < 400)
            {
                slow_variant = variant;
                slow_attempts = attempts;
            }
            else if (attempts <= 20 && fast_attempts == 0)
            {
                fast_variant = variant;
                fast_attempts = attempts;
            }
        }
        CHECK(slow_attempts >= 400 && fast_attempts > 0 && fast_attempts <= 20);

        bool first_always_wins = true;
        for (int repeat = 0; repeat < 40; repeat++)
        {
            std::vector<Candidate> candidates;
            candidates.push_back(crafted_candidate(2, slow_variant));
            candidates.push_back(crafted_candidate(2, fast_variant));
            const size_t winner = mine_candidates_parallel(candidates, 100000, 1);
            first_always_wins = first_always_wins && winner == 0 && candidates[0].attempts == slow_attempts &&
                                candidates[0].block.hasValidProofOfWork();
        }
        CHECK(first_always_wins);
    }

    // Niekas neranda: po leistinu eiliu metama klaida, biudzetas dvigubintas, tuscias kandidatas praleistas.
    {
        std::vector<Candidate> candidates;
        candidates.push_back(crafted_candidate(16, 0));
        candidates.push_back(Candidate{Selection{}, Block(Block_h(std::string(64, '0'), "", 1001, 2, 16, 0), {})});
        candidates.push_back(crafted_candidate(16, 2));
        bool threw = false;
        try
        {
            mine_candidates_parallel(candidates, 5, 2);
        }
        catch (const std::runtime_error &)
        {
            threw = true;
        }
        CHECK(threw);
        CHECK(candidates[0].attempts == 15 && candidates[0].block.get_header().get_nonce() == 15);
        CHECK(candidates[1].attempts == 0 && candidates[1].turns == 0);
        CHECK(candidates[2].attempts == 15 && candidates[2].turns == 2);
    }
}

struct RunResult
{
    Hash tip;
    size_t blocks = 0;
    size_t included = 0;
    size_t rejected = 0;
    Bakugan total = 0;
};

static RunResult run_chain(uint64_t count, uint32_t difficulty, uint64_t selection_seed, MiningMode mode)
{
    std::vector<User> users = generate_users(1);
    std::vector<Transaction> pool = generate_transactions(2, users, count);
    BalanceState balances(users);
    Blockchain chain(difficulty);
    std::mt19937_64 rng(selection_seed);
    MiningResult result = run_mining(pool, balances, chain, rng, false, mode);

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
    std::cout << "Ta pati grandine kaip v0.2 (7 duomenu rinkiniai, nuosekliai ir lygiagreciai)\n";
    for (MiningMode mode : {MiningMode::sequential, MiningMode::parallel})
    {
        for (const Golden &golden : golden_runs)
        {
            const RunResult run = run_chain(golden.count, golden.difficulty, golden.selection_seed, mode);
            CHECK(run.tip == golden.tip);
            CHECK(run.blocks == golden.blocks);
            CHECK(run.included == golden.included);
            CHECK(run.rejected == golden.rejected);
            CHECK(run.total == 498753171);
        }
    }
}

int main()
{
    test_budget_is_exact();
    test_hash_stream_matches_hash();
    test_fast_mining_matches_slow_check();
    test_merkle_module();
    test_block_rejection_reasons();
    test_selection_reasons();
    test_input_limits();
    test_parallel_matches_sequential();
    test_parallel_stops_and_priority();
    test_same_chain_as_v02();

    std::cout << "\nv0.3 patikros: " << checks << ", nepavyko: " << failures << "\n";
    return failures == 0 ? 0 : 1;
}
