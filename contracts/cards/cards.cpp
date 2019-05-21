#include "cards.hpp"


const uint64_t ISSUER_ACCOUNT = "eoslocktoken"_n.value;
const auto TOKEN_SYMBOL = symbol(symbol_code("EVD"), 4);


const uint64_t useconds_per_sec = uint64_t(1000000);
const uint64_t useconds_per_day = 24 * 3600 * useconds_per_sec;
const uint64_t useconds_10min = 600 * useconds_per_sec;


struct impl {


// Reg or set a card
//    int64_t   id;
//    account_name from;
//    int64_t   total;
//    int64_t   sell;
//    int64_t   price;
//    string    content;
void regAction(uint64_t _self, const reg& reg)
{
    name from = reg.from;
    require_auth( from );

    int64_t id = reg.id;
  
    cardTable table1( name(_self), from.value);
  
    int64_t ct = current_time();
  
    if (id <= 0) {
      // Create a new card
      if (table1.begin() == table1.end()) {
        id = 1; // First card
      } else {
        auto last_card = table1.rend();
        id = last_card->id + 1; // Other card
      }
      
      auto total = reg.total;
      if (total < 0) total = 0;
        
      auto sell = reg.sell;
      if (sell < total) sell = 0;
        
      auto price = reg.price;
      if (price < 0) price = 0;
      
      table1.emplace( from, [&]( auto& s ) {
        s.id = id;
        s.creator = from;
        s.mark = from.value ^ id ^ ct; // id XOR from XOR time
        s.total = total;
        s.sell = sell;
        s.price = price;
        s.lastupdate = ct;
        s.content = reg.content;
      });      
    } else {
      // Change an old card
      auto c = table1.find(reg.id);
      eosio_assert(c != table1.end(), "Invalid card ID");
      
      if (reg.total < 0) {
        // Remove card
        table1.erase(c);
      } else {
        // Modify card
          
        auto total = reg.total;
        if (c->total > 0) {
           total = c->total;
        }

        eosio_assert(reg.sell <= reg.total, "Total can not exceed sell");
        
        auto sell = reg.sell;
        if (reg.sell < 0) sell = c->sell;
          
        auto price = reg.price;
        if (reg.price < 0) price = c->price;
            
        if (reg.content.length() < 2) {
          table1.modify( c, from, [&]( auto& s ) {
            s.total = total;
            s.sell = sell;
            s.price = price;
            s.lastupdate = ct;
          });
        } else {
          table1.modify( c, from, [&]( auto& s ) {
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
    if(data.from.value == _self || data.to.value != _self)
      return;

    // Only work when transfer EVD from eoslocktoken
    if (code != ISSUER_ACCOUNT)
      return;

    eosio_assert(data.quantity.is_valid(), "Invalid quantity");

    auto symbol = data.quantity.symbol;
    eosio_assert(symbol == TOKEN_SYMBOL, "Only support EVD");

    int64_t deposit = data.quantity.amount;
    auto str = data.memo;

    // Memo should like: +guest11111113,5
    // It will buy card 5 of guest1111113
    if (str[0] != '+') {
      return;
    }
  
    auto ind = str.find(",");
    if (ind < 0) {
      return;
    }
  
    auto shopper = str.substr(1, ind-1);
    auto cardNo = str.substr(ind+1);
  
    eosio_assert(shopper.length() <= 12, "Account length must less or equal 12");
    auto account = name(shopper.c_str());

    print("todo, buy ", shopper, " , ", cardNo);
//    // Set comment table
//    commentTable table1( _self, account);
//    int64_t depositChanged = 0;
//    int64_t depositDownChanged = 0;
//    int64_t weight = 1;
//
//    auto c = table1.find(data.from);
//    if (c == table1.end()) {
//      eosio_assert(data.quantity.amount >= MIN_EVD,
//        "Amount must be greater than MIN EVD because it takes RAM of contract to save it");
//
//      weight = getWeight(data.from);
//
//      depositChanged = deposit;
//      if (deposit < 0)
//        depositDownChanged = deposit;
//
//      // RAM pay by contract.
//      table1.emplace( _self, [&]( auto& s ) {
//          s.from = data.from;
//          s.deposit = deposit;
//          s.weight = weight;
//          s.lastupdate = current_time();
//      });
//    } else {
//        weight = c->weight;
//        if (weight < 1 || weight > MAX_WEIGHT) weight = 1;
//
//        int64_t oldDeposit = c->deposit;
//        eosio_assert(
//            (oldDeposit >= 0 && deposit >= 0) || (oldDeposit <= 0 && deposit <= 0),
//            "Can not change from positive to negative");
//
//        depositChanged = deposit;
//        depositDownChanged = (deposit < 0) ? deposit : 0;
//
//        table1.modify( c, 0, [&]( auto& s ) {
//            s.deposit += depositChanged;
//            s.lastupdate = current_time();
//        });
//    }
//
//    // Create opposite comment table if no exist
//    createTableIfEmpty(_self, account, data.from);
//
//    // Add details to global table
//    gcommentTable table2(_self, _self);
//    auto current2 = table2.find(account);
//    if (current2 == table2.end()) {
//      table2.emplace( _self, [&]( auto& s ) {
//          s.account = account;
//          s.total = depositChanged * weight;
//          s.totaldown = depositDownChanged * weight;
//          s.lastupdate = current_time();
//      });
//    } else {
//        table2.modify(current2, 0, [&]( auto& s ) {
//            int64_t t1 = s.total + depositChanged * weight;
//
//            int64_t t2 = s.totaldown + depositDownChanged * weight;
//
//            s.total = t1;
//            s.totaldown = t2;
//            s.lastupdate = current_time();
//        });
//    }
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
