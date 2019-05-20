#include "comments.hpp"


const uint64_t ISSUER_ACCOUNT = N(eoslocktoken);
const auto TOKEN_SYMBOL = string_to_symbol(4, "EVD");

const uint64_t DISMISS_SENDER = N(eosvrrewards);


const uint64_t useconds_per_sec = uint64_t(1000000);
const uint64_t useconds_per_day = 24 * 3600 * useconds_per_sec;
const uint64_t useconds_10min = 600 * useconds_per_sec;

const uint64_t MAX_WEIGHT = 1000;  // Weight should be (1-1000). 1 for the account who do not have limitation. 100 is 1% per month.

const uint64_t MIN_EVD = 1 * 10000; // Min lock is 1 EVD at first comment

const uint64_t COMMENT_LOCK_SECONDS = 3 * 86400; // After a comment, it will lock 3 days before withdraw all tokens.


struct impl {

// Get weight of an account by its limitation (percent transfer per month)
int64_t getWeight(uint64_t owner) {
    token::timelockTable timelocks(ISSUER_ACCOUNT, owner);
    auto limit0 = timelocks.find(0);

    if (limit0 == timelocks.end()) {
        return 1;
    } else {
        int64_t limit = limit0->quantity;

        /*
        Limit Multi
        11 - 50: 5 * 100 / Limit;
        1 - 10: 10 * 100 / Limit;
        Other: 1;
        */

        if (limit <= 0 || limit > 50)
            return 1;

        int64_t result = (limit > 10) ? 5 : 10;
        result = result * 100 / limit;

        if (result > MAX_WEIGHT) result = MAX_WEIGHT;

        return result;
    }
}

void doWithDraw(uint64_t _self, account_name from, account_name to, bool onlySupport) {

    commentTable table1( _self, to);

    auto current = table1.find(from);

    if (onlySupport) {
        if (current == table1.end() || current->deposit <= 0)
            return;
    }

    eosio_assert(current != table1.end(), "The account does not exist!");

    auto oldAmount = current->deposit;
    auto oldWeight = current->weight;
    if (oldWeight < 1 || oldWeight > MAX_WEIGHT) oldWeight = 1;

    auto back = asset( oldAmount, TOKEN_SYMBOL );
    if (oldAmount < 0) {
      back.amount = -oldAmount;
    }

    table1.modify(current, 0, [&]( auto& s ) {
        s.deposit = 0;
    });

    removeTableIfEmpty(_self, to, from);

    bool remain_to = hasDataInCommentTable(_self, to);

    // Remove oldAmount from MainTable
    gcommentTable table2(_self, _self);
    auto current2 = table2.find(to);
    if (current2 == table2.end()) return;

    string str = (name{from}).to_string();

    if (!remain_to) {
      table2.erase(current2);
    } else {
      table2.modify(current2, 0, [&]( auto& s ) {
        s.total -= oldAmount * oldWeight;
        if (oldAmount < 0) s.totaldown -= oldAmount * oldWeight;

        s.lastupdate = current_time();
      });
    }

    if (back.amount > 0) {
        action{
            permission_level{_self, N(active)},
            ISSUER_ACCOUNT,
            N(transfer),
            token::transfer_args{
                .from=_self, .to=from, .quantity=back, .memo="#TIME# 86400"}    // Withdraw take 1 day to arrive
        }.send();
    }
}


// Dismiss all support to this account
void dismissAction(uint64_t _self, const dismiss& dismiss) {
    account_name account = dismiss.account;

    require_auth( DISMISS_SENDER );

    commentTable table1( _self, account);
    auto t1 = table1.begin();

    while (t1 != table1.end()) {
        auto from = t1->from;
        t1++;

        if (from) {
            doWithDraw(_self, from , account, true);
        }
    }

}

// Comment a user
void commentAction(uint64_t _self, const comment& comment)
{
    account_name from = comment.from;
    account_name to = comment.to;
    string memo = comment.memo;

    require_auth( from );

    eosio_assert(to != 0 && from != 0, "Must have account");

    commentTable table1( _self, to);

    auto c = table1.find(from);
    if (c == table1.end()) {
        table1.emplace( from, [&]( auto& s ) {
          s.from = from;
          s.memo = memo;
          s.deposit = 0;
          s.weight = getWeight(from);
          s.lastupdate = current_time();
        });

        createTableIfEmpty(_self, to, from);
    } else {
        table1.modify( c, from, [&]( auto& s ) {
          s.memo = memo;
        });

        removeTableIfEmpty(_self, to, from);
    }
}

// Withdraw the token that comment a user
void withdrawAction(uint64_t _self, const withdraw& dat) {
    account_name from = dat.from;
    account_name to = dat.to;

    require_auth(from);

    doWithDraw(_self, from, to, false);
}

void transferAction (uint64_t _self, uint64_t code) {
    auto data = unpack_action_data<token::transfer_args>();
    if(data.from == _self || data.to != _self)
      return;

    // Only work when transfer EVD from eoslocktoken
    if (code != ISSUER_ACCOUNT)
      return;

    eosio_assert(data.quantity.is_valid(), "Invalid quantity");

    auto symbol = data.quantity.symbol;
    eosio_assert(symbol == TOKEN_SYMBOL, "Only support EVD");

    int64_t deposit = data.quantity.amount;
    auto str = data.memo;

    // Do not work with control message.
    if (str.find("#") == 0) {
        return;
    }

    if (str[0] == '+') {
      str = str.substr(1);
    } else if (str[0] == '-') {
      deposit = -deposit;
      str = str.substr(1);
    }

    eosio_assert(str.length() <= 12, "Account length must less or equal 12");
    auto account = eosio::string_to_name(str.c_str());
    if (str.length() == 0) {
      account = data.from;
    }

    // Set comment table
    commentTable table1( _self, account);
    int64_t depositChanged = 0;
    int64_t depositDownChanged = 0;
    int64_t weight = 1;

    auto c = table1.find(data.from);
    if (c == table1.end()) {
      eosio_assert(data.quantity.amount >= MIN_EVD,
        "Amount must be greater than MIN EVD because it takes RAM of contract to save it");

      weight = getWeight(data.from);

      depositChanged = deposit;
      if (deposit < 0)
        depositDownChanged = deposit;

      // RAM pay by contract.
      table1.emplace( _self, [&]( auto& s ) {
          s.from = data.from;
          s.deposit = deposit;
          s.weight = weight;
          s.lastupdate = current_time();
      });
    } else {
        weight = c->weight;
        if (weight < 1 || weight > MAX_WEIGHT) weight = 1;

        int64_t oldDeposit = c->deposit;
        eosio_assert(
            (oldDeposit >= 0 && deposit >= 0) || (oldDeposit <= 0 && deposit <= 0),
            "Can not change from positive to negative");

        depositChanged = deposit;
        depositDownChanged = (deposit < 0) ? deposit : 0;

        table1.modify( c, 0, [&]( auto& s ) {
            s.deposit += depositChanged;
            s.lastupdate = current_time();
        });
    }

    // Create opposite comment table if no exist
    createTableIfEmpty(_self, account, data.from);

    // Add details to global table
    gcommentTable table2(_self, _self);
    auto current2 = table2.find(account);
    if (current2 == table2.end()) {
      table2.emplace( _self, [&]( auto& s ) {
          s.account = account;
          s.total = depositChanged * weight;
          s.totaldown = depositDownChanged * weight;
          s.lastupdate = current_time();
      });
    } else {
        table2.modify(current2, 0, [&]( auto& s ) {
            int64_t t1 = s.total + depositChanged * weight;

            int64_t t2 = s.totaldown + depositDownChanged * weight;

            s.total = t1;
            s.totaldown = t2;
            s.lastupdate = current_time();
        });
    }
}

void createTableIfEmpty(uint64_t _self, uint64_t from, uint64_t to ) {
    if (from == to) return;

    commentTable table2( _self, to);

    auto c2 = table2.find(from);
    if (c2 == table2.end()) {
      table2.emplace( _self, [&]( auto& s ) {
          s.from = from;
          s.deposit = 0;
          s.lastupdate = current_time();
          s.weight = getWeight(from);
      });
    }
}

void removeTableIfEmpty(uint64_t _self, uint64_t from, uint64_t to ) {
    commentTable table1( _self, from);
    auto c1 = table1.find(to);

    commentTable table2( _self, to);
    auto c2 = table2.find(from);

    if (c1 != table1.end() && c2 != table2.end()) {
        if (c1->deposit == 0 && c1->memo.length() == 0
            && c2->deposit == 0 && c2->memo.length() == 0) {

            table1.erase(c1);

            if (to != from) {
                table2.erase(c2);
            }
        }
    }
}

bool hasDataInCommentTable(uint64_t _self, uint64_t to) {
    commentTable table2( _self, to);
    return table2.begin() != table2.end();
}

void clearOne(uint64_t _self, uint64_t to) {
    commentTable table1( _self, to);
    do {
        auto s = table1.begin();
        if (s == table1.end()) break;

        table1.erase(s);
    } while(true);
}

// ===== Debug only =====
// Clear all comments to an account, need auth of _self
void clearAction(uint64_t _self, const withdraw& dat) {
    account_name from = dat.from;
    account_name to = dat.to;

    require_auth(_self);

    clearOne(_self, from);
    clearOne(_self, to);

    if (_self == from) {
        gcommentTable table3(_self, _self);
        do {
            auto s3 = table3.begin();
            if (s3 == table3.end()) return;

            clearOne(_self, s3->account);

            table3.erase(s3);
        } while(true);
    }
}

void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
  if (code == ISSUER_ACCOUNT && action == N(transfer)) {
    transferAction(receiver, code);
    return;
  }

  if (action == N(withdraw)) {
    withdrawAction(receiver, eosio::unpack_action_data<withdraw>());
  } else if (action == N(comment)) {
    commentAction(receiver, eosio::unpack_action_data<comment>());
  } else if (action == N(dismiss)) {
    dismissAction(receiver, eosio::unpack_action_data<dismiss>());
  } else if (action == N(clear)) {
	clearAction(receiver, eosio::unpack_action_data<withdraw>());
  }
}


};



extern "C" {

  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    impl().apply(receiver, code, action);
  }
}
