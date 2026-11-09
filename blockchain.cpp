#include "blockchain.hpp"
#include "block_h.hpp"
#include "validation.hpp"
#include <stdexcept>

Blockchain::Blockchain(uint32_t difficulty) : difficulty(difficulty)
{
    if (difficulty > max_difficulty)
        throw std::invalid_argument("Sudetingumas negali virsyti 64: maisa turi tik 64 hex simbolius");

    create_genesis_block();
}

void Blockchain::create_genesis_block()
{
    Hash zero_hash = std::string(64, '0');
    Hash genesis_tx_root = "";

    Block_h genesis_header(
        zero_hash,
        genesis_tx_root,
        0,
        2,
        difficulty,
        0);

    Block genesis_block(genesis_header, {});

    blocks.push_back(genesis_block);
}

const Block &Blockchain::tip() const
{

    return blocks.back();
}

size_t Blockchain::height() const
{
    return blocks.size() - 1;
}

uint32_t Blockchain::get_difficulty() const
{
    return difficulty;
}

std::string Blockchain::validate_block(const Block &block, BalanceState &state, std::set<Hash> &ids) const
{
    const Block_h &header = block.get_header();

    if (header.get_prev_hash() != tip().calculateHash())
        return "neteisingas prev_hash";

    if (header.get_version() != supported_block_version)
        return "nepalaikoma versija";

    const auto &transactions = block.get_transactions();
    if (transactions.empty())
        return "blokas be transakciju";

    if (transactions.size() > max_block_transactions)
        return "per daug transakciju";

    if (block.calculateTxRoot() != header.get_tx_root())
        return "neteisinga tx_root";

    // Sudetinguma nustato grandine: kandidatas negali jo susimazinti.
    if (header.get_difficulty() != difficulty)
        return "neteisingas sudetingumas";

    // Maisa perskaiciuojama is antrastes lauku.
    if (!block.hasValidProofOfWork())
        return "netinkamas PoW";

    // Transakcijos tikrinamos ir pritaikomos ju tvarka bloke.
    for (size_t i = 0; i < transactions.size(); i++)
    {
        const Transaction &transaction = transactions[i];

        std::string error = validate_transaction(state, transaction, ids);
        if (error.empty())
            error = state.check(transaction);

        if (!error.empty())
            return "transakcija " + std::to_string(i + 1) + ": " + error;

        state.apply(transaction);
        ids.insert(transaction.get_transaction_id());
    }

    return "";
}

std::string Blockchain::check_block(const Block &block, const BalanceState &balances) const
{
    BalanceState state(balances);
    std::set<Hash> ids = known_transaction_ids;
    return validate_block(block, state, ids);
}

bool Blockchain::add_block(const Block &block, BalanceState &balances, std::string *reason)
{
    BalanceState state(balances);
    std::set<Hash> ids = known_transaction_ids;

    const std::string error = validate_block(block, state, ids);
    if (reason != nullptr)
        *reason = error;

    if (!error.empty())
        return false;

    // Visos patikros praejo: grandine, balansai ir ID aibe keiciami kartu.
    blocks.push_back(block);
    balances = state;
    known_transaction_ids = ids;
    return true;
}

Block Blockchain::minePendingTransactions(const std::vector<Transaction> &selected,
                                          uint64_t &attempts,
                                          double &elapsed_seconds) const
{

    Block new_block = prepare_block(selected);

    new_block.mine(attempts, elapsed_seconds);

    return new_block;
}

Block Blockchain::prepare_block(const std::vector<Transaction> &selected) const
{
    const uint64_t base_timestamp = 1000;
    const uint64_t timestamp = base_timestamp + height() + 1;
    Block new_block(
        Block_h(
            tip().calculateHash(),
            "",
            timestamp,
            2,
            difficulty,
            0),
        selected);

    return new_block;
}