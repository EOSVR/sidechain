
#include "../locktoken/locktoken.hpp"


using namespace eosio;

using std::string;


struct cards {
    int64_t   id;  // = create_time
    name creator;
    int64_t   mark;
    int64_t   max_supply;      // Max number of this item (Only when creator = tableName)
    int64_t   total;           // Total number of this item
    int64_t   sell;            // Total number of this item to sell
    int64_t   price;
    int64_t   lastupdate;
    string    content;
    int64_t   rampayed; // 0.1 EOS / K , EOS:EVD = 1: 100 => 100EVD = 10K => 1 EVD = 100byte => 100 EVDS = 1 byte

    uint64_t primary_key()const { return id; }

    uint64_t second_key()const { return mark; }

    EOSLIB_SERIALIZE( cards, (id)(creator)(mark)(max_supply)(total)(sell)(price)(lastupdate)(content)(rampayed) );
};

typedef eosio::multi_index<"cardss"_n, cards,
        indexed_by< "bymark"_n,
                const_mem_fun< cards, uint64_t, &cards::second_key> >

> cardTable;

struct reg {
    int64_t   id;
    name from;
    int64_t   total;
    int64_t   sell;
    int64_t   price;
    string    content;

    EOSLIB_SERIALIZE( reg, (id)(from)(total)(sell)(price)(content));
};

class currency {
public:
    struct transfer
    {
        uint64_t from;
        uint64_t to;
        asset        quantity;
        string       memo;

        EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(memo) )
    };
};
