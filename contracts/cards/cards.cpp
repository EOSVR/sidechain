#include "cards.hpp"

//* RAM PAY METHOD *//
// reg:  owner pay.
// buy:  contract pay. (eos limitation)
// remove: if it pays by contract, send back pay money.

// true content = content of creator + content of current (Reference Mode) (Normal)
//  or
// content of current ONLY (Copy Mode) (Test)

const uint64_t ISSUER_ACCOUNT = "eoslocktoken"_n.value;
const auto TOKEN_SYMBOL = symbol(symbol_code("EVD"), 4);

const uint64_t EVD_RAM_RATE = 1; // 1 EVDS = 1 byte

const uint64_t useconds_per_sec = uint64_t(1000000);
const uint64_t useconds_per_day = 24 * 3600 * useconds_per_sec;
const uint64_t useconds_10min = 600 * useconds_per_sec;


struct impl {


    // Reg or set a card
    void regAction(uint64_t _self, const reg& reg)
    {
        name from = reg.from;
        require_auth( from );

        int64_t id = reg.id;

        cardTable table1( name(_self), from.value);

        int64_t ct = current_time() / 1000;

        if (id <= 0) {
            id = ct;

            auto total = reg.total;
            int64_t max_supply = 0;
            if (total <= 0)
                total = 0;
            else
                max_supply = total;

            auto sell = reg.sell;
            if (sell < total) sell = 0;

            auto price = reg.price;
            if (price < 0) price = 0;

            table1.emplace( from, [&]( auto& s ) {
                s.id = id;
                s.creator = from;
                s.mark = from.value ^ id; // UserName XOR Time
                s.max_supply = max_supply;
                s.total = total;
                s.sell = sell;
                s.price = price;
                s.lastupdate = ct;
                s.content = reg.content;
                s.rampayed = 0;
            });
        } else {
          // Change an old card
          auto c = table1.find(reg.id);
          eosio_assert(c != table1.end(), "Invalid card ID");

          if (reg.total < 0) {
              // Remove card

              if (c->rampayed > 0) {
                  // Send back ram payed
                  action{
                          permission_level{name(_self), "active"_n},
                          name(ISSUER_ACCOUNT),
                          "transfer"_n,
                          currency::transfer{
                                  .from=_self, .to=from.value, .quantity=asset(c->rampayed, TOKEN_SYMBOL), .memo="Send back ram payed"}
                  }.send();
              }

              table1.erase(c);
          } else {
            // Modify card

            auto total = reg.total;
            auto max_supply = c->max_supply;
            if (total > 0 && total != c->total) {
                // Change total and max_supply
                check(c->max_supply == 0, "Must be first time to change total");
                check(c->creator == from, "Must be owner to change total");
                max_supply = total;
            } else {
                total = c->total;
            }

            check(reg.sell <= total, "Total can not exceed sell");

            auto sell = reg.sell;
            if (reg.sell < 0) sell = c->sell; // Do not change

            auto price = reg.price;
            if (price < 0) price = c->price; // Do not change

            if (reg.content.length() <= 12) {
                table1.modify( c, same_payer, [&]( auto& s ) {
                    s.max_supply = max_supply;
                    s.total = total;
                    s.sell = sell;
                    s.price = price;
                    s.lastupdate = ct;
                });
            } else {
                // If modify content, change ram payer to card owner.

                table1.modify( c, from, [&]( auto& s ) {
                    s.max_supply = max_supply;
                    s.total = total;
                    s.sell = sell;
                    s.price = price;
                    s.lastupdate = ct;
                    s.content = reg.content;
                });
            }
          }
        }
    }


    void transferAction (uint64_t _self, uint64_t code) {
        auto data = unpack_action_data<token::transfer_args>();
        auto from = data.from;
        auto to = data.to;

        if(from.value == _self || to.value != _self)
          return;

        // Only work when transfer EVD from eoslocktoken
        if (code != ISSUER_ACCOUNT)
          return;

        check (data.quantity.is_valid(), "Invalid quantity");

        auto symbol = data.quantity.symbol;
        check (symbol == TOKEN_SYMBOL, "Only support EVD");

        int64_t deposit = data.quantity.amount;
        auto str = data.memo;

        // Memo should like:
        //      +guest11111113,5    (Copy card 5)
        //      &guest1111113,5     (Use card 5 as reference)
        // It will buy card 5 of guest1111113
        if (str[0] != '+' && str[0] != '&') {
          return;
        }

        bool isReference = (str[0] == '&');

        auto ind = str.find(",");
        if (ind < 0) {
          return;
        }

        auto shopperStr = str.substr(1, ind-1);
        auto cardNoStr = str.substr(ind+1);

        check (shopperStr.length() <= 12, "Account length must less or equal 12");

        auto shopper = name(shopperStr.c_str());

        int64_t cardNo = std::stoll(cardNoStr);

        cardTable table1( name(_self), shopper.value);
        auto c = table1.find(cardNo);
        check (c != table1.end(), "Can not find this card");

        check (c->sell > 0, "No more card");

        int64_t existedId = getExisted(_self, from, c->creator, c->mark);

        int64_t ram_pay = 0;

        if (existedId == 0) {
            ram_pay = getRamPay(isReference ? 0 : c->content.length());
        }
        deposit -= ram_pay;
        check (deposit >= c->price, "Can not buy this card");


        int64_t number = deposit / c->price;
        if (number > c->sell) number = c->sell;

        int64_t new_total = c->total - number;
        if (new_total > 0) {
            // Change number of shopper
            table1.modify(c, same_payer, [&](auto &s) {
                s.total = new_total;
                s.sell = c->sell - number;
                s.lastupdate = current_time() / 1000;
            });
        } else {
            // Remove item in shopper
            if (c->rampayed > 0) {
                // Send back ram payed
                action{
                        permission_level{name(_self), "active"_n},
                        name(ISSUER_ACCOUNT),
                        "transfer"_n,
                        currency::transfer{
                                .from=_self, .to=shopper.value, .quantity=asset(c->rampayed, TOKEN_SYMBOL), .memo="Send back ram payed"}
                }.send();
            }

            table1.erase(c);
        }

        // Add card for buyer
        addCard(_self, existedId, from, number, c->creator, c->mark, c->max_supply,
                isReference ? "" : c->content, ram_pay);

        int64_t sendto = number * c->price;
        int64_t sendback = deposit - sendto;

        action{
                permission_level{name(_self), "active"_n},
                name(ISSUER_ACCOUNT),
                "transfer"_n,
                currency::transfer{
                        .from=_self, .to=shopper.value, .quantity=asset(sendto, TOKEN_SYMBOL), .memo="Send to shopper"}
        }.send();

        if (sendback > 0) {
            action{
                    permission_level{name(_self), "active"_n},
                    name(ISSUER_ACCOUNT),
                    "transfer"_n,
                    currency::transfer{
                            .from=_self, .to=data.from.value, .quantity=asset(sendback, TOKEN_SYMBOL), .memo="Send back extra money"}
            }.send();
        }
    }

    int64_t getRamPay(int64_t contentLength) {
        if (contentLength < 100)
            return 100;
        else
            return contentLength * EVD_RAM_RATE;
    }

    int64_t getExisted(uint64_t _self, name account, name creator, int64_t mark) {
        cardTable table1( name(_self), account.value);

        auto second = table1.get_index<"bymark"_n>();

        auto existed = second.find(mark);

        if (existed != second.end() && existed->creator == creator)
            return existed->id;

        return 0;
    }

    void addCard(uint64_t _self, int64_t existedId, name account,
                 int64_t number, name creator, int64_t mark, int64_t max_supply,
                 string content,
                 int64_t ramPay
    ) {
        cardTable table1( name(_self), account.value);

        if (existedId > 0) {
            auto item = table1.find(existedId);

            table1.modify(item, same_payer, [&]( auto& s ) {
                s.total = item->total + number;
                s.content = content;         // 购买新的将会刷新内容
                s.lastupdate = current_time() / 1000;
            });
        } else {
            auto ct = current_time() / 1000;
            table1.emplace( name(_self), [&]( auto& s ) {
                s.id = ct;
                s.creator = creator;
                s.mark = mark;
                s.max_supply = max_supply;
                s.total = number;
                s.sell = 0;
                s.price = 0;
                s.lastupdate = ct;
                s.content = content;
                s.rampayed = ramPay;
            });
        }


    }


    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
      if (code == ISSUER_ACCOUNT && action == "transfer"_n.value) {
        transferAction(receiver, code);
        return;
      }

      if (action == "reg"_n.value) {
        regAction(receiver, eosio::unpack_action_data<reg>());
      }
    }


};



extern "C" {

  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    impl().apply(receiver, code, action);
  }
}
