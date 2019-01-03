const { Api, JsonRpc, RpcError, Serialize } = require('eosjs');
const JsSignatureProvider = require('eosjs/dist/eosjs-jssig');
const fetch = require('node-fetch');                            // node only; not needed in browsers
const { TextEncoder, TextDecoder } = require('util');           // node only; native TextEncoder/Decoder


const rp = require('request-promise');
const Long = require("long");
const fs = require('fs');

// DoTx(eos, 'eosio.token', 'transfer', 'account1', {from: 'account1',to: 'account2',quantity: '1.0000 EOS', memo: ''} )
exports.PushTransaction = function (config, contract, action, actor, data, permission='active') {
    const rpc = new JsonRpc(config.httpEndpoint, {fetch});
    const signatureProvider = new JsSignatureProvider.default(config.keyProvider);
    const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

    return (async () => {
      const result = await api.transact({
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
      }, {
        blocksBehind: 3,
        expireSeconds: 30,
      });
      return result;
    })();
};

exports.SetContract = function (config, account, wasm_file, abi_file) {
  const rpc = new JsonRpc(config.httpEndpoint, {fetch});
  const signatureProvider = new JsSignatureProvider.default(config.keyProvider);
  const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
  
  console.log('Set Contract');
  
  return (async () => {
    let wasm = fs.readFileSync(wasm_file);
    let abi = fs.readFileSync(abi_file);
    //wasm = wasm.replace(/(\r\n\t|\n|\r\t| )/gm, "");

    //get abi hex
    const buffer = new Serialize.SerialBuffer({
      textEncoder: api.textEncoder,
      textDecoder: api.textDecoder,
    });
    
    abi = JSON.parse(abi);
    let abiDefinition = api.abiTypes.get('abi_def');
    abi = abiDefinition.fields.reduce(
      (acc, {name: fieldName}) => Object.assign(acc, {[fieldName]: acc[fieldName] || []}),
      abi,
    );
    abiDefinition.serialize(buffer, abi);
    let abi_buf = Buffer.from(buffer.asUint8Array()).toString(`hex`);
    
    const result = await api.transact({
      actions: [{
        account: 'eosio',
        name: 'setcode',
        authorization: [{
          actor: account,
          permission: 'active',
        }],
        data: {
          account: account,
          vmtype: 0,
          vmversion: 0,
          code: wasm
        },
      },
        {
          account: 'eosio',
          name: 'setabi',
          authorization: [{
            actor: account,
            permission: 'active',
          }],
          data: {
            account: account,
            abi: abi_buf
          },
        }
      
      ]
    }, {
      blocksBehind: 3,
      expireSeconds: 30,
    });
  
    return result;
  })();
  
};

exports.CreateAccount = function(config, from, newAccount,
     pubkey,
     ram=8192, net='10.0000 EOS', cpu='10.0000 EOS') {
  
  const rpc = new JsonRpc(config.httpEndpoint, {fetch});
  const signatureProvider = new JsSignatureProvider.default(config.keyProvider);
  const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
  
  console.log('Create account');
  
  return (async () => {
    const result = await api.transact({
      actions: [
        {
          account: 'eosio',
          name: 'newaccount',
          authorization: [{
            actor: from,
            permission: 'active'
          }],
          data: {
            creator: from,
            name: newAccount,
            owner: {
              threshold: 1,
              keys: [{ key: pubkey, weight: 1 }],
              accounts: [],
              waits:[]
            },
            active: {
              threshold: 1,
              keys: [{ key: pubkey, weight: 1 }],
              accounts: [],
              waits:[]
            }
          }
        },
  
        {
          account: 'eosio',
          name: 'delegatebw',
          authorization: [{
            actor: from,
            permission: 'active'
          }],
          data: {
            from: from,
            receiver: newAccount,
            stake_net_quantity: net,
            stake_cpu_quantity: cpu,
            transfer: false
          }
        },
  
        {
          account: 'eosio',
          name: 'buyrambytes',
          authorization: [{
            actor: from,
            permission: 'active'
          }],
          data: {
            payer: from,
            receiver: newAccount,
            bytes: ram
          }
        }
      ]
    }, {
      blocksBehind: 3,
      expireSeconds: 30,
    });
    return result;
  })();
  
};

/*
const getNameCode = function(account) {
  if (account.length === 0) return '';
  
  return EOS.modules['format'].encodeName(account, false);
};
*/

// GetTable('http://localhost:8888', 'account1', 'eosvrcomment', 'commentss')
exports.GetTableItem = function (site, scope, contract, table, accountId='', index_position='', key_type='i64') {
  
  let body = { "scope": scope,
              "code": contract, 
              "table": table, 
              "json": true}; 
  
  /*let nameCode = getNameCode(accountId);
  if (nameCode.length > 0) {
    body.lower_bound = nameCode;
    body.upper_bound = Long.fromString(nameCode, true).add(1).toString();
  }
  */
  if (accountId) {
    body.lower_bound = accountId;
    body.limit = 1;
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

exports.CreateContractAccount = function (config, account, contract_folder, set_eosio_code,
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
  
    if (set_eosio_code) {
      return exports.UpdateAuth2(config, account, false, [pubkey], ['eosio', account], ['active', 'eosio.code']).then(() => {
        return exports.SetContract(config, account, wasm_file, abi_file);
      });
    } else {
      return exports.SetContract(config, account, wasm_file, abi_file);
    }
  });
  
};

// auth = { threshold: 1, keys: [{ key: pubkey, weight: 1 }], accounts: [], waits:[] }
exports.UpdateAuth = function (config, account, is_owner, auth) {
  const rpc = new JsonRpc(config.httpEndpoint, {fetch});
  const signatureProvider = new JsSignatureProvider.default(config.keyProvider);
  const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });
  
  
  return (async () => {
    const result = await api.transact({
      actions: [{
        account: 'eosio',
        name: 'updateauth',
        authorization: [{
          actor: account,
          permission: 'owner',
        }],
        data: {
          account: account,
          permission: (is_owner ? 'owner': 'active'),
          parent: 'owner',
          auth: auth
        },
      }]
    }, {
      blocksBehind: 3,
      expireSeconds: 30,
    });
    return result;
  })();
  
};

exports.UpdateAuth2 = function (config, account, is_owner, keys, accounts=[], accounts_permissions=[]) {
  let auth = {
    threshold: 1,
    keys: [],
    accounts: [],
    waits: []
  };
  
  for(let one in keys) {
    let key = keys[one];
    if (typeof key === 'string' && key.length > 50) {
      auth.keys.push({key: key, weight: 1});
    }
  }
  
  for(let one in accounts) {
    let account = accounts[one];
    
    if (typeof account === 'string' && account.length > 0 && account.length <= 12) {
      let perm = 'active';
      if (accounts_permissions[one]) {
        perm = accounts_permissions[one];
      }
      
      auth.accounts.push({ permission: { actor: account, permission: perm }, weight: 1});
    }
  }
  
  return exports.UpdateAuth(config, account, is_owner, auth);
};
