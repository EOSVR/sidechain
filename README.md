## Setup of SideChain

[中文](README-cn.md)

[Introduction](https://github.com/EOSVR/EOSVR/blob/master/sidechain.md)

### Precondition

To create a sidechain, need the following precondition:

- Setup a EOS chain with contracts. [Details](setup_chain.md)

- Linker have accounts in both chains (EOS main-chain and EOS side-chain), Example: eoslinker111.
It is better to use the same public key in active permission of linker account in both EOS-chains. Can change it like:

    ```
    cleos set account permission eoslinker111 active '{"threshold": 1,"keys": [{"key": "EOS6noQsmvdoddJicNsTQD6sMhKxT5JPvFh7J61Na31oigR94ZWS2","weight": 1}],"accounts": []}' owner -p eoslinker111
    ```

- In main-chain, account eoslinker111 sent(burn) some EVD to eoslocktoken with memo equals chain_id of side-chain.
Chain_id can get by ```cleos get info```;

    ```
    cleos push action eoslocktoken transfer '{"from":"eoslinker111", "to":"eoslocktoken","quantity":"1000.0000 EVD","memo":"b6a3a2e75f6fc47e7ef8b413ae4ee6eb3a8fefcd01c0b0ecdf688563cfa5f493"}' -p eoslinker111

    # Check status
    cleos get table eoslocktoken eoslocktoken depositss
    ```

And side-chain owner (BPs) issue same amount of EVD to it;

When others want to know if sidechain is trustworthy, they can simply run the following in both chains to do a simple check:

    ```
    cleos get table eoslocktoken eoslocktoken depositss
    ```

Note: Linker account can NOT be a string with only numbers, example: 111111111111.

### Setup steps
  
1, Run "npm i";


2, Copy "config_example.js" to "config.js" and edit "config.js" to your settings;


3, Run "node linker.js" to help the transfer between EOS chains;


Note: linker must have enough CPU and RAM to operate. And must have enough EVD in both sides.


### Test Chain

There is a test EOS-Chain that can try: http://s1.eosvr.io:8888/

Linker of it is : eoslinker111

Website to view the accounts in all EOS chains: http://id.eosvr.io/

And the source code of the website is in html folder.



### Contact

Twitter: https://twitter.com/EVD89490917

Medium: https://medium.com/@eosvr

Mail: contact@eosvr.io
