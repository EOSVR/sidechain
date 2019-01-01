
#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include "eosio.token.hpp"
#include <string>

using namespace eosio;

using std::string;

// Save the data of investor.
// When account is 0, deposit is total of all original investors. (Do not include _self (eoslocktoken))
// When account is _self, deposit is total of all bonus.
struct deposits {
  account_name account;
  int64_t deposit;

  auto primary_key() const { return account; }

  EOSLIB_SERIALIZE( deposits, (account)(deposit))
};

typedef eosio::multi_index<N(depositss1), deposits> depositTable1;

typedef eosio::multi_index<N(depositss2), deposits> depositTable2;
