#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/transaction.hpp>

#include <string>

#define account_name name


namespace eosiosystem {
   class system_contract;
}

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
      
         void confirm(  name from,
                        name to,
                        string       key,
                        name executer);


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

         // When timeout == 0, quantity = token taken limit per day (thousand of), from = current time limit
         struct timelocks {
            uint64_t timeout;

            int64_t quantity; // limit

            int64_t from; // currentlimit

            uint64_t primary_key()const { return timeout; }

         };

         struct lockaccounts {
            name from;
            int64_t quantity;
            string memo;
            int64_t reverse;
            int64_t lastupdate;

            uint64_t primary_key()const { return from.value; }
            uint64_t second_key()const { return INT64_MAX - std::abs(quantity); }
            uint64_t third_key()const { return INT64_MAX - lastupdate; }
         };

         struct hashlocks {
            uint64_t mix;
            account_name from;
            account_name to;
            int64_t quantity;
            capi_checksum256 hash;
            uint64_t timeout;
            string memo;

            uint64_t primary_key()const { return mix; }
            uint64_t second_key()const { return from.value; }
            uint64_t third_key()const { return to.value; }
         };

         // List previous revealed hash, it can be cleared after timeout.
         struct hashlist {
            int64_t timeout;
            capi_checksum256 hash;
            string key;

            uint64_t primary_key()const { return timeout; }
            capi_checksum256 second_key()const { return hash; }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         typedef eosio::multi_index< "lockss"_n, lockaccounts
            , indexed_by< "byquantity"_n, const_mem_fun< lockaccounts, uint64_t, &lockaccounts::second_key> >
            , indexed_by< "bytime"_n, const_mem_fun< lockaccounts, uint64_t, &lockaccounts::third_key> >
            > lockaccountTable;

         typedef eosio::multi_index<
            "hashlockss"_n, hashlocks,
            indexed_by< "byfrom"_n, const_mem_fun< hashlocks, uint64_t, &hashlocks::second_key> >,
            indexed_by< "byto"_n, const_mem_fun< hashlocks, uint64_t, &hashlocks::third_key> >
            > hashlockTable;

         // When transfer to _self, save in this table for reference (used in side-chain, memo is chain-id)
         typedef eosio::multi_index<"depositss"_n, lockaccounts
            , indexed_by< "byquantity"_n, const_mem_fun< lockaccounts, uint64_t, &lockaccounts::second_key> >
                 , indexed_by< "bytime"_n, const_mem_fun< lockaccounts, uint64_t, &lockaccounts::third_key> >
            > depositTable;

         typedef eosio::multi_index<"hashss"_n, hashlist
            > hashListTable;



         int64_t sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
         void add_balance2( name owner, int64_t value, name ram_payer );

         int64_t change_lock( name from, name to, int64_t amount, string memo );
         int64_t change_lock_from( name from, name to, int64_t amount, int64_t reverse, string memo );
         void change_lock_main( name from, name to, int64_t amount );

         void add_time_lock( name owner, int64_t amount, int64_t delay,  name ram_payer );
         void refresh_tokens(name from);
         void add_day_limit(name from, int64_t amount, int64_t remain);
         void set_limit(name owner, int64_t new_limit);

         int64_t confirm_lock( name from, name to );
         int64_t remove_lock_from( name from, name to );

         void createTableIfEmpty(name from, name to, name ram_payer, int64_t reverse );
         uint64_t removeTableIfEmpty(uint64_t from, uint64_t to );

         void add_hash_record(name from, name to, asset quantity, string hash_memo);

         void save_deposit( name from, asset quantity, string memo, name ram_payer);

      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };

         typedef eosio::multi_index<"timelockss"_n, timelocks> timelockTable;
   };


} /// namespace eosio
