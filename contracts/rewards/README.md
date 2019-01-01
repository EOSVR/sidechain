## EVR REWARDS

Send back EVR to the account who get top X in comments.


### Usage

Account abcdabcdabcd apply for rewards.
// 50000,eosvrcomment,84600,10,0,10
//      50K EVD (Every time, instead of times),
//      commenter,
//      7*86400 (Interval),
//      10 (amount of reward_accounts, 1-1000)
//      0 (reward type)
//      10 (reward limit, default 10)

```

# Apply for reward of account1
cleos push action eosvrrewards reward '{"to":"account1"}' -p account1

# Rewarder publish a reward for other people.
cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrrewards","quantity":"100000.0000 EVD","memo":"30,eosvrcomment,84600,10,0,5"}' -p account1

# Rewarder withdraw the published reward. (Type must be even (0, 2, 4...) )
cleos push action eosvrrewards withdraw '{"to":"account1"}' -p account1

```


### Memo

Memo should added as:

"times,ranker,interval,receiver,reward_type"

Default is: "30,eosvrcomment,84600,10,0,10"


-times(How many times it should send)

-ranker(Which commenter account should be received)

-interval(How many seconds a reward can be triggered)

-receiver(number of accounts can receive rewards)

-reward_type(type of reward)


### Types

- 0-1, even

- 2-3, No1 get 50%, Other get even.

- 4-5, No1 get 50%, No2 get 25%, No3 get 12.5% ...

- Other, even


When it is odd (1,3,5), reward owner can not withdraw.

And when type > 10, it is even, and:

- Can not exceed the applied value. Account can apply by:

```

# account1 applies the reward from rewardsender that ranked by eosvrcomment.
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"100.0000 EVD","memo":"#APPLY#eosvrcomment,rewardsender"}' -p account1

# account1 can cancel the apply by:
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"0.0001 EVD","memo":"#CANCEL#"}' -p account1

# Note: when anyone cancel the application, all supports to it will be dismissed.

```

- If anyone is against it, the against will multi by "(type - 10) / 2 + 1";
Example: when type = 20, against will multi 6.
