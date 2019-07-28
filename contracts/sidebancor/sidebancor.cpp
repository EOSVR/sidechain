#include "sidebancor.hpp"
#include "eosio.token.hpp"


const name ISSUER1 = "eoslocktoken"_n;
const auto SYMBOL1 = symbol(symbol_code("EVD"), 4);

const name ISSUER2 = "eosio.token"_n;
const auto SYMBOL2 = symbol(symbol_code("EOS"), 4);

const uint64_t MAX_1To2 = 0;
const uint64_t MAX_2To1 = 0;

const int64_t MIN_INVEST_1 = 1000000; // Min invest amount = 100
const int64_t MIN_INVEST_2 = 1000000; // Min invest amount = 100

const int feeDivided = 200; // fee = amount / feeDivided. Current fee rate = 5/1000

struct impl {

bool begin_with(string memo, string str) {
    auto ind = memo.find(str);
    return (ind == 0);
}

int64_t getBalance(name issuer, symbol symbol1, name account)
{
    auto result = eosio::token::get_balance(issuer, account, symbol1.code()).amount;
    return result;
}
    
int64_t getBancorToken(int64_t deposit2, name _self,
                       name issuer1, symbol symbol1,
                       name issuer2, symbol symbol2,
                       uint64_t max) {
  const auto balance1 = getBalance(issuer1, symbol1, _self);
  const auto balance2 = getBalance(issuer2, symbol2, _self);

  if (balance1 <= 0 || balance2 <= 0) return 0;

  if (deposit2 * 3 > balance2) return 0; // Out of Range. (Input must be within 1/3 total)

  auto init_out = balance1 * 1.0 / balance2 * deposit2;
    
  auto final_price = (balance1 - init_out) * 1.0 / (balance2 + deposit2);
  
  if (max > 0 && final_price > max) final_price = max;
  
  int64_t final_out = final_price * deposit2;
  return final_out;  
}
  
int64_t getToken_1To2(int64_t deposit1, name _self) {
  return getBancorToken(deposit1, _self, ISSUER2, SYMBOL2, ISSUER1, SYMBOL1, MAX_1To2);
}    

int64_t getToken_2To1(int64_t deposit2, name _self) {
  return getBancorToken(deposit2, _self, ISSUER1, SYMBOL1, ISSUER2, SYMBOL2, MAX_2To1);
}

int64_t ReduceFee(name _self, int64_t amount, bool isSymbol1) {
    if (amount <= 1) return 0;

    int64_t fee = amount / feeDivided;
    if (fee < 1) fee = 1; // Min is 1.

    AddDepositTable(_self, fee, _self, isSymbol1, _self);

    return amount - fee;
}

// Add into deposit table, and return remain deposit. (When amount = 0, will not return actual value !)
int64_t AddDepositTable(name _self, int64_t amount, name account, bool isSymbol1, name ram_payer) {
    if (amount == 0) return 0;

    if (isSymbol1) {
        depositTable1 table1( _self, _self.value );
        auto t1 = table1.find(account.value);
        if (t1 == table1.end()) {
            table1.emplace( ram_payer, [&]( auto& s ) {
               s.account  = account;
               s.deposit = amount;
            });

            return amount;
        } else {
            int64_t result = amount + t1->deposit;
            table1.modify( t1, same_payer, [&]( auto& a ) {
                a.deposit = result;
            });

            return result;
        }
    } else {
        depositTable2 table2( _self, _self.value );
        auto t2 = table2.find(account.value);
        if (t2 == table2.end()) {
            table2.emplace( ram_payer, [&]( auto& s ) {
               s.account  = account;
               s.deposit = amount;
            });

            return amount;
        } else {
            int64_t result = amount + t2->deposit;
            table2.modify( t2, same_payer, [&]( auto& a ) {
                a.deposit = result;
            });

            return result;
        }

    }

}

int64_t GetDepositTable(name _self, int64_t account, bool isSymbol1) {
    if (isSymbol1) {
        depositTable1 table1( _self, _self.value );
        auto t1 = table1.find(account);
        if (t1 == table1.end()) return 0;

        return t1->deposit;
    } else {
        depositTable2 table2( _self, _self.value );
        auto t2 = table2.find(account);
        if (t2 == table2.end()) return 0;

        return t2->deposit;
    }
}

void SaveDeposit(name _self, name from, int64_t amount, bool isSymbol1) {
    if (amount <= 0) return;

    int64_t bonus = GetDepositTable(_self, _self.value, isSymbol1);
    int64_t total = GetDepositTable(_self, 0, isSymbol1);

    // Deposit += Amount * Total / (Bonus + Total)
    int64_t bonus_add = 0;
    if (total > 0 && bonus + total > 0) {
        bonus_add = amount * 1.0 / (bonus + total) * bonus;
    }

    int64_t amount_add = amount - bonus_add;
    AddDepositTable(_self, amount_add, from, isSymbol1, _self); // Add to account
    AddDepositTable(_self, amount_add, name(0), isSymbol1, _self); // Add to total

    // Bonus += Amount * Bonus / (Bonus + Total)
    AddDepositTable(_self, bonus_add, _self, isSymbol1, _self); // Add to bonus
}

void Withdraw(name _self, name account, int64_t amount, bool isSymbol1) {
    if (amount <= 0) return;

    int64_t bonus = GetDepositTable(_self, _self.value, isSymbol1);
    int64_t total = GetDepositTable(_self, 0, isSymbol1);
    int64_t saved = GetDepositTable(_self, account.value, isSymbol1);

    eosio_assert (saved > 0 && bonus + total > 0, "No deposit");

    double percent = 10000 * total / (bonus + total); // Fix overflow of (amount * total)
    int64_t saved_remove = amount * percent / 10000;
    int64_t bonus_remove = amount - saved_remove;

    if (saved_remove > saved) { // No enough saved, remove all
        saved_remove = saved;
        bonus_remove = saved * bonus / (bonus + total);
    }

    auto remain1 = AddDepositTable(_self, -saved_remove, account, isSymbol1, _self); // Remove deposit
    auto remain2 = AddDepositTable(_self, -bonus_remove, _self, isSymbol1, _self);   // Remove bonus
    auto remain3 = AddDepositTable(_self, -saved_remove, name(0), isSymbol1, _self); // Remove total of deposit

    eosio_assert (remain1 >= 0, "What's wrong deposit ?");
    eosio_assert (remain2 >= 0, "What's wrong bonus ?");

    int64_t back = amount + saved_remove + bonus_remove;

    transferBack(_self, account, back, isSymbol1, "Withdraw");
}

void transferBack(name _self, name account, int64_t back_amount, bool isSymbol1, string memo ) {
    if (isSymbol1) {
        auto back = asset( back_amount, SYMBOL1 );

        action{
            permission_level{_self, "active"_n},
            ISSUER1,
            "transfer"_n,
            token::transfer_args{
              .from=_self, .to=account, .quantity=back, .memo=memo}
        }.send();

    } else {
        auto back = asset( back_amount, SYMBOL2 );

        action{
            permission_level{_self, "active"_n},
            ISSUER2,
            "transfer"_n,
            token::transfer_args{
              .from=_self, .to=account, .quantity=back, .memo=memo}
        }.send();
    }

}
  
void transferAction (uint64_t _self_int, uint64_t code) {
    auto data = unpack_action_data<token::transfer_args>();
    auto _self = name(_self_int);
    if(data.from == _self || data.to != _self)
      return;

    eosio_assert(data.quantity.is_valid(), "Invalid quantity");

    auto symbol = data.quantity.symbol;
  
    name from = data.from;
    name to = data.to;

    require_auth(from);
    check(code == ISSUER1.value || code == ISSUER2.value, "Do not transfer other token");

    string memo = data.memo;
    if (begin_with(memo, "#INVEST#")) {
        if (code == ISSUER1.value) {
            check(data.quantity.amount >= MIN_INVEST_1, "Must reach min amount of invest");

            SaveDeposit(_self, data.from, data.quantity.amount, true);
        } else {
            check(data.quantity.amount >= MIN_INVEST_2, "Must reach min amount of invest");

            SaveDeposit(_self, data.from, data.quantity.amount, false);
        }
        return;
    } else if (begin_with(memo, "#WITHDRAW#")) {
        if (code == ISSUER1.value) {
            Withdraw(_self, data.from, data.quantity.amount, true);
        } else {
            Withdraw(_self, data.from, data.quantity.amount, false);
        }
        return;
    }

    name transferTo = from;
    string transfer_memo = "Exchange";
    if (begin_with(memo, "#TO#")) { // #TO#user11111111,Hello
        auto ind = memo.find(",");
        if (ind == string::npos) {
            transferTo = name(memo.substr(4));
        } else {
            transferTo = name(memo.substr(4, ind - 4));
            transfer_memo = memo.substr(ind + 1);
        }
    }


    if (code == ISSUER1.value) {
        // 1 To 2
        eosio_assert(symbol == SYMBOL1, "Invalid symbol1");

        auto amount = ReduceFee(_self, data.quantity.amount, true);
        eosio_assert(amount > 0, "Must be positive.");

        auto backToken = getToken_1To2(amount, _self);
        eosio_assert(backToken > 0, "Must greater than 0");

        transferBack(_self, transferTo, backToken, false, transfer_memo );
    } else {
        // 2 To 1
        eosio_assert(symbol == SYMBOL2, "Invalid symbol2");

        auto amount = ReduceFee(_self, data.quantity.amount, false);
        eosio_assert(amount > 0, "Must be positive.");

        auto backToken = getToken_2To1(amount, _self);
        eosio_assert(backToken > 0, "Must greater than 0");

        transferBack(_self, transferTo, backToken, true, transfer_memo );
    }
}


void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if (action == "transfer"_n.value) {
        transferAction(receiver, code);
    }
}


};



extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    impl().apply(receiver, code, action);
  }
}
