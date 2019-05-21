## Cards Contract

[中文](README-cn.md)

Cards can buy or sell.

#### Usage

##### Create a card

```
# guest1111111 create a card
cleos push action eoscardcards reg '{"id":0, "from":"guest1111111", "total":0, "sell":0, "price":0, "content":"vhuiejnm vn nvbxvbjbeiuo392387y8f7ywUFHJBVMXV"}' -p guest1111111
```

##### Change a card

```
# guest1111111 change a card
cleos push action eoscardcards reg '{"id":3, "from":"", "total":10, "sell":3, "price":1000, "content":""}' -p guest1111111
```

Note: When a value is 0 or empty, use old value and do not set it.

##### Buy/Sell 

```
# guest1111111 buy card (id=3) from guest1111112 with 2 EVD
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrcomment", "quantity": "2.0000 EVD", "memo":"+guest1111112,3"}' -p guest1111111

```

With it, guest1111112 will get a card with same content of card3 of guest1111111.

Because creator is still guest1111111, a user can not clone a card from old card.


##### Destroy a card

```
# guest1111111 destroy a card
cleos push action eoscardcards reg '{"id":3, "from":"", "total":-1, "sell":0, "price":0, "content":""}' -p guest1111111
```

If change total to -1, this card will be removed.
