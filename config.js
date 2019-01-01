
// === Used by deploy_contracts.js , it initializes the contracts of this blockchain ===
exports.contracts = {
  creator: 'eosio',
  httpEndpoint: 'http://localhost:8888',
  keyProvider: ['5KidrBMWSHYAGsh37PGTGRwHKfgpQ2k6Q4qt3YuNmzicg6of1MX'],
  pubkey: 'EOS7xjXnjGoFv9dFPZirn6vziaKEmZhHjEcnsPJdFqZqXoEBaf1Ej'
};



// === Used linker.js , it proves the transfer across-chain ===
exports.linker = 'eoslinker111';
const linkerpks = ['5KidrBMWSHYAGsh37PGTGRwHKfgpQ2k6Q4qt3YuNmzicg6of1MX'];

exports.src = {  // Main-chain
  httpEndpoint: 'http://mainnet.eoscalgary.io',
  keyProvider: linkerpks,
  transfer_create_account_cost: 100,  //   ( The cost of create an account in side chain)
  transfer_cost: 100    // = 0.01 EVD   ( The cost of transfer from this chain to another chain)
};

exports.dest = { // Side-chain
  httpEndpoint: 'http://s1.eosvr.io:8888',
  keyProvider: linkerpks,
  transfer_create_account_cost: 10000000, // Cost of create an account in mainnet is high (example: 1000EVD)
  transfer_cost: 100     // = 0.01 EVD
};

