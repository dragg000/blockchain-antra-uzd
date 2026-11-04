#pragma once

#include <string>
#include <utility>
#include <cstdint>
#include <array>

using std::string;
using Hash = std::string;
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
    void set_public_key(Hash& key) { public_key = key; }
    void set_balance(uint64_t& b) { balance = b; }

    string get_name() { return name; }
    Hash get_key() { return public_key; }
    uint64_t get_balance() { return balance; }
};