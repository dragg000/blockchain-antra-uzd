#include <string>
using std::string;
class User{

private:
    string name;
    uint64_t unique_id;
    uint64_t balance;
public:
    User(string n, uint64_t id, uint64_t bal)
        : name(std::move(n)), unique_id(id), balance(bal) 
    {} 

    void set_name(string& n) { name = n; }
    void set_id(u_int64_t& i) { unique_id = i; }
    uint64_t set_balance(uint64_t& b) { balance = b; }

    string get_name() { return name; }
    uint64_t get_id() { return unique_id; }
    uint64_t get_balance() { return balance; }
};