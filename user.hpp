#pragma once

#include <string>
#include <utility>
#include <cstdint>
#include <array>

using std::string;
using Hash = std::array<uint32_t, 8>;
class User{

private:
    string name;
    Hash public_key;
    uint64_t balance;
public:
    User(string n, Hash id, uint64_t bal)
        : name(std::move(n)), public_key(id), balance(bal) 
    {} 

    void set_name(string& n) { name = n; }
    void set_id(u_int64_t& i) { public_key = i; }
    void set_balance(uint64_t& b) { balance = b; }

    string get_name() { return name; }
    Hash get_key() { return public_key; }
    uint64_t get_balance() { return balance; }
};