## Simple way to setup EOS side-chain

Edit and run init.sh can setup a side-chain simply.

### Precondition

1, Download EOS from https://github.com/eosio/eos ;

2, Build EOS by:

```

# Build with token "EOS"
./eosio_build.sh -s EOS

# Install it
sudo ./eosio_install.sh

# Try by checking version
nodeos --version

```

3, Edit EOS config and run the EOS. Please refer to: [https://github.com/eosio/eos](https://github.com/eosio/eos)

- In config folder, there is a example config: config.ini. Copy the config.ini file to: /root/.local/share/eosio/nodeos/config (CentOS) or "~/Library/Application Support/eosio/nodeos/config" (MacOS).

There is a genesis.json in config folder, it is the genesis.json of test chain: http://s1.eosvr.io:8888/ .

- Edit config.ini and change to your settings. Such as the following publickey/privatekey:

```

signature-provider=EOS7xjXnjGoFv9dFPZirn6vziaKEmZhHjEcnsPJdFqZqXoEBaf1Ej=KEY:5KidrBMWSHYAGsh37PGTGRwHKfgpQ2k6Q4qt3YuNmzicg6of1MX

```


- Run ```nodeos``` to start a EOS chain.


### Setup EOS side-chain steps

1, Run "npm i";

2, Edit ```init.sh```. Change parameters in it to your EOS chain, example:

```
cu1="cleos -u http://s1.eosvr.io:8888"
pubkey=EOS7xjXnjGoFv9dFPZirn6vziaKEmZhHjEcnsPJdFqZqXoEBaf1Ej
linker=eoslinker111

cd ~/git/eos/build/contracts
```

Change them to your EOS server, your public key for new accounts, your linker name, and your eos contracts folder.

3, Copy config_example.js to config.js and edit ```config.js```.
Change parameters in it to your EOS chain, example:

```
exports.contracts = {
  creator: 'eosio',
  httpEndpoint: 'http://s1.eosvr.io:8888',
  keyProvider: ['5KidrBMWSHYAGsh37PGTGRwHKfgpQ2k6Q4qt3YuNmzicg6of1MX'],
  pubkey: EOS7xjXnjGoFv9dFPZirn6vziaKEmZhHjEcnsPJdFqZqXoEBaf1Ej
};
```

3, Run "cleos wallet unlock" and unlock your wallet;

4, Import related private keys into your wallet by: ```cleos wallet import```;

5, Run "sh init.sh".

Note: Run in EOS version 1.5.1, EOS.CDT version 1.4.1.
