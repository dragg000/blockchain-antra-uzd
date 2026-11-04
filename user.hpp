#pragma once

#include <string>
#include <utility>
#include <cstdint>
#include <array>
#include <vector>

using std::string;
using Hash = std::string;

using Key = Hash;
using Bakugan = uint64_t; //name of coin
class User{

private:
    string name;
    Key public_key;
    Bakugan balance;
public:
    User(string n, Hash id, uint64_t bal)
        : name(std::move(n)), public_key(id), balance(bal) 
    {} 

    void set_name(string& n) { name = n; }
    void set_public_key(Key& key) { public_key = key; }
    void set_balance(Bakugan& b) { balance = b; }

    string get_name() const { return name; }
    Key get_key() const { return public_key; }
    Bakugan get_balance() const { return balance; }
};

std::vector<User> generate_users(uint64_t seed);