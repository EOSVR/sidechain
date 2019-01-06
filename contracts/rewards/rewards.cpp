#include "rewards.hpp"

const uint64_t ISSUER_ACCOUNT = N(eoslocktoken);
const auto TOKEN_SYMBOL = string_to_symbol(4, "EVD");

const uint64_t useconds_per_sec = uint64_t(1000000);
const uint64_t useconds_per_33day = 33 * 24 * 3600 * useconds_per_sec;


const uint64_t MAX_TOKEN_PER_REWARD = 100000000; // 100 M
const uint64_t MAX_INTERVAL = 86400 * 365; // Max 1 year interval
const uint64_t MAX_RECEIVE_ACCOUNT = 1000;
const uint64_t MAX_REWARD_LIMIT = 100000;  // Can get 100 times of voted.

const uint64_t DEFAULT_COMMENTER = N(eosvrcomment);
const uint64_t DEFAULT_MAX_TOKEN_PER_REWARD = 100; // Default max token per reward is 100 EVD.
const uint64_t DEFAULT_INTERVAL = 84600 * 7; // 1 week
const uint64_t DEFAULT_RECEIVE_ACCOUNT = 10; // Default 10 accounts can get reward.
const uint64_t DEFAULT_REWARD_LIMIT = 10; // Can get 1% of voted.
const uint64_t DEFAULT_COMMENT_TYPE = 0; // 0: Can withdraw, even.

const uint64_t MIN_TOTAL_REWARD = 1000000; // Min total reward should be 100 EVD.
const uint64_t MIN_APPLY = 1000000; // Min apply is 100 EVD, because there is only big apply.

struct impl {

int64_t getParameterInt(string str, int64_t& ind, string delims, int64_t defaultValue) {
  if (ind < 0 || ind >= str.length()) {
    return defaultValue;
  }
  
  string result;
  auto next = str.find(delims, ind);
  if (next == string::npos) {
    result = str.substr(ind);
    ind = -1;
  } else {
    result = str.substr(ind, next - ind);
    ind = next + 1;
  }

  if (result.length() == 0) {
    return defaultValue;
  }
  
  return std::stoi(result);
}

bool begin_with(string memo, string str) {
    auto ind = memo.find(str);
    return (ind == 0);
}
  
account_name getParameterAccount(string str, int64_t& ind, string delims, account_name defaultValue) {
  if (ind < 0 || ind >= str.length()) {
    return defaultValue;
  }
  
  string result;
  auto next = str.find(delims, ind);
  if (next == string::npos) {
    result = str.substr(ind);
    ind = -1;
  } else {
    result = str.substr(ind, next - ind);
    ind = next + 1;
  }
  
  if (result.length() == 0) {
    return defaultValue;
  }

  eosio_assert(result.length() <= 12, "Account length must less or equal 12.");
  
  return eosio::string_to_name(result.c_str());
}

int64_t getBalance(uint64_t issuer, uint64_t symbol, uint64_t account) {
    eosio::token t(issuer);
    const auto sym_name = eosio::symbol_type(symbol).name();
    return t.get_balance(account, sym_name ).amount;
}


int64_t getCurrentSupport(account_name commenter, account_name account) {
    gcommentTable table1( commenter, commenter );

    auto item = table1.find(account);

    if (item != table1.end()) {
        return item->total - item->totaldown;
    }

    return 0;
}

void setApply(uint64_t _self, account_name from, int64_t amount, string memo_part) {
    eosio_assert(amount >= MIN_APPLY, "Apply must be greater than 100 EVD");

    auto next = memo_part.find(",");
    eosio_assert(next != string::npos, "Must be commenter,owner");

    auto ranker = memo_part.substr(0, next);
    auto reward_owner_str = memo_part.substr(next + 1);

    eosio_assert(ranker.length() > 0 && ranker.length() <= 12
        && reward_owner_str.length() > 0 && reward_owner_str.length() <= 12
        , "Account name length must be 1-12");

    auto commenter = eosio::string_to_name(ranker.c_str());
    auto reward_owner = eosio::string_to_name(reward_owner_str.c_str());

    auto balance1 = getBalance(ISSUER_ACCOUNT, TOKEN_SYMBOL, commenter);
    auto balance2 = getBalance(ISSUER_ACCOUNT, TOKEN_SYMBOL, reward_owner);

    eosio_assert(balance1 > 0 && balance2 > 0, "Must be valid commenter and reward_owner (have EVD)");


    //auto support = getCurrentSupport(commenter, from);

    applyTable table1(_self, _self);

    auto t1 = table1.find(from);
    eosio_assert(t1 == table1.end(), "Can not apply again!");

    table1.emplace( _self, [&]( auto& s ) {
      s.account = from;
      s.deposit = amount;

      s.ranker = commenter;
      s.reward_owner = reward_owner;

      s.last_support = 0; // Support can only get by reward
      s.start_time = 0;
    });
}

void cancelApply(uint64_t _self, account_name from, int64_t amount) {
    applyTable table1(_self, _self);

    auto t1 = table1.find(from);
    eosio_assert(t1 != table1.end(), "Can not find apply");

    auto back = amount + t1->deposit;

    backToDeposit(_self, from, back);

    table1.erase(t1);

    depositSendBack(_self, from);

    action{
          permission_level{_self, N(active)},
          t1->ranker,
          N(dismiss),
          dismiss{
              .account=from
              }
    }.send();
}
  
void transferAction (uint64_t _self, uint64_t code) {
  auto data = unpack_action_data<token::transfer_args>();
  if(data.from == _self || data.to != _self)
    return;

  if (code != ISSUER_ACCOUNT)
    return;  

  eosio_assert(data.quantity.is_valid(), "Invalid quantity.");

  account_name from = data.from;
  account_name to = data.to;

  require_auth(from);

  eosio_assert(data.quantity.symbol == TOKEN_SYMBOL, "Invalid symbol1");
  eosio_assert(data.quantity.is_valid(), "Invalid quantity");

  // MEMO Like: Maxtoken,eosvrcomment,interval,receiver_number,rewardtype,rewardlimit

  auto str = data.memo;

  // ===== User can APPLY for special rewards (Type >= 10), and get the rewards same as apply.
  // User can CANCEL and re-APPLY for more rewards.
  // And when CANCEL, all supports to account will be removed.
  if (begin_with(str, "#APPLY#")) {
    setApply(_self, from, data.quantity.amount, str.substr(7)); // #APPLY#RANKER,OWNER, example: #APPLY#eosvrcomment,wsdfz1
    return;
  } else if (begin_with(str, "#CANCEL#")) {
    cancelApply(_self, from, data.quantity.amount);
    return;
/* === DEBUG START ===
  } else if (begin_with(str, "#RESET#")) {
    rewardTable table2( _self, _self );
    auto current = table2.find(from);
    eosio_assert(current != table2.end(), "No account.");

    table2.modify( current, 0, [&]( auto& s ) {
       s.lastreward = 0; // Let it can reward again
    });
    return;
// === DEBUG END */
  }


  int64_t ind = 0;

  eosio_assert(data.quantity.amount >= MIN_TOTAL_REWARD, "Total rewards must be greater than 100 EVD"); // Or it is less than the price of consumed memory

  int64_t       maxtoken = getParameterInt(str, ind, ",", DEFAULT_MAX_TOKEN_PER_REWARD);
  eosio_assert(maxtoken > 0 && maxtoken <= MAX_TOKEN_PER_REWARD, "Max token must be 1 - 100000000 (100M)");
  
  account_name  commenter = getParameterAccount(str, ind, ",", DEFAULT_COMMENTER);

  int64_t       interval = getParameterInt(str, ind, ",", DEFAULT_INTERVAL);
  eosio_assert(interval >= 60 && interval <= MAX_INTERVAL, "interval must be 60 - 31,536,000 seconds (1 year)");
  
  int64_t       receiver_number = getParameterInt(str, ind, ",", DEFAULT_RECEIVE_ACCOUNT);
  eosio_assert(receiver_number > 0 && receiver_number <= MAX_RECEIVE_ACCOUNT, "Receiver_number must be 1 - 1000");
  
  int64_t       rewardtype = getParameterInt(str, ind, ",", DEFAULT_COMMENT_TYPE);
  eosio_assert(rewardtype >= 0 && rewardtype <= 20000, "Reward_type must be 0-20000");

  int64_t       rewardlimit = getParameterInt(str, ind, ",", DEFAULT_REWARD_LIMIT);
  eosio_assert(rewardlimit > 0 && rewardlimit <= MAX_REWARD_LIMIT, "Reward_limit must be 1 - 100000 (100 times)");
  
  rewardTable table1( _self, _self );
  table1.emplace( _self, [&]( auto& s ) {
    s.deposit = data.quantity.amount;
    s.maxtoken = maxtoken;  // maxtoken to reward every time

    s.ranker = commenter;
    s.receiver_number = receiver_number;
    s.rewardtype = rewardtype;

    s.interval = interval;
    s.lastreward = current_time();
    s.rewardlimit = rewardlimit;

    s.owner = from;
  });
}

// Type: 0, Average
//       1, No1 get 50%, Others get the other.
//       2, No1 get 50%, No2 get 25% , No3 get 12.5% ....
// A user will receiver_number,
// current_max_reward is the total this comment received.
int64_t getReward(
    int64_t total_max_reward,
    int64_t current_max_reward,
    int64_t index,
    int64_t receiver_number,
    int64_t rewardtype)
{
  int64_t trueType = rewardtype / 2;

  int64_t result = total_max_reward;
  if (trueType == 1) {
    if (receiver_number <= 1)
        result = total_max_reward;
    else if (index == 0)
        result = total_max_reward / 2;
    else
        result = total_max_reward / (receiver_number - 1) / 2;
  } else if (trueType == 2) { // EXPONENT v2
    for (int i = 0; i < index + 1; i++) {
      result /= 2;
    }
  } else {
    // AVERAGE
    result = total_max_reward / receiver_number;
  }

  if (result > current_max_reward) {
    result = current_max_reward;
  }
  
  return result;
}

// Reward in max_reward (10000 EVD), to receiver_number (10) accounts, in commenter and rewardtype.
int64_t calculateOneReward(uint64_t _self, int64_t max_reward, account_name commenter,
    int64_t receiver_number, int64_t rewardtype, int64_t rewardlimit) {

  if (rewardlimit <= 0 || rewardlimit > 1000) {
    rewardlimit = DEFAULT_REWARD_LIMIT;
  }

  gcommentTable table1( commenter, commenter );
  applyTable tableApply(_self, _self);

  auto item_index = table1.get_index< N(bytotal)>();
  auto item_itr = item_index.begin();

  int64_t result = 0;
  int64_t index = 0;
  while (item_itr != item_index.end()
         && index < receiver_number) {
    if (item_itr->total <= 0) break;

    int64_t current_max_reward = item_itr->total * 1.0 * rewardlimit / 1000;

    if (rewardtype >= 10) {

        // Must apply first, max reward that can get is applied deposit.
        auto applied = tableApply.find(item_itr->account);
        if (applied == tableApply.end()) {
            index++;
            item_itr++;
            continue;
        }

        auto support = applied->last_support;

        int64_t newSupport = item_itr->total - item_itr->totaldown;
        int64_t newStartTime = applied->start_time;
        if (newSupport > 0 && newStartTime == 0)
            newStartTime = current_time();

        tableApply.modify( applied, 0, [&]( auto& s ) {
            s.last_support = newSupport;
            s.start_time = newStartTime;
        });

        int64_t reject_multi = ((rewardtype - 10) / 2 + 1);
        eosio_assert(reject_multi > 0 && reject_multi <= 10000, "Reject must between 1-10000");

        current_max_reward = (support + item_itr->totaldown * reject_multi) * rewardlimit / 1000;

        // When timeout (After 33 days), NO reward.
        if (applied->start_time > 0 && current_time() - applied->start_time >= useconds_per_33day) {
            current_max_reward = 0;
        } else if (current_max_reward > applied->deposit) {
            current_max_reward = applied->deposit;
        }

        if (current_max_reward <= 0) {
            index++;
            item_itr++;
            continue;
        }
    }

    int64_t oneReward = getReward(max_reward, current_max_reward, index, receiver_number, rewardtype);

    if (oneReward <= 0) break;
    
    backToDeposit(_self, item_itr->account, oneReward);
    result += oneReward;
    
    index++;
    item_itr++;
  }  
    
  return result;
}
  
void backToDeposit(uint64_t _self, uint64_t account, uint64_t backToken) {
  depositTable table1( _self, _self );

  auto current = table1.find(account);

  if (current != table1.end()) {
    table1.modify(current, 0, [&]( auto& s ) {
        s.deposit += backToken;
      });
  } else {
    table1.emplace( _self, [&]( auto& s ) {
      s.deposit = backToken;
      s.to = account;
    });
  }
}

// Send reward back, all reward will lock 2 hours.
void sendBack(uint64_t _self, uint64_t account, uint64_t backToken) {
  if (backToken > 0) {
    auto back = asset( backToken, TOKEN_SYMBOL );

    action{
          permission_level{_self, N(active)},
          ISSUER_ACCOUNT,
          N(transfer),
          token::transfer_args{
              .from=_self, .to=account, .quantity=back, 
              .memo="#TIME# 86400"} // Transfer takes 1 day to arrive
      }.send();
  }
}

bool calculateRewards(uint64_t _self) {
  auto now = current_time();
  
  rewardTable table( _self, _self );

  bool changed = false;

  auto itr = table.begin();
  while (itr != table.end()) {
    if (now <= itr->interval * uint64_t(1000000) + itr->lastreward) {
      ++itr;
      continue;
    }

    auto used = itr->maxtoken * 10000;
    if (itr->deposit < used) used = itr->deposit;
    used = calculateOneReward(_self, used, itr->ranker, itr->receiver_number, itr->rewardtype, itr->rewardlimit);

    auto newDeposit = itr->deposit - used;
    eosio_assert(newDeposit >= 0, "Must be positive after reward");

    if (newDeposit == 0) {
      // Remove the reward
      backToDeposit(_self, itr->owner, itr->deposit);

      auto old = itr;
      ++itr;
      table.erase(old);
    } else {
      table.modify(itr, 0, [&]( auto& s ) {
        s.deposit = newDeposit;
        s.lastreward = now;
      });
      
      ++itr;
    }

    changed = true;
  }

  return changed;
}

bool depositSendBack(uint64_t _self, account_name account) {
  depositTable table1( _self, _self );

  auto current = table1.find(account);

  if (current != table1.end()) {
    sendBack(_self, account, current->deposit);

    table1.erase(current);

    return true;
  }

  return false;
}
    
void rewardAction(uint64_t _self, const reward& dat) {
  auto account = dat.to;
  require_auth( account );
  
  bool ok1 = calculateRewards(_self);

  bool ok2 = depositSendBack(_self, account);

  eosio_assert(ok1 || ok2, "Nothing changed at all.");

}
    
void withdrawAction(uint64_t _self, const reward& dat) {
  auto account = dat.to;
  require_auth( account );

  rewardTable table1( _self, _self );

  auto current = table1.find(account);
  
  eosio_assert(current != table1.end(), "No need to withdraw.");

  eosio_assert(current->rewardtype % 2 == 0, 
               "Can not withdraw with type ODD Type (1,3,5...)");

  backToDeposit(_self, account, current->deposit);

  table1.erase(current);

  depositSendBack(_self, account);
}

void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
  if (action == N(transfer)) {
    transferAction(receiver, code);
    return;
  }

  if (action == N(withdraw)) {
    withdrawAction(receiver, eosio::unpack_action_data<reward>());
  } else if (action == N(reward)) {
    rewardAction(receiver, eosio::unpack_action_data<reward>());
  }
}


};



extern "C" {
  void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    impl().apply(receiver, code, action);
  }
}
