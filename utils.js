const binaryen = require('binaryen');
const rp = require('request-promise');
const EOS = require('eosjs');
const Long = require("long");
const fs = require('fs');

// DoTx(eos, 'eosio.token', 'transfer', 'account1', {from: 'account1',to: 'account2',quantity: '1.0000 EOS', memo: ''} )
exports.PushTransaction = function (config, contract, action, actor, data, permission='active') {
    eos = EOS({
        keyProvider: config.keyProvider,
        httpEndpoint: config.httpEndpoint,
        broadcast: true,
        sign: true
    });

  return eos.transaction(
    {
      actions: [
        {
          account: contract,
          name: action,
          authorization: [{
            actor: actor,
            permission: permission
          }],
          data: data
        }
      ]
    }
  );
  
};

exports.SetContract = function (config, account, wasm_file, abi_file) {
  let eos = EOS({
      keyProvider: config.keyProvider,
      httpEndpoint: config.httpEndpoint,
      broadcast: true,
      sign: true
  });


  let wasm = fs.readFileSync(wasm_file);
  let abi = fs.readFileSync(abi_file);

  return eos.setcode(account, 0, 0, wasm).catch(function (err) {
    console.log(err);
  }).then(function () {
    
    let obj;
    try {
      obj = JSON.parse(abi);
    } catch (ex) {
      console.log('Error in abi file: ' + abi_file);
      console.log(ex);
    }
    
    return eos.setabi(account, obj);
  });
  
};

exports.CreateAccount = function(config, from, newAccount,
     pubkey,
     ram=8192, net='10.0000 EOS', cpu='10.0000 EOS') {

  let eos = EOS({
    keyProvider: config.keyProvider,
    httpEndpoint: config.httpEndpoint,
    broadcast: true,
    sign: true
  });

  return eos.transaction(tr => {
    tr.newaccount({
      creator: from,
      name: newAccount,
      owner: pubkey,
      active: pubkey
    })
    tr.buyrambytes({
      payer: from,
      receiver: newAccount,
      bytes: ram
    })
    tr.delegatebw({
      from: from,
      receiver: newAccount,
      stake_net_quantity: net,
      stake_cpu_quantity: cpu,
      transfer: 0
    })
  })
  
};

const getNameCode = function(account) {
  if (account.length === 0) return '';
  
  return EOS.modules['format'].encodeName(account, false);
};

// GetTable('http://localhost:8888', 'account1', 'eosvrcomment', 'commentss')
exports.GetTableItem = function (site, scope, contract, table, accountId='', index_position='', key_type='i64') {
  
  let body = { "scope": scope,
              "code": contract, 
              "table": table, 
              "json": true}; 
  
  let nameCode = getNameCode(accountId);
  if (nameCode.length > 0) {
    body.lower_bound = nameCode;
    body.upper_bound = Long.fromString(nameCode, true).add(1).toString();
  }

  if (index_position.length > 0) {
    body.index_position = index_position;
    body.key_type = key_type;
  }

  let options = {
      method: 'POST',
      uri: site + '/v1/chain/get_table_rows',  
      body: body,
      json: true
  };

  return rp(options);
};

exports.nextLong = function (long) {
  return Long.fromString(long, true).add(1).toString();
};

exports.GetTableRange = function (site, scope, contract, table,
                                  lower_bound='', upper_bound='',
                                  index_position='', key_type='i64' ) {
  
  let body = { "scope": scope,
    "code": contract,
    "table": table,
    "json": true};
  
  if (lower_bound)
    body.lower_bound = lower_bound;
  
  if (upper_bound)
    body.upper_bound = upper_bound;
  
  if (index_position.length > 0) {
    body.index_position = index_position;
    body.key_type = key_type;
  }
  
  let options = {
    method: 'POST',
    uri: site + '/v1/chain/get_table_rows',
    body: body,
    json: true
  };
  
  return rp(options);
};

// GetTable('http://localhost:8888', 'account1', 'eosvrcomment', 'commentss')
exports.GetTableItems = function(site, scope, contract, table, limit=0, index_position='', key_type='i64' ) {
  
  let body = { "scope": scope,
              "code": contract, 
              "table": table, 
              "json": true}; 
  
  if (limit > 0) body.limit = limit;
  
  if (index_position.length > 0) {
      body.index_position = index_position;
      body.key_type = key_type;
  }

  let options = {
      method: 'POST',
      uri: site + '/v1/chain/get_table_rows',  
      body: body,
      json: true
  };

  return rp(options);
};

exports.GetAccount = function (site, account) {
  
  let body = {
    account_name: account
  };
  
  let options = {
    method: 'POST',
    uri: site + '/v1/chain/get_account',
    body: body,
    json: true
  };
  
  return rp(options);
};

exports.GetAccountActiveKey = function (site, account) {
  return exports.GetAccount(site, account).then(function (dat) {
    for(let one in dat.permissions) {
      let item = dat.permissions[one];
      
      if (item.required_auth.keys.length === 0) continue;
      
      if (item.perm_name === 'active')
        return item.required_auth.keys[0].key;
    }
    
    return '';
  });
};

exports.CreateAccountIfNotExist = function (config, from, account,
                                            pubkey,
                                            ram=8192, net='10.0000 EOS', cpu='10.0000 EOS') {
  
  return exports.GetAccount(config.httpEndpoint, account).catch(() => {
  
    return exports.CreateAccount(config, config.creator, account, pubkey, ram, net, cpu);
    
  });
};

exports.CreateContractAccount = function (config, account, contract_folder,
                                          pubkey,
                                          ram=800000, net='10.0000 EOS', cpu='10.0000 EOS') {
  
  return exports.CreateAccountIfNotExist (config, config.creator, account, pubkey, ram, net, cpu).then(function () {
    
    if (contract_folder[contract_folder.length-1] === '/') {
      contract_folder = contract_folder.substring(0, contract_folder.length-1);
    }
    
    let ind= contract_folder.lastIndexOf('/');
    let cname = contract_folder.substring(ind + 1);
    
    let wasm_file = contract_folder + '/' + cname + '.wasm';
    let abi_file = contract_folder + '/' + cname + '.abi';
    
    return exports.SetContract(config, account, wasm_file, abi_file );
  });
  
};