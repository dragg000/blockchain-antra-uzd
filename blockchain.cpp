#include "blockchain.hpp"
#include "block_h.hpp"

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
        1,
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

bool Blockchain::add_block(const Block &block)
{

    if (block.get_header().get_prev_hash() != tip().calculateHash())
        return false;

    if (!block.hasValidProofOfWork())
        return false;

    blocks.push_back(block);

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
            1,
            difficulty,
            0),
        selected);

    new_block.mine(attempts, elapsed_seconds);

    return new_block;
}