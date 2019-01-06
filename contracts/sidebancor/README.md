## BANCOR in Side-Chain

[中文](README-cn.md)

- Use Bancor algorithm to exchange two tokens. Reference the bancor code in EOS contract.

- User can invest too.

### Usage

```
#EOS to EVR
cleos transfer abcdabcdabcd eosvrmarkets "10.0000 EOS"

#EVR to EOS
cleos push action eosvrtokenss transfer '{"from":"abcdabcdabcd", "to":"eosvrmarkets","quantity":"10.0000 EVR","memo":""}' -p abcdabcdabcd
```

### Invest

Users can invest the exchange contract by both tokens. All users exchange in it with 0.5% fee. These fee is the profits of the investment.

```

#INVEST (min 100 EOS/EVD)
cleos transfer account1 eosvrmarkets "100.0000 EOS" "#INVEST#"

cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"100.0000 EVD","memo":"#INVEST#"}' -p account1

```

User can withdraw the investment and get the bonus which is from the exchange fee.

```

# Withdraw the invested (with bonus if any)
# Example: account1 withdraw 50 EVD from invest and profits.
cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"50.0000 EVD","memo":"#WITHDRAW#"}' -p account1

```

Investment and profits can check by:

```
cleos get table eosvrmarkets eosvrmarkets depositss1
cleos get table eosvrmarkets eosvrmarkets depositss2
```

Result like:
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

In it, total amount of all investments is the deposit of account ""(empty). Deposit in account "eosvrmarkets" is the total of profits.

NOTE: Invest will change the exchange rate. It is better to invest in both tokens or invest with small amount.