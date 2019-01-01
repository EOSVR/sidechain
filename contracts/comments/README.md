## Comment Contract （eosvrcomment）

One account can comment itself or another account. Every comment can use EVD to support or against.


#### Usage

##### comment

One account comment another

```
# guest1111111 comment guest1111112
cleos push action eosvrcomment comment '{"from":"guest1111111", "to":"guest1111112", "memo":"hello, good."}' -p guest1111111
```


##### transfer

For or against another by EVD;

```
# guest1111111 support guest1111112 with 2 EVD in contract eosvrcomment
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrcomment", "quantity": "2.0000 EVD", "memo":"+guest1111112"}' -p guest1111111

```


##### withdraw

Take back the support or against to another. This operation can only run after 3 days.

```
# guest1111111 withdraw the comment EVD to guest1111112 in contract eosvrcomment
cleos push action eosvrcomment withdraw '{"from":"guest1111111", "to":"guest1111112"}' -p guest1111111

```

