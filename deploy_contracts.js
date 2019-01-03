// Deploy 5 contracts that are needed in EOS side chain

utils = require('./utils.js');

conf = require('./config.js');

const config = conf.contracts;

utils.CreateContractAccount(config, 'eoslocktoken', './contracts/locktoken', true, config.pubkey).catch(console.log);  // EVD Token Contract

utils.CreateContractAccount(config, 'eosvrmarkets', './contracts/sidebancor', true, config.pubkey).catch(console.log); // Exchange for "EOS in Side Chain" <-> EVD

utils.CreateContractAccount(config, 'eosvrcomment', './contracts/comments', true, config.pubkey).catch(console.log);   // Comment Contract (Account's Description)

utils.CreateContractAccount(config, 'evrportraits', './contracts/comments', true, config.pubkey).catch(console.log);   // Comment Contract (Account's Portrait)

utils.CreateContractAccount(config, 'eosvrrewards', './contracts/rewards', true, config.pubkey).catch(console.log);    // Rewards Contract (Optional)
