
#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include <string>
#include <eosiolib/asset.hpp>
#include "../comments/comments.hpp"

using namespace eosio;

using std::string;


struct reward {
  account_name to;
  
  EOSLIB_SERIALIZE( reward, (to))
};

// When one account call rewards, all rewards will be put into deposits.
// And other accounts can withdraw their rewards from deposits.
struct deposits {
  account_name to;
  int64_t deposit;
  
  auto primary_key() const { return to; }
  
  EOSLIB_SERIALIZE( deposits, (to)(deposit))
};

typedef eosio::multi_index<N(depositss), deposits> depositTable;

struct rewards {
  int64_t   deposit;
  
  int64_t   maxtoken;
  account_name ranker;
  int64_t   receiver_number;
  int64_t   interval;
  int64_t   rewardtype;

  int64_t   lastreward;  // time of last reward

  account_name owner;

  int64_t   rewardlimit; // For certain reward, max_reward = rewardlimit * total(gcomments) / 1000;

  auto primary_key() const { return owner; }

  EOSLIB_SERIALIZE( rewards, (deposit)(maxtoken)(ranker)(receiver_number)(interval)(rewardtype)(lastreward)(owner)(rewardlimit) )
  //EOSLIB_SERIALIZE( rewards, (deposit)(maxtoken)(ranker)(receiver_number)(interval)(rewardtype)(lastreward)(owner) ) // Old
};

typedef eosio::multi_index<N(rewardss), rewards> rewardTable;

// User can apply for rewards by certain reward_owner.
// Some rewards (Example: eosvradvancers) only reward to the account who applied.
struct applies {
  account_name account;
  int64_t deposit;
  account_name ranker; // commenter (such as: eosvrcomment)
  account_name reward_owner; // The user who send the reward

  // In last reward, support to this account (Do not include against)
  // It records when account "apply", "reward" (or others reward)
  int64_t last_support;

  auto primary_key() const { return account; }

  EOSLIB_SERIALIZE( applies, (account)(deposit)(ranker)(reward_owner)(last_support))
};

typedef eosio::multi_index<N(appliess), applies> applyTable;
