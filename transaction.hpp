#pragma once
#include "user.hpp"

class Transaction {
private:
    Hash sender_key;
    Hash receiver_key;
    uint64_t amount;
    Hash transaction_id;
    uint64_t sequence;

public:
    Transaction(Hash s, Hash r, uint64_t a, Hash id, uint64_t seq)
        : sender_key(std::move(s)), receiver_key(std::move(r)),
            amount(a), transaction_id(id), sequence(seq)
    {}
    void set_sender(Hash s) { sender_key = s; }
    void set_receiver(Hash r) { receiver_key = r; }
    void set_amount(uint64_t a) { amount = a; }
    void set_transaction_id(Hash id) { transaction_id = id; }
    void set_sequence(uint64_t seq) { sequence = seq; }

    Hash get_sender() const { return sender_key; }
    Hash get_receiver() const { return receiver_key; }
    uint64_t get_amount() const { return amount; }
    Hash get_transaction_id() const { return transaction_id; }
    uint64_t get_sequence() const { return sequence; }


    std::string serialize() const 
    {
        return sender_key + "|" + receiver_key + "|" +
        std::to_string(amount) + "|" + std::to_string(sequence);
    }
};