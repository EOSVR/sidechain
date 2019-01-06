## EVD Reward Contract（eosvrrewards）

[中文](README-cn.md)

It is to set rewards and send rewards to top accounts supported in [discuss contract](../comments/README.md);


### Set rewards

Send EVD to eosvrrewards to set rewards, settings of rewards should be:

  Maximum token per reward, Discuss Contract, Reward Interval(Second), Maximum number of accounts who can get reward, Reward Type, Reward Limit

Default values are:
  30 EVD, eosvrcomment, 86400*7(7 days), 10(10 accounts), 0(average and can rollback), 10 (1%)

Example:

```
# guest1111111 use 100K EVD to reward top 10 in eosvrcomment everyday. Every time 10K EVD will send to 10 accounts average.
# Everyone can get 1000 EVD at most;
# And, guest1111111 can stop this reward at any time (rewardtype=0).
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrrewards","quantity":"100000.0000 EVD","memo":"10000,eosvrcomment,84600,10,0"}' -p guest1111111


# Rewarder withdraw the published reward. (Type must be even (0, 2, 4...) )
cleos push action eosvrrewards withdraw '{"to":"account1"}' -p account1


# account1 push action to get the reward.
# And this command will refresh all rewards in eosvrrewards.
cleos push action eosvrrewards reward '{"to":"account1"}' -p account1

```


### Reward type

Only rewardtype is even, reward provider can withdraw the reward that do not send to others. If rewardtype is odd, it can not be revoked. Example: 0 can revoke, 1 can not revoke.

- When reward type is 2 or 3, the first account get 50%, others get the remain evenly;

- When reward type is 4 or 5, the first account get 50%, the second get 25%, the third get 12.5%, etc;

- In other condition, all accounts get reward evenly. 

When type >= 10:

- Must apply first, or no reward;

```
# Account "account1" apply reward sent by "rewardsender" and ranked by "eosvrcomment". It applied 100 EVD, and it can receive at most 100 EVD every reward.
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"100.0000 EVD","memo":"#APPLY#eosvrcomment,rewardsender"}' -p account1

# Can NOT change the apply, but can cancel the apply and re-apply. And when cancel apply, all supports to it will be sent back automatically. (Reject will not) 
# Account "account1" cancel the apply.
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"0.0001 EVD","memo":"#CANCEL#"}' -p account1

```

- Maximum reward of one account = (Last support - Current Reject * ((rewardtype - 10) / 2 + 1)) * rewardLimit(default: 10) / 1000 

Last support is the support of last reward invoked. Current reject is the reject in this reward.

This is to prevent the votes at the last minute. In this condition, rejecter have lots of time to reject.

When type < 10, maximum reward = (Support - Reject) * rewardLimit / 1000.

```


# Example, 0.1G EVD with 1000K rewards per 3 days. Total 100 times, about 1 year.
cleos push action eoslocktoken transfer '{"from":"eosvrmanager", "to":"eosvrrewards","quantity":"100000000.0000 EVD","memo":"1000000,eosvrcomment,259200,10,13,10"}' -p eosvrmanager

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

- 0-1, all accounts get the same rewards;

- 2-3, No1 get 50%, Other get same rewards;

- 4-5, No1 get 50%, No2 get 25%, No3 get 12.5% ...

- Other, all accounts get the same rewards;

When it is odd (1,3,5), reward owner can not withdraw.

And when type > 10:

- Account must apply how many EVD it wants at first. Every round, returned EVD will not exceed the applied value.
Also, one application can only last 33 days.

```

# account1 applies the reward from rewardsender that ranked by eosvrcomment.
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"100.0000 EVD","memo":"#APPLY#eosvrcomment,rewardsender"}' -p account1

# account1 can cancel the apply by:
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"0.0001 EVD","memo":"#CANCEL#"}' -p account1

# Note: when anyone cancel the application, all supports to it will be dismissed.

# Check the applies data
cleos get table eosvrrewards eosvrrewards appliess

```

- If anyone is against it, the against will multi by "(type - 10) / 2 + 1";
Example: when type = 20, against will multi 6.
