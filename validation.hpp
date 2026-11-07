#pragma once

#include <set>
#include <string>
#include <string_view>
#include "balance_state.hpp"
#include "transaction.hpp"

struct RefusedTransaction
{
    Transaction transaction;
    std::string reason;
};

bool isHex64(std::string_view s);

// Returns an empty string if the transaction may enter the pool, otherwise the reason it can't.
std::string validate_transaction(const BalanceState &balance,
                                 const Transaction &transaction,
                                 const std::set<Hash> &known_ids);

std::string add_transaction_to_pool(std::vector<Transaction> &pool, const Transaction &transaction, const BalanceState &balances, const std::set<Hash> &chain_ids);