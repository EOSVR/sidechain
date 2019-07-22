#include "cards.hpp"

//* RAM PAY METHOD *//
// reg:  owner pay.
// buy:  contract pay. (eos limitation)
// remove: if it pays by contract, send back pay money.

// true content = content of creator + content of current (Reference Mode) (Normal)
//  or
// content of current ONLY (Copy Mode) (Test)

// 当使用 reg '{"from":"guest1111111", "content":"XXXXX", "id":1558446691000000, "total":1000, "sell":-50, "price":10000}' -p guest1111111 时，
// 一张宝藏卡将建立（sell < 0），这时 creator 账户中：
//      rampayed < 0, 表示宝藏量，记录的是 eoscardcards 收到的所有EVD的数量（-XXX），初始-1。
//      max_supply 表示的是总共卖出的数量。
//      total 表示每张每天的分红数量。（比如：1000，表示每张每天可以分 0.1 EVD）。
//      sell 表示矿山维持时间（天）
//
// 挖宝(回收分红) = Min(total, -rampayed / max_supply / sell) * days ( days = [1, price / 10,0000]，price 为 0 的话无上限 )
// 执行命令为： withdraw '{"id": "123184920012", "owner": "user111111111"}'

// 摧毁卖出的宝藏卡 (买了宝藏卡的人用), 摧毁不会返 token
// 如果是宝藏卡的 creator 使用，需要
// removemine '{"id": "123184920012", "owner": "user111111111"}'

//
// Service 输入token的命令为: transfer {..., "to": "eoscardcards", ... , "memo": "^guest1111111,123184920012"}
// 表示直接加 rampayed 中的量。
//
// 限制：
// 1, 卖出了10张卡后，宝藏卡无法被销毁 (即，max_supply > 10 时，removemine 不起作用。10张之前表示测试)
// 2, creator无法买自己
// 3, 之后的 reg 无法修改 total 和 sell (防止创建者乱改)，只能修改 price
// 4, price 影响挖矿的允许间隔。 间隔为[1, price / 10,0000] day （最大30days）
// 5, 如果不想加人的话，price 改为一个非常大的数即可(之前卖出的还可以流通)。
// 6, price 只许增，不许减

const uint64_t ISSUER_ACCOUNT = "eoslocktoken"_n.value;
const auto TOKEN_SYMBOL = symbol(symbol_code("EVD"), 4);

// When 100, 1 EVDS = 1 byte.
// When it is 200, 2 EVDS = 1 byte.
// When it is 10, 1 EVDS = 10 bytes.
const uint64_t EVD_RAM_RATE = 100;

const uint64_t useconds_per_sec = uint64_t(1000000);
const uint64_t useconds_per_day = 24 * 3600 * useconds_per_sec;
const uint64_t useconds_10min = 600 * useconds_per_sec;

const uint64_t ms_per_day = 24 * 3600 * 1000;
const uint64_t min_per_day = 24 * 60;
const uint64_t ms_per_1hour = 60 * 60 * 1000;


struct impl {
    void removeAction(uint64_t _self, const withdraw& withdraw) {
        name owner = withdraw.owner;
        require_auth( owner );
        
        int64_t id = withdraw.id;
        
        cardTable table1( name(_self), owner.value);
        
        auto c = table1.find(id);
        check (c != table1.end(), "No card");
        
        name creator = c->creator;
        
        if (owner == creator) {
            check (c->rampayed >= 0, "Use removemine to remove a mine card.");
            
            check(c->max_supply == c->total, "Can not remove a card that sold to other");
            
            if (c->rampayed > 0) {
                // Send back ram payed
                action{
                    permission_level{name(_self), "active"_n},
                    name(ISSUER_ACCOUNT),
                    "transfer"_n,
                    currency::transfer{
                        .from=_self, .to=owner.value, .quantity=asset(c->rampayed, TOKEN_SYMBOL), .memo="Send back ram payed"}
                }.send();
            }
            
            table1.erase(c);
        } else {
            int64_t mark = c->mark;
            int64_t rawId = mark ^ creator.value;
            
            cardTable table2( name(_self), creator.value);
            auto origin = table2.find(rawId);
            
            if (origin != table2.end()) {
                int64_t ct = current_time() / 1000;
                int64_t new_max_supply = origin->max_supply - c->total;
                check (new_max_supply >= 0, "Invalid max supply");

                table2.modify( origin, same_payer, [&]( auto& s ) {
                    s.max_supply = new_max_supply;  // Remove max_supply
                    s.lastupdate = ct;
                });
            }
            
            table1.erase(c);
        }
    }
    
    
    void removemineAction(uint64_t _self, const withdraw& withdraw) {
        name owner = withdraw.owner;
        require_auth( owner );
        
        int64_t id = withdraw.id;
        
        cardTable table1( name(_self), owner.value);
        
        auto c = table1.find(id);
        check (c != table1.end(), "No card");
        
        name creator = c->creator;
        
        if (owner == creator) {
            check (c->max_supply <= 10, "Mine card can not destroy when other have more than 10");
    
            // Send back all remain token to creator.
            int64_t payback = -c->rampayed - 1;
            if (payback > 0) {
                action{
                    permission_level{name(_self), "active"_n},
                    name(ISSUER_ACCOUNT),
                    "transfer"_n,
                    currency::transfer{
                        .from=_self, .to=owner.value, .quantity=asset(payback, TOKEN_SYMBOL), .memo="Mine removed"}
                }.send();
            }
            
            table1.erase(c);
        } else {
            if (c->total > 0) {
                int64_t mark = c->mark;
                int64_t rawId = mark ^ creator.value;
                
                cardTable table2( name(_self), creator.value);
                auto origin = table2.find(rawId);
                check (origin != table2.end(), "No origin card");
                
                int64_t ct = current_time() / 1000;
                int64_t new_max_supply = origin->max_supply - c->total;
                check (new_max_supply >= 0, "Invalid max supply");
                
                table2.modify( origin, same_payer, [&]( auto& s ) {
                    s.max_supply = new_max_supply;
                    s.lastupdate = ct;
                });
            }

            table1.erase(c);
        }
        

        
    }

    void withdrawAction(uint64_t _self, const withdraw& withdraw) {
        name owner = withdraw.owner;
        require_auth( owner );

        int64_t id = withdraw.id;

        cardTable table1( name(_self), owner.value);

        auto c = table1.find(id);
        check (c != table1.end(), "No card");

        int64_t ct = current_time() / 1000;
        int64_t elapsed_ms = ct - c->lastupdate;

        check (elapsed_ms >= ms_per_1hour, "Must bigger than 1 hour");

        name creator = c->creator;
        check (owner != creator, "Mine card creator can not withdraw!");

        int64_t mark = c->mark;
        int64_t rawId = mark ^ creator.value;

        cardTable table2( name(_self), creator.value);
        auto origin = table2.find(rawId);
        check (origin != table2.end(), "No origin card");

        int64_t max_day = origin->price / 100000;
        if (origin->price == 0 || max_day > 365) {
            max_day = 365; // At most 1 year
        }

        if (max_day < 1) max_day = 1;

        int64_t max_minutes = max_day * min_per_day;
        int64_t elapsed_minutes = elapsed_ms / 60 / 1000;
        if (elapsed_minutes > max_minutes) elapsed_minutes = max_minutes;

        int64_t back_per_day = -origin->rampayed / origin->max_supply / origin->sell;
        if (origin->total < back_per_day)
            back_per_day = origin->total;

        int64_t payback;

        if (elapsed_minutes > min_per_day * 10) {
            payback = back_per_day * (elapsed_minutes / min_per_day) * c->total;
        } else {
            payback = back_per_day * elapsed_minutes / min_per_day * c->total;
        }

        if (-origin->rampayed < payback) payback = -origin->rampayed; // Withdraw all.
        check(payback > 0, "No token to withdraw");

        table2.modify( origin, same_payer, [&]( auto& s ) {
            s.rampayed = origin->rampayed + payback;
            s.lastupdate = ct;
        });

        table1.modify( c, same_payer, [&]( auto& s ) {
            s.lastupdate = ct;
        });

        action{
                permission_level{name(_self), "active"_n},
                name(ISSUER_ACCOUNT),
                "transfer"_n,
                currency::transfer{
                        .from=_self, .to=owner.value, .quantity=asset(payback, TOKEN_SYMBOL), .memo="Withdraw"}
        }.send();

    }

    // Reg or set a card
    void regAction(uint64_t _self, const reg& reg)
    {
        name from = reg.from;
        require_auth( from );

        int64_t id = reg.id;

        cardTable table1( name(_self), from.value);

        int64_t ct = current_time() / 1000;

        if (id <= 0) id = ct;

        // Change an old card
        auto c = table1.find(id);

        if (c == table1.end()) {
            // ===== Create NEW CARD =====

            // Can only create a card within 5 minutes
            check(abs(ct - id) < 300000, "Invalid card ID (More than 5 minutes)");

            auto total = reg.total;
            auto sell = reg.sell;
            int64_t max_supply = 0;
            int64_t rampayed = 0;

            if (sell < 0) {
                sell = -sell;
                rampayed = -1;

                check(total > 0 , "Can not create no paid-back mine card");
            } else {
                check(total >= 0, "Can not remove non-exist card");
                if (total > 0) max_supply = total;

                // When sell > 0 and total == 0, it is a card that can only sell by creator. (Max_Supply is minus)
                if (total == 0 && sell > 0) {
                    max_supply = -sell;
                    total = sell;
                } else if (sell > total) {
                    total = sell;
                }
            }

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
                s.rampayed = rampayed;
            });
        } else {
            // ===== Change old Card =====
            check(reg.total >= 0, "Total must be positive"); // Remove card change to action: remove
            
            // Modify card
            auto total = reg.total;
            auto max_supply = c->max_supply;
            auto sell = reg.sell;

            auto price = reg.price;
            if (price < 0) price = c->price; // Do not change

            if (c->rampayed < 0) {
                // mine card, can not change total or sell
                total = c->total;
                sell = c->sell;

                // price must be bigger than old
                check(price > c->price, "Mine price can only change to bigger");
            } else {
                if (total > 0 && total != c->total) {
                    // Change total and max_supply
                    check(c->max_supply == 0, "Must be first time to change total");
                    check(c->creator == from, "Must be owner to change total");
                    max_supply = total;
                }
                else {
                    total = c->total;
                }

                check(reg.sell <= total, "Total can not exceed sell");
                if (reg.sell < 0) sell = c->sell; // Do not change
            }

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
        //      =guest1111113,5     (Use card 5 as reference)
        //      ^guest1111113,5     (Support card 5 (a service card), only used in mine card, +rampayed only)
        // It will buy card 5 of guest1111113
        if (str[0] != '+' && str[0] != '=' && str[0] != '^') {
          return;
        }

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

        if (str[0] == '^') {
            check(c->rampayed < 0, "Must be a mine card");

            table1.modify(c, same_payer, [&](auto &s) {
                s.rampayed = c->rampayed - deposit;
                s.lastupdate = current_time() / 1000;
            });

            return;
        }

        if (c->max_supply < 0) {
            check(shopper == c->creator, "This card can only be sold by its creator.");
        }

        check(c->price > 0, "Price must > 0 to sell");

        if (c->rampayed < 0) {
            check(from != c->creator, "Creator can not buy a mine card of itself.");
            check (c->sell > 0, "duration must > 0");
        } else {
            check (c->sell > 0, "No more card");
        }

        bool isReference = (str[0] == '=');

        int64_t existedId = getExisted(_self, from, c->creator, c->mark);

        int64_t ram_pay = 0;

        if (existedId == 0) {
            ram_pay = getRamPay(isReference ? 0 : c->content.length());
        }
        deposit -= ram_pay;
        check (deposit >= c->price, "Can not buy this card");


        int64_t number = deposit / c->price;

        bool isMineCard = c->rampayed < 0;
        if (isMineCard) {
            table1.modify(c, same_payer, [&](auto &s) {
                s.max_supply = c->max_supply + number;
                s.rampayed = c->rampayed - number * c->price;
                s.lastupdate = current_time() / 1000;
            });
        } else {
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
                                    .from=_self, .to=shopper.value, .quantity=asset(c->rampayed,
                                                                                    TOKEN_SYMBOL), .memo="Send back ram payed"}
                    }.send();
                }

                table1.erase(c);
            }
        }

        // Add card for buyer
        addCard(_self, existedId, from, number, c->creator, c->mark, isMineCard ? 0 : c->max_supply,
                isReference ? "" : c->content, ram_pay);

        int64_t sendto = number * c->price;
        int64_t sendback = deposit - sendto;

        if (c->rampayed >= 0) {
            action{
                    permission_level{name(_self), "active"_n},
                    name(ISSUER_ACCOUNT),
                    "transfer"_n,
                    currency::transfer{
                            .from=_self, .to=shopper.value, .quantity=asset(sendto,
                                                                            TOKEN_SYMBOL), .memo="Send to shopper"}
            }.send();
        }

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
            return contentLength * EVD_RAM_RATE / 100;
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
      else if (action == "withdraw"_n.value) {
          withdrawAction(receiver, eosio::unpack_action_data<withdraw>());
      }
      else if (action == "remove"_n.value) {
          removeAction(receiver, eosio::unpack_action_data<withdraw>());
      }
      else if (action == "removemine"_n.value) {
          removemineAction(receiver, eosio::unpack_action_data<withdraw>());
      }

    }


};



extern "C" {

  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    impl().apply(receiver, code, action);
  }
}
