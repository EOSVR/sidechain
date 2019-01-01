## BANCOR for Side-Chain

Use bancor algorithm to exchange EVD <-> EOS. Referenced from EOS bancor.

At first, account do not have EOS or EVD. Other accounts can invest it. Every exchange in bancor will take exchange fee, these fee will record. And when an investor withdraw, it can take some bonus.

If there are bonus in contract, new investor will separate its invest into two parts. One part to bonus, one part to invest. This is to make sure when new invest come, old invests will have same bonus if withdraw.

### Usage

#### Invest

```

#INVEST (min 100 EOS/EVD)
cleos transfer account1 eosvrmarkets "100.0000 EOS" "#INVEST#"

cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"100.0000 EVD","memo":"#INVEST#"}' -p account1

```

```

# Withdraw the invested (with bonus if any)
cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"100.0000 EVD","memo":"#WITHDRAW#"}' -p account1

```

#### Exchange

```

#EOS to EVD
cleos transfer account1 eosvrmarkets "10.0000 EOS"

#EVD to EOS
cleos push action eoslocktoken transfer '{"from":"account1", "to":"eosvrmarkets","quantity":"10.0000 EVD","memo":""}' -p account1

```

#### Get DATA

```

# Get EVD Invest Data
# NOTE: 
#   When account is empty, represent the total amount of deposit.
#   When account is eosvrmarket, represent the total amount of bonus.
cleos get table eosvrmarkets eosvrmarkets depositss1

# Get EOS Invest Data
cleos get table eosvrmarkets eosvrmarkets depositss2

```



### LIMITATION

No limitation

