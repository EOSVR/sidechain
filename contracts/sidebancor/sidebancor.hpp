#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include <string>
#include <eosiolib/asset.hpp>

#define account_name name
using namespace eosio;

using std::string;

// Save the data of investor.
// When account is 0, deposit is total of all original investors. (Do not include _self (eoslocktoken))
// When account is _self, deposit is total of all bonus.
struct deposits {
  account_name account;
  int64_t deposit;

  uint64_t primary_key() const { return account.value; }

  EOSLIB_SERIALIZE( deposits, (account)(deposit))
};

typedef eosio::multi_index<"depositss1"_n, deposits> depositTable1;

typedef eosio::multi_index<"depositss2"_n, deposits> depositTable2; 
