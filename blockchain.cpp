#include "blockchain.hpp"
#include "block_h.hpp"
#include "validation.hpp"

Blockchain::Blockchain(uint32_t difficulty) : difficulty(difficulty)
{
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

const std::set<Hash> &Blockchain::get_transaction_ids() const
{
    return known_transaction_ids;
}

bool Blockchain::add_block(const Block &block, BalanceState &balances)
{

    const Block_h &header = block.get_header();

    if (header.get_prev_hash() != tip().calculateHash())
        return false;

    if (header.get_version() != 2)
        return false;

    const auto &transactions = block.get_transactions();
    if (transactions.empty() || transactions.size() > 100)
        return false;

    if (block.calculateTxRoot() != header.get_tx_root())
        return false;

    if (header.get_difficulty() != difficulty)
        return false;

    if (!block.hasValidProofOfWork())
        return false;

    BalanceState tmp(balances);

    std::set<Hash> new_ids = known_transaction_ids;

    for (const Transaction &transaction : transactions)
    {
        std::string error = validate_transaction(tmp, transaction, new_ids);
        if (!error.empty())
        {
            return false;
        }

        if (!tmp.apply(transaction))
            return false;

        new_ids.insert(transaction.get_transaction_id());
    }

    blocks.push_back(block);
    balances = tmp;
    known_transaction_ids = new_ids;
    return true;
}

Block Blockchain::minePendingTransactions(const std::vector<Transaction> &selected,
                                          uint64_t &attempts,
                                          double &elapsed_seconds) const
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

    new_block.mine(attempts, elapsed_seconds);

    return new_block;
}