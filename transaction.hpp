#include "user.hpp"

class Transaction {
private:
    User sender;
    User receiver;
    uint64_t amount;
    Hash transaction_id;
    uint64_t sequence;

public:
    Transaction(User s, User r, uint64_t a, uint64_t id, uint64_t seq)
        : sender(std::move(s)), receiver(std::move(r)),
            amount(a), transaction_id(id), sequence(seq)
    {}
    void set_sender(User s) { sender = s; }
    void set_receiver(User r) { receiver = r; }
    void set_amount(uint64_t a) { amount = a; }
    void set_transaction_id(uint64_t id) { transaction_id = id; }
    void set_sequence(uint64_t seq) { sequence = seq; }

    User get_sender() const { return sender; }
    User get_receiver() const { return receiver; }
    uint64_t get_amount() const { return amount; }
    hash get_transaction_id() const { return transaction_id; }
    uint64_t get_sequence() const { return sequence; }
};