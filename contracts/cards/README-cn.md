## Card合约

[English](README.md)

用户使用卡片交易

### 使用方式

##### 建卡

```
# guest1111111 create a card
cleos push action eoscardcards reg '{"id":0, "from":"guest1111111", "total":0, "sell":0, "price":0, "content":"vhuiejnm vn nvbxvbjbeiuo392387y8f7ywUFHJBVMXV"}' -p guest1111111
```

##### 修改卡片内容

```
# guest1111111 change a card
cleos push action eoscardcards reg '{"id":3, "from":"", "total":10, "sell":3, "price":1000, "content":""}' -p guest1111111
```

规则：
1，creator 建立时自动加入，其他时候无法修改；
2，total 只能设置一次，再设就只能设成 -1，表示删除；
3，sell 不能超过 total；

注意：如果数据是0或空，表示不改变。

##### 买卖

```
# guest1111111 buy card (id=3) from guest1111112 with 2 EVD
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrcomment", "quantity": "2.0000 EVD", "memo":"+guest1111112,3"}' -p guest1111111

```

买卖后，新用户将新建一张卡片。卡片的 creator 将不变，还是guest1111111，用来防止随意克隆卡片。


##### 销毁卡片

```
# guest1111111 destroy a card
cleos push action eoscardcards reg '{"id":3, "from":"", "total":-1, "sell":0, "price":0, "content":""}' -p guest1111111
```

参数 "total" 改成 -1 后，卡片将被销毁。
