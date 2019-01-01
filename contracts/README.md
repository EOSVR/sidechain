## Contracts

EOS related contracts.

#### Build steps

1, Download EOS from: https://github.com/EOSIO/eos , and build it by: ```./eosio_build.sh -s EOS``` . 

2, Download EOS Contract Development Toolkit from: https://github.com/eosio/eosio.cdt , and build it. Make sure ```eosio-cpp``` can work.

3, Edit file: eb, and change it to correct folders of your environment.

4, Copy eb to a folder in path, such as ```/usr/local/bin``` (MacOS)

5, Run ```npm test``` or ```./build``` in each contract folder to build contract.

Current contracts are built by eosio-cpp with version 1.4.1 , and eos version is 1.5.1.

