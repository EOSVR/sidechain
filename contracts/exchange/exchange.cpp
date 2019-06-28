#include "exchange.hpp"

const name MANAGER_ACCOUNT = "eosvrmanager"_n;

const name ISSUER_ACCOUNT1 = "eosvrtokenss"_n;
const name ISSUER_ACCOUNT2 = "eoslocktoken"_n;
const auto TOKEN_SYMBOL1 = symbol(symbol_code("EVR"), 4);
const auto TOKEN_SYMBOL2 = symbol(symbol_code("EVD"), 4);
const uint64_t rate_1to2 = 1;
const uint64_t rate_2to1 = 1;

struct impl {

    bool begin_with(string memo, string str) {
        auto ind = memo.find(str);
        return (ind == 0);
    }


    void transferAction (uint64_t _self, uint64_t code) {
      auto data = unpack_action_data<currency::transfer>();

      // When setup, use manager account to put EVD into it.
      if(data.from == _self || data.to != _self || data.from == MANAGER_ACCOUNT.value)
        return;

      uint64_t from = data.from;
      uint64_t to = data.to;

      require_auth(from);

      eosio_assert(data.quantity.is_valid(), "Invalid quantity");

      auto memo = data.memo;
      eosio_assert(begin_with(memo, "#TO#") || memo.length() == 0, "memo must be empty or to in exchange");

      uint64_t transferTo = from;
      string transfer_memo = "Exchange";
      if (begin_with(memo, "#TO#")) { // #TO#user11111111,Hello
          auto ind = memo.find(",");
          if (ind == string::npos) {
              transferTo = name(memo.substr(4)).value;
          } else {
              transferTo = name(memo.substr(4, ind - 4)).value;
              transfer_memo = memo.substr(ind + 1);
          }
      }


      if (code == ISSUER_ACCOUNT1.value) {
        eosio_assert(data.quantity.symbol == TOKEN_SYMBOL1, "Invalid symbol1");

        auto back = asset(data.quantity.amount * rate_1to2, TOKEN_SYMBOL2);
        action{
            permission_level{name(_self), "active"_n},
            ISSUER_ACCOUNT2,
            "transfer"_n,
            currency::transfer{
                .from=_self, .to=transferTo, .quantity=back, .memo=transfer_memo}
        }.send();
      } else {
        eosio_assert(data.quantity.symbol == TOKEN_SYMBOL2, "Invalid symbol2");

        auto back = asset(data.quantity.amount * rate_2to1, TOKEN_SYMBOL1);
        action{
            permission_level{name(_self), "active"_n},
            ISSUER_ACCOUNT1,
            "transfer"_n,
            currency::transfer{
                .from=_self, .to=transferTo, .quantity=back, .memo=transfer_memo}
        }.send();
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
