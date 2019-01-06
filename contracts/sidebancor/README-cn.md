## EVR BANCOR （eosvrtokenss）

[English](README.md)

- 使用 Bancor 算法来交换两种代币，参考了 EOS 中的 bancor。

- 用户除了可以交易，也可以投资。


### 使用方式

```
#EOS to EVR
cleos transfer abcdabcdabcd eosvrmarkets "10.0000 EOS"

#EVR to EOS
cleos push action eosvrtokenss transfer '{"from":"abcdabcdabcd", "to":"eosvrmarkets","quantity":"10.0000 EVR","memo":""}' -p abcdabcdabcd
```

### 投资

用户可以用交易用的两种代币进行投资。之后其他用户的交易会收取千分之五的手续费，这些手续费就是这个投资的利润。

```

# 投资 (最低 100 EOS/EVD)
cleos transfer account1 eosvrmarkets "100.0000 EOS" "#INVEST#"

cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"100.0000 EVD","memo":"#INVEST#"}' -p account1

```

之后，用户可以收回部分或全部投资，并且获得利润，也就是用户的交易费用。

```

# 收回投资以及交易费用
# 例子: account1 从投资和利润中收回了 50 EVD 
cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"100.0000 EVD","memo":"#WITHDRAW#"}' -p account1

```

投资和利润可以通过如下命令查看：

```
cleos get table eosvrmarkets eosvrmarkets depositss1
cleos get table eosvrmarkets eosvrmarkets depositss2
```

结果类似：
{
  "rows": [{
      "account": "",
      "deposit": 81000051
    },{
      "account": "eosvrmarkets",
      "deposit": 449
    },{
      "account": "account1",
      "deposit": 80000056
    },{
      "account": "account2",
      "deposit": 999995
    }
  ],
  "more": false
}

其中 account 为空的是总投资，account 为 "eosvrmarkets" 的是总利润。

注意：投资与撤资都将改变币价，最好是两种币同时投，或者都投少量。
