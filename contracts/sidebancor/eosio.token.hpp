#pragma once

/*
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/transaction.hpp>


#define account_name name


namespace eosiosystem {
   class system_contract;
}
*/

#include <string>

using namespace eosio;

namespace eosio {

   using std::string;

   class token : public contract {
      public:
         using contract::contract;

         void create( name   issuer,
                      asset  maximum_supply);

         void issue( name to, asset quantity, string memo );

         void transfer( name from,
                        name to,
                        asset        quantity,
                        string       memo );
      
      
         static asset get_supply( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }
         
         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

      private:
         struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            name   issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );

      public:
         struct transfer_args {
            name  from;
            name  to;
            asset         quantity;
            string        memo;
         };
   };


} /// namespace eosio
