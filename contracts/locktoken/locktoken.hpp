#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/transaction.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

using namespace eosio;

namespace eosio {

   using std::string;

   class token : public contract {
      public:
         token( account_name self ):contract(self){}

         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );
      
         void confirm(  account_name from,
                        account_name to,
                        string       key,
                        account_name executer);


         inline asset get_supply( symbol_name sym )const;
         
         inline asset get_balance( account_name owner, symbol_name sym )const;


      private:
         struct account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.name(); }
         };

         struct currency_stats {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };

         // When timeout == 0, quantity = token taken limit per day (thousand of), from = current time limit
         struct timelocks {
            uint64_t timeout;

            int64_t quantity; // limit

            int64_t from; // currentlimit

            uint64_t primary_key()const { return timeout; }

         };

         struct lockaccounts {
            account_name from;
            int64_t quantity;
            string memo;
           
            uint64_t primary_key()const { return from; }
            uint64_t second_key()const { return INT64_MAX - std::abs(quantity); }
         };

         struct hashlocks {
            uint64_t mix;
            account_name from;
            account_name to;
            int64_t quantity;
            checksum256 hash;
            uint64_t timeout;
            string memo;

            uint64_t primary_key()const { return mix; }
            uint64_t second_key()const { return from; }
            uint64_t third_key()const { return to; }
         };

         // List previous revealed hash, it can be cleared after timeout.
         struct hashlist {
            int64_t timeout;
            checksum256 hash;
            string key;

            uint64_t primary_key()const { return timeout; }
            checksum256 second_key()const { return hash; }
         };

         typedef eosio::multi_index<N(accounts), account> accounts;
         typedef eosio::multi_index<N(stat), currency_stats> stats;

         typedef eosio::multi_index<N(lockss), lockaccounts
            , indexed_by< N( byquantity ), const_mem_fun< lockaccounts, uint64_t, &lockaccounts::second_key> >
            > lockaccountTable;

         typedef eosio::multi_index<
            N(hashlockss), hashlocks,
            indexed_by< N( byfrom ), const_mem_fun< hashlocks, uint64_t, &hashlocks::second_key> >,
            indexed_by< N( byto ), const_mem_fun< hashlocks, uint64_t, &hashlocks::third_key> >
            > hashlockTable;

         // When transfer to _self, save in this table for reference (used in side-chain, memo is chain-id)
         typedef eosio::multi_index<N(depositss), lockaccounts
            , indexed_by< N( byquantity ), const_mem_fun< lockaccounts, uint64_t, &lockaccounts::second_key> >
            > depositTable;

         typedef eosio::multi_index<N(hashss), hashlist
            > hashListTable;



         int64_t sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );
         void add_balance2( account_name owner, int64_t value, account_name ram_payer );

         int64_t change_lock( account_name from, account_name to, int64_t amount, string memo );
         int64_t change_lock_from( account_name from, account_name to, int64_t amount, string memo );
         void change_lock_main( account_name from, account_name to, int64_t amount );

         void add_time_lock( account_name owner, int64_t amount, int64_t delay,  account_name ram_payer );
         void refresh_tokens(account_name from);
         void add_day_limit(account_name from, int64_t amount, int64_t remain);
         void set_limit(account_name owner, int64_t new_limit);

         int64_t confirm_lock( account_name from, account_name to );
         int64_t remove_lock_from( account_name from, account_name to );

         void createTableIfEmpty(uint64_t from, uint64_t to, uint64_t ram_payer );
         uint64_t removeTableIfEmpty(uint64_t from, uint64_t to );

         void add_hash_record(account_name from, account_name to, asset quantity, string hash_memo);

         void save_deposit( account_name from, asset quantity, string memo, account_name ram_payer);

      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };

         typedef eosio::multi_index<N(timelockss), timelocks> timelockTable;
   };

   asset token::get_supply( symbol_name sym )const
   {
      stats statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset token::get_balance( account_name owner, symbol_name sym )const
   {
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

} /// namespace eosio
