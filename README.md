## Linker For SideChain

It can create a linker for EOS side-chain.

### Precondition

1, Linker have accounts in both chains (EOS main-chain and EOS side-chain), Example: eoslinker111;

2, In main-chain, account eoslinker111 sent(burn) some EVD to eoslocktoken with memo equals chain_id of side-chain. 
Chain_id can get by ```cleos get info```;

```
cleos push action eoslocktoken transfer '{"from":"eoslinker111", "to":"eoslocktoken","quantity":"1000.0000 EVD","memo":"b6a3a2e75f6fc47e7ef8b413ae4ee6eb3a8fefcd01c0b0ecdf688563cfa5f493"}' -p eoslinker111
```

And side-chain BPs issue same amount of EVD to it;


### Setup steps
  
1, Run "npm i";

2, Copy "config_example.js" to "config.js" and edit "config.js" to your settings;


3, Run "node linker.js" to help the transfer between EOS chains;


[More about setup a EOS chain simply](setup_chain.md)