const EOS = require('eosjs');

utils = require('./utils.js');

conf = require('./config.js');

const config = conf.contracts;

utils.CreateContractAccount(config, 'eoslocktoken', './contracts/locktoken', config.pubkey).catch(console.log);  // EVD Contract
utils.CreateContractAccount(config, 'eosvrcomment', './contracts/comments', config.pubkey).catch(console.log);   // Comments
utils.CreateContractAccount(config, 'eosvrrewards', './contracts/rewards', config.pubkey).catch(console.log);    // Rewards
utils.CreateContractAccount(config, 'eosvrmarkets', './contracts/sidebancor', config.pubkey).catch(console.log); // Sidechain EOS  <-> EVD

utils.CreateContractAccount(config, 'evrportraits', './contracts/comments', config.pubkey).catch(console.log);   // Comments
