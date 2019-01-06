## Comment Contract

[中文](README-cn.md)

One account can comment itself or another account. Every comment can use EVD to support or against.

#### Usage

##### comment 

One account comment another

```
# guest1111111 comment guest1111112
cleos push action eosvrcomment comment '{"from":"guest1111111", "to":"guest1111112", "memo":"hello, good."}' -p guest1111111
```


##### transfer 

Support or against another by EVD;

```
# guest1111111 support guest1111112 with 2 EVD in contract eosvrcomment
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrcomment", "quantity": "2.0000 EVD", "memo":"+guest1111112"}' -p guest1111111

```


##### withdraw 

Take back the support or against to another;

```
# guest1111111 withdraw the comment EVD to guest1111112 in contract eosvrcomment
cleos push action eosvrcomment withdraw '{"from":"guest1111111", "to":"guest1111112"}' -p guest1111111

```

##### Weight for limitation

If an account have a limitation to transfer, it can not use all of its EVD to support.

It can only use limited EVD.

And there is a additional weight multiply for it:

- When Limit is 11 - 50, multiply by 5 * 100 / Limit;

- When Limit is 1 - 10, multiply by 10 * 100 / Limit;

- Other: No change;

Example: when transfer limit is 1% per month, its comment will multiply by 1000.


##### Description or Portrait

It can use for the self-introduction or portrait.

The common accounts of them are: eosvrcomment (for description), and evrportraits  (for image in base64)

Get them like the following:

```

cleos get table eosvrcomment guest1111113 commentss

cleos get table evrportraits guest1111113 commentss

```