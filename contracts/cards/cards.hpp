
#include "../locktoken/locktoken.hpp"


using namespace eosio;

using std::string;


struct cards {
    int64_t   id;
    name creator;
    int64_t   mark;
    int64_t   total;
    int64_t   sell;
    int64_t   price;
    int64_t   lastupdate;
    string    content;

    int64_t primary_key()const { return id; }

    EOSLIB_SERIALIZE( cards, (id)(creator)(mark)(total)(sell)(price)(lastupdate)(content) );
};

typedef eosio::multi_index<"cardss"_n, cards> cardTable;

struct reg {
    int64_t   id;
    name from;
    int64_t   total;
    int64_t   sell;
    int64_t   price;
    string    content;

    EOSLIB_SERIALIZE( reg, (id)(from)(total)(sell)(price)(content));
};
