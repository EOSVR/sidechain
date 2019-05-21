
#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include <string>
#include <eosiolib/asset.hpp>

#define N(_A_) "_A_"_n.value
#define account_name uint64_t

using namespace eosio;

using std::string;

//#include <eosiolib/currency.hpp>
// Add Old currency:transfer support
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


