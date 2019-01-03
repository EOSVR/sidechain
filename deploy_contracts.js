utils = require('./utils.js');

conf = require('./config.js');

const config = conf.contracts;

utils.CreateContractAccount(config, 'eoslocktoken', './contracts/locktoken', true, config.pubkey).catch(console.log);  // EVD Contract

utils.CreateContractAccount(config, 'eosvrcomment', './contracts/comments', true, config.pubkey).catch(console.log);   // Comments

utils.CreateContractAccount(config, 'eosvrrewards', './contracts/rewards', true, config.pubkey).catch(console.log);    // Rewards

utils.CreateContractAccount(config, 'eosvrmarkets', './contracts/sidebancor', true, config.pubkey).catch(console.log); // Sidechain EOS  <-> EVD

utils.CreateContractAccount(config, 'evrportraits', './contracts/comments', true, config.pubkey).catch(console.log);   // Comments

