#pragma once

#include <map>
#include <limits>
#include "transaction.hpp"
#include "user.hpp"

class BalanceState
{

private:
    std::map<Key, Bakugan> balances;
    Bakugan initial_total;

public:
    BalanceState(const std::vector<User> &users)
    {
        for (const User &user : users)
        {
            balances[user.get_key()] = user.get_balance();
        }

        initial_total = sum();
    }

    bool apply(const Transaction &transaction)
    {
        const Key sender = transaction.get_sender();
        const Key receiver = transaction.get_receiver();
        const Bakugan amount = transaction.get_amount();

        if (balances.find(sender) == balances.end() || balances.find(receiver) == balances.end())
        {
            return false;
        }

        if (sender == receiver || amount == 0)
        {
            return false;
        }

        if (balances[sender] < amount)
        {
            return false;
        }

        if (balances[receiver] > std::numeric_limits<Bakugan>::max() - amount)
        {
            return false;
        }

        balances[sender] = balances[sender] - amount;
        balances[receiver] = balances[receiver] + amount;

        return true;
    }

    Bakugan sum() const
    {
        Bakugan total = 0;
        for (const auto &i : balances)
        {
            total += i.second;
        }
        return total;
    }

    Bakugan get_balance(const Key &key) const
    {
        return balances.at(key);
    }

    Bakugan get_initial_total() const
    {
        return initial_total;
    }

    bool has_user(const Key &key) const
    {
        return balances.find(key) != balances.end();
    }
};