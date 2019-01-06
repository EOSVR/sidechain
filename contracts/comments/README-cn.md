## 评论用合约

[English](README.md)

用户可以互相评论，也可以给自己写说明，或者用来给自己存储数据。每一对评价都可以使用 EVD 进行支持或反对。

### 使用方式

#### comment

一个用户用一段话评价另一个用户或自己；

```
# guest1111111 评价 guest1111112
cleos push action eosvrcomment comment '{"from":"guest1111111", "to":"guest1111112", "memo":"hello, good."}' -p guest1111111
```


#### transfer

给某个评价以 EVD 支持或者反对；

```
# guest1111111 在 eosvrcomment 这个评价合约下，对 guest1111112 使用 2 EVD 进行支持评价
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrcomment", "quantity": "2.0000 EVD", "memo":"+guest1111112"}' -p guest1111111

```


#### withdraw

收回给某个评价的 EVD 支持或者反对；

```
# guest1111111 在 eosvrcomment 这个评价合约下，收回对 guest1111112 评价花费的所有 EVD
cleos push action eosvrcomment withdraw '{"from":"guest1111111", "to":"guest1111112"}' -p guest1111111

```

### 因为限制产生的评论加成

如果一个帐号设置了限制，它只能使用它的一部分EVD 进行评论支持或者反对。为了鼓励大家进行限制，做了限制的人有如下加成：

- 转出限制为 11% - 50%, 评论加成为： 5 * 100 / 限制;

- 转出限制为 1% - 10%, 评论加成为： 10 * 100 / 限制;

- 其他无变化

比如：一个帐号一个月只能转出 1%，那么它评论的加成为：1000倍。


### 自我描叙与头像

这也可以用来放置自我描叙，或者头像。

通常使用帐号: eosvrcomment 来放描叙， evrportraits 放图片 (base64格式)

可以通过如下命令来获取:

```

cleos get table eosvrcomment guest1111113 commentss

cleos get table evrportraits guest1111113 commentss

```

