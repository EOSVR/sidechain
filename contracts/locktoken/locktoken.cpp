#include "locktoken.hpp"

const auto MY_TOKEN_SYMBOL = symbol(symbol_code("EVD"), 4);

const uint64_t useconds_per_sec = 1000000;

const uint64_t seconds_per_day = 3600 * 24;
const uint64_t useconds_per_day = seconds_per_day * useconds_per_sec;

const uint64_t seconds_per_month = 3600 * 24 * 30;
const uint64_t useconds_per_month = seconds_per_month * useconds_per_sec;

const uint64_t max_delay = 3600 * 24 * uint64_t(365) * 100; // 100 years

const uint64_t max_limit = 50; // Must be 1% - 50% per month

const uint64_t confirm_receive_percent = 10; // After confirm the lock, locker can get 90%, locked account can get 10%.

const uint64_t MIN_DEPOSIT = 10000000; // At least , deposit 1000 EVD to eoslocktoken at the first time.

namespace eosio {

// === Related functions ===

void printSha256(string txt) {
    capi_checksum256 hash;

    sha256( txt.c_str(), txt.size(), &hash );

    printhex( &hash, sizeof(hash) );
}

bool begin_with(string memo, string str) {
    auto ind = memo.find(str);
    return (ind == 0);
}

int64_t get_param_int(string memo, string str, int64_t defaultValue) {
    auto ind = memo.find(str);
    if (ind == 0) {
        return std::stoi(memo.substr(ind + str.length()));
    }

    return defaultValue;
}

uint8_t from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    eosio_assert(false, "Invalid hex character");
    return 0;
}

bool isEqual(capi_checksum256 c1, capi_checksum256 c2) {
    for(int i = 0 ; i < 32; i++) {
        if (c1.hash[i] != c2.hash[i]) return false;
    }

    return true;
}


size_t from_hex(const std::string& hex_str, char* out_data, size_t out_data_len) {
    auto i = hex_str.begin();
    uint8_t* out_pos = (uint8_t*)out_data;
    uint8_t* out_end = out_pos + out_data_len;
    while (i != hex_str.end() && out_end != out_pos) {
        *out_pos = from_hex((char)(*i)) << 4;
        ++i;
        if (i != hex_str.end()) {
            *out_pos |= from_hex((char)(*i));
            ++i;
        }
        ++out_pos;
    }
    return out_pos - (uint8_t*)out_data;
}

capi_checksum256 hex_to_sha256(const std::string& hex_str) {
    eosio_assert(hex_str.length() == 64, "invalid sha256");
    capi_checksum256 checksum;
    from_hex(hex_str, (char*)(checksum.hash), sizeof(checksum.hash));
    return checksum;
}

// =========================


void token::create( name   issuer,
                    asset  maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( _self, sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
      SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
	       {st.issuer, to, quantity, memo} );
    }

    save_deposit( to, quantity, "Issued", _self);
}

void token::set_limit(name owner, int64_t new_limit) {
    check((new_limit > 0 && new_limit <= max_limit), "Limit must be 1-50" );

    timelockTable timelocks(_self, owner.value);
    auto limit0 = timelocks.find(0);

    if (limit0 == timelocks.end()) {
        timelocks.emplace( owner, [&]( auto& a ){
            a.from = 0;
            a.quantity = new_limit; // limit
            a.timeout = 0;
        });
    } else {
        check( new_limit < limit0->quantity, "Limit can only set to smaller than old");

        timelocks.modify( limit0, same_payer, [&]( auto& a ) {
            a.quantity = new_limit; // Set new limit
        });
    }
}

void token::add_day_limit(name from, int64_t amount, int64_t remain) {
    timelockTable timelocks(_self, from.value);

    // Use timelocks.find(0) as limitation
    // limit0->from is current time limit. It must lower than (now + one month)
    // limit0->quantity is current limitation (1-50).
    auto limit0 = timelocks.find(0);
    if (limit0 != timelocks.end()) {

        auto l1 = limit0->quantity;
        int64_t timeadd;
        auto time1 = current_time();
        int64_t time_limit;

        if (l1 > 0 && l1 <= max_limit) {

            int64_t limit_token_per_month = l1 * remain / 100;
            eosio_assert (amount <= limit_token_per_month, "Exceed limitation !");

            timeadd = 1.0 * amount * seconds_per_month / limit_token_per_month * useconds_per_sec;
            time_limit = time1 + useconds_per_month;


            if (limit0->from > time1) {
                time1 = limit0->from;
            }

            time1 += timeadd;

            eosio_assert (time1 <= time_limit, "Exceed limitation");

            timelocks.modify( limit0, same_payer, [&]( auto& a ) {
                a.from = time1; // currentlimit
            });

        }
    }

}

void token::refresh_tokens(name owner) {

    timelockTable timelocks(_self, owner.value);

    auto timeout = current_time();

    auto added = 0;

    auto item = timelocks.begin();
    while (item != timelocks.end() ) {
        if (item->timeout > 0) {
            if (item->timeout < timeout) {
                auto old = item;
                item++;
                added += old->quantity;
                timelocks.erase(old);
            } else {
                break;
            }
        } else {
            item++;
        }
    }

    if (added > 0)
        add_balance2( owner, added, owner);
}

void token::add_hash_record(account_name from, account_name to, asset quantity, string hash_memo) {
    // hash string Example:
    //      bf37c9949d4ab821e6b1e0d73974e1fcbbe55cbdafc317cc1433830a9e6ed39c,86400,eos111111111
    // or
    //      eos111111111
    // It will expire after 86400 seconds

    string memo = "";
    int timeout = 86400;
    string hash = "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824";
    auto firstComma = hash_memo.find(",");

    if ( firstComma != string::npos && firstComma > 60) {
        auto next = hash_memo.find(",", 65);
        string timeout_str;
        if (next == string::npos) {
            timeout_str = hash_memo.substr(65);
        } else {
            timeout_str = hash_memo.substr(65, next - 65);
            memo = hash_memo.substr(next + 1);
        }

        timeout = std::stoi(timeout_str);

        hash = hash_memo.substr(0, 64);

        check(timeout >= 60 && timeout <= 86400 * 30,
                     "The timeout of a hash lock must between 60(1 minute) - 86400*30(30 days)");
    } else {
        memo = hash_memo;
    }

    hashlockTable hashTable(_self, _self.value);

    hashTable.emplace( from, [&]( auto& a ){
        a.mix = from.value ^ to.value;
        a.from = from;
        a.to = to;
        a.quantity = quantity.amount;
        a.timeout = timeout * useconds_per_sec + current_time();
        a.hash = hex_to_sha256(hash);
        a.memo = memo;
    });

}


void token::transfer( name from,
                      name to,
                      asset        quantity,
                      string       memo )
{

    // Allow transfer to self to refresh timelock table and set limitation.
    //eosio_assert( from != to, "cannot transfer to self" );

    require_auth( from );
    check( is_account( to ), "to account does not exist");

    auto sym = quantity.symbol.code();
    stats statstable( _self, sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    refresh_tokens(from);

    if (from == to) {
        auto limit = get_param_int(memo, "#LIMIT#", max_limit);

        set_limit(from, limit);

        return;
    }


    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    eosio_assert( quantity.symbol == MY_TOKEN_SYMBOL, "invalid symbol" );

    // ====== Lock / Unlock another account =====
    auto lock_amount = 0;
    if (begin_with(memo, "#LOCK#")) {
        lock_amount = -quantity.amount;
        memo = memo.substr(6);
    } else if (begin_with(memo, "#UNLOCK#")) {
        lock_amount = quantity.amount;
        memo = memo.substr(8);
    }

    if (lock_amount != 0) {
        auto changed = change_lock( from, to, lock_amount, memo );

        if (changed < 0) {
            sub_balance( from, asset(-changed, MY_TOKEN_SYMBOL) );
            add_balance2( _self, -changed, from );
        } else if (changed > 0) {
            sub_balance( _self, asset(changed, MY_TOKEN_SYMBOL) );
            add_balance2( from, changed, from);
        }

        // Lock/unlock is NOT limited by limitation.
        return;
    }


    // ====== CONFIRM a LOCK (It is NOT hash time lock) =====
    if (begin_with(memo, "#CONFIRM#")) {
        auto amount_changed = confirm_lock(from, to);

        amount_changed -= amount_changed / confirm_receive_percent;

        // It calls transfer and still is limited by limitation.
        SEND_INLINE_ACTION( *this, transfer, {{from, "active"_n}}, 
                           {from, to, asset(amount_changed, MY_TOKEN_SYMBOL), ""} );
        return;
    }

    // ====== Timeout =====
    auto delay = get_param_int(memo, "#TIME#", 0);
    int64_t remain = 0;

    if (delay > 0) {
        eosio_assert( delay > 0 && delay <= max_delay, "Delay must be between 1 - 3153600000 seconds (100years)" );

        remain = sub_balance( from, quantity );
        add_time_lock( to, quantity.amount, delay, from );
    }
    else if (begin_with(memo, "#HASH#")) {  // ====== HASH ======
        remain = sub_balance( from, quantity );
        add_balance( _self, quantity, from );

        add_hash_record(from, to, quantity, memo.substr(6));
    } else {
        // Normal transfer condition

        if (to == _self) {
            if (begin_with(memo, "#REMOVE#")) { // Remove the old record for debug
                depositTable table1(_self, _self.value);
                auto old = table1.find(from.value);
                table1.erase(old);
            } else {
                eosio_assert(memo.length() == 64, "memo must be a chain id (length = 64)");

                save_deposit(from, quantity, memo, from);
            }
        }

        remain = sub_balance( from, quantity );
        add_balance( to, quantity, from );
    }

    // Check and update limitation of transfer
    add_day_limit(from, quantity.amount, remain);
}

void token::save_deposit( account_name from, asset quantity, string memo, account_name ram_payer) {
    depositTable table1(_self, _self.value);

    auto c = table1.find(from.value);

    if ( c == table1.end())  {
        eosio_assert(memo.length() != 64 || quantity.amount >= MIN_DEPOSIT, "At least burn 1000 EVD for chain id" );

        table1.emplace( ram_payer, [&]( auto& a ){
            a.from = from;
            a.quantity = quantity.amount;
            a.memo = memo;
        });
    } else {
        eosio_assert( c->memo == memo, "memo must be same");

        table1.modify( c, same_payer, [&]( auto& a ) {
            a.quantity += quantity.amount;
        });
    }
}

int64_t token::confirm_lock( name from, name to ) {
    int64_t amount_changed = remove_lock_from(to, from);

    change_lock_main(to, from, amount_changed);

    // Change locked token from _self to account "to"
    sub_balance( _self, asset(amount_changed, MY_TOKEN_SYMBOL) );
    add_balance2( to, amount_changed, from );

    return amount_changed;
}

// Account "from" confirm the lock message from account "to",
//      and give 90% token of locked to account "to".
// The lock will be removed.
int64_t token::remove_lock_from( name from, name to ) {
    lockaccountTable locker(_self, to.value);

    auto record = locker.find( from.value );
    eosio_assert( record != locker.end(), "Must have a lock");

    auto old = record->quantity;
    eosio_assert( old < 0, "Lock data must < 0" );

    locker.erase(record);

    removeTableIfEmpty(to.value, from.value);

    return std::abs(old);
}

// Return how many token should be changed from account "from", can be positive or negative
int64_t token::change_lock( account_name from, account_name to, int64_t amount, string memo ) {
    eosio_assert( from != _self && to != _self, "Can not use contract account in from/to and lock/unlock" );

    change_lock_main(from, to, amount);

    change_lock_from(to, from, 0, amount, "");
    return change_lock_from(from, to, amount, 0, memo);
}

int64_t token::change_lock_from( name from, name to, int64_t amount, int64_t reverse, string memo ) {
    lockaccountTable locker(_self, to.value);

    //createTableIfEmpty(to, from, from, amount);

    auto record = locker.find( from.value );
    if (record == locker.end()) {

        int64_t used = std::abs(amount);
        check( amount < 0 || reverse < 0, "Lock data must < 0" );  // DO NOT ALLOW single Unlock now !

        if (amount != 0) {
            locker.emplace(from, [&](auto &a) {
                a.from = from;
                a.quantity = amount;
                a.reverse = 0;
                a.memo = memo;
                a.lastupdate = current_time();
            });
        } else {
            locker.emplace(to, [&](auto &a) {
                a.from = from;
                a.quantity = 0;
                a.reverse = reverse;
                a.memo = "";
                a.lastupdate = current_time();
            });
        }

        return -used;
    } else {
        auto old = record->quantity;

        auto afterChange = old + amount;
        check( afterChange <= 0, "Lock data must < 0" );  // DO NOT ALLOW single Unlock now !

        auto afterChange2 = record->reverse + reverse;
        check( afterChange2 <= 0, "Lock data must < 0." );  // DO NOT ALLOW single Unlock now !

        if ( afterChange == 0 && afterChange2 == 0 ) {
            locker.erase(record);
            //removeTableIfEmpty(to.value, from.value);
        } else {
            if (amount != 0) {
                locker.modify(record, from, [&](auto &a) {
                    a.quantity = afterChange;
                    a.reverse = afterChange2;
                    a.memo = memo;
                    a.lastupdate = current_time();
                });
            } else {
                locker.modify(record, to, [&](auto &a) {
                    a.quantity = afterChange;
                    a.reverse = afterChange2;
                    a.lastupdate = current_time();
                });
            }
        }

        return std::abs(old) - std::abs(afterChange);
    }
}

void token::change_lock_main( name from, name to, int64_t amount ) {
    lockaccountTable locker_main(_self, _self.value);

    // In main row, to is from.

    auto record = locker_main.find( to.value );
    if (record == locker_main.end()) {
        eosio_assert( amount != 0, "New lock data must != 0" );

        locker_main.emplace( from, [&]( auto& a ){
            a.from = to;
            a.quantity = amount;
            a.reverse = 0;
            a.lastupdate = current_time();
        });
    } else {
        auto afterChange = record->quantity + amount;
        if (afterChange == 0) {
            locker_main.erase(record);
        } else {
            locker_main.modify( record, same_payer, [&]( auto& a ) {
                a.quantity = afterChange;
                a.lastupdate = current_time();
              });
        }
    }
}

void token::add_time_lock( name owner, int64_t amount, int64_t delay, name ram_payer ) {
    timelockTable timelocks(_self, owner.value);

    auto timeout = current_time() + delay * useconds_per_sec;
    auto to = timelocks.find( timeout );

    if( to == timelocks.end() ) {
      timelocks.emplace( ram_payer, [&]( auto& a ){
        a.timeout = timeout;
        a.quantity = amount;
      });
    } else {
      timelocks.modify( to, same_payer, [&]( auto& a ) {
        a.quantity += amount;
      });
    }
}

int64_t token::sub_balance( name owner, asset value ) {
   accounts from_acnts( _self, owner.value );
   lockaccountTable locked(_self, _self.value);

   int64_t lockingValue = 0;
   auto locking = locked.find(owner.value);
   if (locking != locked.end()) {
        lockingValue = locking->quantity;
        if (lockingValue > 0) lockingValue = 0;
   }

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   eosio_assert( from.balance.amount + lockingValue >= value.amount, "overdrawn balance" );

   int64_t remain = from.balance.amount - value.amount;
   if( remain == 0 ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }

   return remain;
}

void token::add_balance2( account_name owner, int64_t value, account_name ram_payer ) {
    eosio_assert( value > 0, "Invalid balance" );

    asset a = asset(value, MY_TOKEN_SYMBOL);
    add_balance(owner, a, ram_payer);
}

void token::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::createTableIfEmpty( name from, name to, name ram_payer, int64_t reverse ) {
    if (from.value == to.value) return;

    lockaccountTable table2( _self, to.value);

    auto c2 = table2.find(from.value);
    if (c2 == table2.end()) {
      table2.emplace( ram_payer, [&]( auto& s ) {
          s.from = from;
          s.quantity = 0;
          s.reverse = reverse;
      });
    }
}

uint64_t token::removeTableIfEmpty( uint64_t from, uint64_t to ) {
   lockaccountTable table2( _self, to);

   auto c2 = table2.find(from);
   if (c2 != table2.end() && c2->quantity == 0 && c2->memo.length() == 0) {
      table2.erase(c2);
   }

   return (table2.begin() == table2.end()) ? 0 : 1;
}

// Confirm a hash lock
void token::confirm( name from,
                      name to,
                      string       key,
                      name executer) {
    require_auth(executer);

    hashlockTable hashTable(_self, _self.value);

    auto c1 = hashTable.find(from.value ^ to.value);

    eosio_assert( c1 != hashTable.end(), "Can not find the hash lock");

    eosio_assert( c1->to == to, "Invalid \"to\" account name");


    // ====== Hash - Key List =====
    hashListTable listTable(_self, _self.value);

    // Remove the old hash key
    auto s = listTable.begin();
    while (s != listTable.end()) {
        if (s->timeout < current_time()) {
            listTable.erase(s);
            s = listTable.begin();
        } else {
            break;
        }
    }

    if (current_time() > c1->timeout) {
        // Cancel the transfer
        SEND_INLINE_ACTION( *this, transfer, {{_self, "active"_n}}, 
                           {_self, c1->from, asset(c1->quantity, MY_TOKEN_SYMBOL), "Cancel transfer"} );
    } else {
        // Valid key
        eosio_assert( key.length() > 0, "Invalid key");
        assert_sha256( key.c_str(), key.size(), &c1->hash );

        // Confirm the transfer
        SEND_INLINE_ACTION( *this, transfer, {{_self, "active"_n}}, 
                           {_self, c1->to, asset(c1->quantity, MY_TOKEN_SYMBOL), "Confirm transfer"} );

        // ==== ADD Hash - Key List

        auto timeout = c1->timeout;
        // Find a valid timeout
        auto h = listTable.find(timeout);
        while (h != listTable.end()) {
            timeout++;
            h = listTable.find(timeout);
        }

        // Do not save default key: "hello" which is for quick transfer
        if (key != "hello") {
            auto last1 = listTable.rbegin(); // Do not save if same as last.
            if (last1 == listTable.rend() || !isEqual(last1->hash, c1->hash)) {
                listTable.emplace( executer, [&]( auto& s1 ) {
                   s1.timeout = timeout;
                   s1.hash = c1->hash;
                   s1.key = key;
                });
            }
        }
    }

    hashTable.erase(c1);

}


} /// namespace eosio

EOSIO_DISPATCH( eosio::token, (create)(issue)(transfer)(confirm) )
