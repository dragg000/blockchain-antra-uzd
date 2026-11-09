#include "validation.hpp"
#include "hash.hpp"

bool isHex64(std::string_view s)
{
    if (s.size() != 64)
        return false;
    for (char c : s)
    {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
    }
    return true;
}

std::string validate_transaction(const BalanceState &balance,
                                 const Transaction &transaction,
                                 const std::set<Hash> &known_ids)
{
    const Key sender = transaction.get_sender();
    const Key receiver = transaction.get_receiver();
    const Bakugan amount = transaction.get_amount();
    const Hash id = transaction.get_transaction_id();

    if (!isHex64(sender) || !isHex64(receiver) || !isHex64(id))
    {
        return "netinkamas formatas";
    }

    if (sender == receiver)
    {
        return "siuntejas ir gavejas sutampa";
    }

    if (!balance.has_user(sender) || !balance.has_user(receiver))
    {
        return "nezinomas dalyvis";
    }

    // uint64_t can't hold a negative amount: -1 becomes a huge number, which the second check refuses.
    if (amount == 0 || amount > balance.get_initial_total())
    {
        return "netinkama suma";
    }

    if (hashTextUtf8(transaction.serialize()) != id)
    {
        return "neteisingas ID";
    }

    if (known_ids.count(id) > 0)
    {
        return "pasikartojantis ID";
    }

    return "";
}

std::string add_transaction_to_pool(std::vector<Transaction> &pool, std::set<Hash> &pool_ids,
                                    const Transaction &transaction,
                                    const BalanceState &balances, const std::set<Hash> &chain_ids)
{
    std::string error = validate_transaction(balances, transaction, chain_ids);

    if (error.empty() && pool_ids.count(transaction.get_transaction_id()) > 0)
    {
        error = "pasikartojantis ID";
    }

    if (!error.empty())
    {
        return error;
    }

    pool.push_back(transaction);
    pool_ids.insert(transaction.get_transaction_id());
    return "";
}
