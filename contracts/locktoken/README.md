## Lock Token

The token that can be locked by other or time.


### Usage

Same as eosio.token. And add the following in transfer:


1，Allow transfer to self, this will refresh table by time. If add #LIMIT# in memo, it can set the LimitPerMonth of transfer. Min is 1/100 per month.
Can only change it to smaller, can not change it to bigger.

Example:

```
cleos push action eoslocktoken transfer '{"from":"account1", "to":"account1","quantity":"0.0000 EVD","memo":"#LIMIT#50"}' -p account1
```

Will set account only can transfer out 50% token per month. (Range is (1-50) percent per month)

2, User only can transfer within limit and locked.
  Token can transfer in one day = (Token (Do not include timelock tokens) - Lock token) * LimitPerDay


3, When transfer to other with memo #LOCK#..., it will lock the token of other, instead of transfer.
Example:

```
cleos push action eoslocktoken transfer '{"from":"account1", "to":"helloworld11","quantity":"100.0000 EVD","memo":"#LOCK# I do not agree with your comments."}' -p account1
```

Note: In table, lock value is negative.

4, When transfer to other with memo #UNLOCK#XXX, it will unlock the token of other, instead of transfer.
Example:

```
cleos push action eoslocktoken transfer '{"from":"account1", "to":"helloworld11","quantity":"100.0000 EVD","memo":"#UNLOCK# OK"}' -p account1
```

Note: Only can unlock to 0, can not unlock to a positive value.

5, When transfer to other with memo #TIME#XXX, these token will unlock after these seconds passed.
Example:

```
cleos push action eoslocktoken transfer '{"from":"account1", "to":"helloworld11","quantity":"100.0000 EVD","memo":"#TIME# 86400"}' -p account1
```

These 100 EVD will unlock after 1 day.

6, All token can be transfer = Fluid token - Locked token recorded in eoslocktoken.

7, If A lock B, B can give 90% token to A and remove the lock. Use "#CONFIRM#":
Example:

```
cleos push action eoslocktoken transfer '{"from":"account1", "to":"account2","quantity":"0.0001 EVD","memo":"#CONFIRM#"}' -p account1
```

Shops may use it to get tokens from customs by lock. And custom can confirm the lock and give token to shop.


### Limitation

When limit is 1-50, an account only allow transfer limit% token per month.

Note: limit% = transfered / remain.

Example:

If A have 30 EVD and limitation is 50, A can only transfer 10 at one time because 50%=10/(30-10).

If A want to transfer more EVD, A can transfer many times.


Note:

Limitation can only set to smaller number. Example: Can change from 50 to 20. Can NOT change from 20 to 30.

Limit weight: ( code is in contract: comments )

```

25 - 50: 10;

11 - 24: 20;

5 - 10: 30;

2 - 4: (9 - limit) * 10;

1: 100;

Other: 1;

```


### Transfer with HASH LIMIT

Example:

```

cleos push action eoslocktoken transfer '{"from":"account1", "to":"account2","quantity":"10.0000 EVD","memo":"#HASH#2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824,86400"}' -p account1

# This command do not have privilege limit, anyone can confirm
cleos push action eoslocktoken confirm '{"from":"account1", "to":"account2","key":"hello,world","executer":"account3"}' -p account3


```


### ARRIVAL OF DELAY TRANSFER

A delay transfer can not arrive automatically. The owner of account must run a transfer to let it appear. Like:

```
cleos push action eoslocktoken transfer  '{"from":"account2", "to":"account2","quantity":"0.0000 EVD","memo":""}' -p account2
```


