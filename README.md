## Linker For SideChain

It can create a linker for EOS side-chain.

### Steps

1, Linker have accounts in both chains (EOS main-chain and EOS side-chain), Example: eoslinker111;

2, In main-chain, account eoslinker111 sent(burn) some EVD to eoslocktoken with memo equals chain_id of side-chain. 
Chain_id can get by ```cleos get info```;

```
cleos push action eoslocktoken transfer '{"from":"eoslinker111", "to":"eoslocktoken","quantity":"1000.0000 EVD","memo":"b6a3a2e75f6fc47e7ef8b413ae4ee6eb3a8fefcd01c0b0ecdf688563cfa5f493"}' -p eoslinker111
```

And side-chain BPs issue same amount of EVD to it;
  
3, Run "npm i", edit "config.js" to your settings, and run "node linker.js" to help the transfer between EOS chains.


### Simple way to setup EOS side-chain

Edit and run init.sh can setup a side-chain simply. 

#### Precondition

1, Download EOS from https://github.com/eosio/eos ;

2, Build EOS by: 

```
./eosio_build.sh -s EOS
```

3, Edit EOS config and run the EOS. Please refer to: [https://github.com/eosio/eos](https://github.com/eosio/eos)

- In config folder, there is a example config: config.ini. Copy the config.ini file to: /root/.local/share/eosio/nodeos/config (CentOS) or "~/Library/Application Support/eosio/nodeos/config" (MacOS).

- Edit config.ini and change to your settings, especial the "public key/private key" ;

- Run ```nodeos``` to start a EOS chain.


#### Setup EOS side-chain steps

1, Run "npm i";

2, Edit ```init.sh```. Change parameters in it to your EOS chain, example:

```
cu1="cleos -u http://s1.eosvr.io:8888"
pubkey=EOS7xjXnjGoFv9dFPZirn6vziaKEmZhHjEcnsPJdFqZqXoEBaf1Ej
linker=eoslinker111

cd ~/git/eos/build/contracts
```

Change them to your server, your public key for new accounts, your linker name, and your eos contracts folder.

3, Edit ```config.js```. Change parameters in it to your EOS chain, example:

```
exports.contracts = {
  creator: 'eosio',
  httpEndpoint: 'http://s1.eosvr.io:8888',
  keyProvider: ['5KidrBMWSHYAGsh37PGTGRwHKfgpQ2k6Q4qt3YuNmzicg6of1MX'],
  pubkey: EOS7xjXnjGoFv9dFPZirn6vziaKEmZhHjEcnsPJdFqZqXoEBaf1Ej
};
```

3, Import related private keys into your wallet by: ```cleos wallet import```

4, Run "cleos wallet unlock" and unlock your wallet;

5, Run "sh init.sh";

Note: Run in EOS version 1.5.1, EOS.CDT version 1.4.1.
