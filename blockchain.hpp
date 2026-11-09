#ifndef BLOCKCHAIN_HPP
#define BLOCKCHAIN_HPP

#include <vector>
#include "block_h.hpp"
#include "balance_state.hpp"
#include <set>
#include <string>

// Bloko taisykles, kurias nustato grandine, o ne kandidatas.
constexpr size_t max_block_transactions = 100;
constexpr uint32_t supported_block_version = 2;
// Maisa turi 64 hex simbolius, todel didesnio sudetingumo patenkinti neimanoma.
constexpr uint32_t max_difficulty = 64;

class Blockchain
{
private:
    std::vector<Block> blocks; // blocks[0] is genesis
    uint32_t difficulty;       // the chain sets this, not the candidate
    std::set<Hash> known_transaction_ids;

    void create_genesis_block();

    // Visos bloko patikros. `state` ir `ids` yra kopijos, kuriose pritaikomos bloko transakcijos.
    // Grazina tuscia eilute arba atmetimo priezasti.
    std::string validate_block(const Block &block, BalanceState &state, std::set<Hash> &ids) const;

public:
    // Sukuria genesis. Meta std::invalid_argument, jei difficulty > max_difficulty.
    Blockchain(uint32_t difficulty);

    const Block &tip() const; // the newest block
    const Block &get_block(size_t height) const { return blocks.at(height); }
    size_t height() const;
    uint32_t get_difficulty() const;
    const std::vector<Block> &get_blocks() const { return blocks; }
    const std::set<Hash> &get_transaction_ids() const { return known_transaction_ids; }

    // Ar bloka butu galima prideti dabar. Grazina tuscia eilute arba priezasti, nieko nekeicia.
    //   neteisingas prev_hash, nepalaikoma versija, blokas be transakciju, per daug transakciju,
    //   neteisinga tx_root, neteisingas sudetingumas, netinkamas PoW,
    //   transakcija N: <transakcijos atmetimo priezastis>
    std::string check_block(const Block &block, const BalanceState &balances) const;

    // Prideda bloka tik jei jis praeina visas patikras. Kitu atveju nieko nekeicia,
    // o priezastis irasoma i *reason, jei rodykle nurodyta.
    bool add_block(const Block &block, BalanceState &balances, std::string *reason = nullptr);

    // mines one block on top of the tip, without adding it to the chain
    Block minePendingTransactions(const std::vector<Transaction> &selected,
                                  uint64_t &attempts,
                                  double &elapsed_seconds) const;
    
    Block prepare_block(const std::vector<Transaction> &selected) const;
};

#endif