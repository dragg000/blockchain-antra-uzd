#pragma once

#include <unordered_map>
#include <limits>
#include <string>
#include "transaction.hpp"
#include "user.hpp"

class BalanceState
{

private:
    std::unordered_map<Key, Bakugan> balances;
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

    // Ar transakcija tilptu i sia busena. Grazina tuscia eilute arba priezasti. Nieko nekeicia.
    std::string check(const Transaction &transaction) const
    {
        const Key &sender = transaction.get_sender();
        const Key &receiver = transaction.get_receiver();
        const Bakugan amount = transaction.get_amount();

        const auto from = balances.find(sender);
        const auto to = balances.find(receiver);
        if (from == balances.end() || to == balances.end())
        {
            return "nezinomas dalyvis";
        }

        if (sender == receiver)
        {
            return "siuntejas ir gavejas sutampa";
        }

        if (amount == 0)
        {
            return "netinkama suma";
        }

        if (from->second < amount)
        {
            return "siuntejui nepakako lesu";
        }

        if (to->second > std::numeric_limits<Bakugan>::max() - amount)
        {
            return "gavejo balanso perpildymas";
        }

        return "";
    }

    // Pritaiko transakcija, jei check() neranda kliuties. Kitu atveju nieko nekeicia.
    bool apply(const Transaction &transaction)
    {
        if (!check(transaction).empty())
        {
            return false;
        }

        balances[transaction.get_sender()] -= transaction.get_amount();
        balances[transaction.get_receiver()] += transaction.get_amount();

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