const fs = require('fs');
const Q = require('bluebird');
const utils = require('./utils.js');

const conf = require('./config.js');

const linker = conf.linker;
const src = conf.src;
const dest = conf.dest;


const retry_create_pair = 3;
const retry_fail_timeout = 3600; // After 1 hour , will retry create pair again.
const time_buffer = 1800; // 0.5 hour buffer for one operation (transaction)
const check_interval = 15000; // Check status every 15 seconds


let hashkeys = { '2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824': 'hello' };
let src_data = {};
let dest_data = {};



const isTimeout = function (timeout) {
  return +(new Date()) > (+timeout) / 1000;
};

const GetChainTransferParameters = function (row, src, need_create_account=false) {
  let cost = src.transfer_cost;
  if (need_create_account)
    cost += src.transfer_create_account_cost;
  
  if (row.quantity - cost <= 0)
    return { reason: 'No enough cost, must > ' + (cost/10000).toFixed(4) + ' EVD' };
  
  let remain_seconds = ((+row.timeout) / 1000 - new Date()) / 1000 - time_buffer;
  if (remain_seconds < time_buffer)
    return { reason: 'Timeout is too short' };
  
  let seconds = Math.floor(remain_seconds);
  let memo = "#HASH#" + row.hash + "," + seconds;
  
  let to = row.memo;
  let ind = to.indexOf('@');
  if (ind > 0) {
    to = to.substr(0, ind);
    memo = memo + "," + to.substr(ind + 1);
  }
  
  if (need_create_account && to.length !== 12) {
    return { reason: 'New account name: ' + to + ' is invalid. Length must be 12'};
  }
  
  let quantity = ((row.quantity - cost) / 10000).toFixed(4) + ' EVD';
  
  return {
    to: to,
    quantity: quantity,
    memo: memo
  };
};

const getHashKey = function (hash) {
  if (!hash) return '';
  return hashkeys[hash];
};

const CrossChainGetHashKey = function (src) {
  return utils.GetTableItem(src.httpEndpoint, 'eoslocktoken', 'eoslocktoken', 'hashss').then(function (dat) {
    for(let one in dat.rows) {
      let onerow = dat.rows[one];
      
      if (onerow.hash && onerow.key)
        hashkeys[onerow.hash] = onerow.key;
    }
  });
};

const havePairs = function (data, row) {
  let mix = row.mix + '_' + row.timeout;
  let ind = data.pairs.indexOf(mix);
  return (ind >= 0);
};

const confirmFromData = function (data, row) {
  if (!data.pairs) return;
  let mix = row.mix + '_' + row.timeout;
  
  let ind = data.pairs.indexOf(mix);
  if (ind >= 0) {
    data.pairs.splice(ind, 1);
  
    data.finish_pairs.push(ind);
  
    if (data.failed && data.failed[mix])
      delete data.failed[mix];
  
    let cost_change = 0;
    if (data.costs[mix])
      cost_change = -data.costs[mix];
  
    setUserCredit(data.users, row, 0, 1, 0, cost_change);
  }
};

// This user send a hash lock, but do not reveal the lock.
// Maybe it is malicious. Should block it.
const cancelFromData = function (data, row) {
  if (!data.pairs) return;
  
  let mix = row.mix + '_' + row.timeout;
  
  let ind = data.pairs.indexOf(mix);
  if (ind >= 0) {
    data.pairs.splice(ind, 1);
    
    data.finish_pairs.push(ind);
    
    if (data.failed && data.failed[mix])
      delete data.failed[mix];
    
    setUserCredit(data.users, row, 0, 0, 1);
  }
};

const setUserCredit = function (users, row, start, finished, cancelled, cost=0) {
  let account = row.from;
  if (row.from === linker) account = row.to;
  
  if (!users[account]) {
    users[account] = { started: 0, finished:0, cancelled: 0, cost: 0 };
  }
  
  users[account].started += start;
  users[account].finished += finished;
  users[account].cancelled += cancelled;
  users[account].cost += cost;
  users[account].lastupdated = +new Date();
};

const addIntoData = function (data, row) {
  let mix = row.mix + '_' + row.timeout;
  
  let ind = data.pairs.indexOf(mix);
  if (ind < 0) {
    data.pairs.push(mix);
    data.failed[mix] = 0;
    
    setUserCredit(data.users, row, 1, 0, 0);
  }
};

const addCostIntoData = function (data, row) {
  let mix = row.mix + '_' + row.timeout;
  
  let ind = data.pairs.indexOf(mix);
  if (ind < 0) {
    data.pairs.push(mix);
    
    data.costs[mix] = 1;
    
    setUserCredit(data.users, row, 0, 0, 0, 1);
  }
};

const addFailIntoData = function (data, row, times=1) {
  let mix = row.mix + '_' + row.timeout;
  
  if (data.failed[mix])
    data.failed[mix] += times;
  else
    data.failed[mix] = times;
  
  if (data.failed[mix] >= retry_create_pair) {
    data.fail_timeout[mix] = new Date() / 1000 + retry_fail_timeout; // Retry after 30 minutes
  }
};

const canTransfer = function (data, row) {
  let account = row.from;
  if (row.from === linker) account = row.to;
  
  if (data.users[account]) {
    if (data.users[account].started > data.users[account].finished + data.users[account].cancelled) {
      return false;
    }

    // If create an account for it, but it do not confirm the hash lock. Reject it.
    if (data.users[account].cost > 0) {
      return false;
    }
  }
  
  return true;
};

const isTransfered = function (data, row) {
  if (!data.pairs) return false;
  
  let mix = row.mix + '_' + row.timeout;
  
  return data.pairs.indexOf(mix) >= 0 || data.finish_pairs.indexOf(mix) >= 0;
};

const isFailed = function (data, row) {
  if (!data.failed) return false;
  
  let mix = row.mix + '_' + row.timeout;
  
  if (data.failed[mix] >= retry_create_pair) {
    if (data.fail_timeout && data.fail_timeout[mix] < new Date() / 1000) {
      data.failed[mix] = 0;
      return false; // Retry again
    }
    
    return true;
  }
  
  return false;
};

const doTransferTo = function (dest, linker, data) {
  return utils.PushTransaction(dest, 'eoslocktoken', 'transfer', linker, data).then(function () {
    console.log('Linked transfer: ' + data.from + "->" + data.to + ' created.');
  });
};

// When fail, transfer back with reason
const doTransferBack = function (src, row, reason) {
  if (row.to !== linker) throw new Error('Invalid transfer back!');
  
  let quantity = (row.quantity / 10000).toFixed(4) + ' EVD';
  
  let remain_seconds = ((+row.timeout) / 1000 - new Date()) / 1000 - time_buffer;
  if (remain_seconds < time_buffer)
    return; // Do not transfer back if timeout is too short.
  
  let seconds = Math.floor(remain_seconds);
  let memo = "#HASH#" + row.hash + "," + seconds + ',' + reason;
  
  let data = { from: row.to, to: row.from, quantity: quantity, memo: memo };
  
  // Can not transfer back because it is same Index in contract. (from^to == to^from)
  //return utils.PushTransaction(src, 'eoslocktoken', 'transfer', linker, data).then(function () {
  console.log('Transfer ' + row.from + '->' + row.to + ' can not link because: ' + reason);
  //});
};

const createAccountFromSrc = function (src, from, dest, to, linker) {
  return utils.GetAccountActiveKey(src.httpEndpoint, from).then((publickey) => {
    return utils.CreateAccount(dest, linker, to, publickey);
  });
};


const createTransferTo = function (onerow, src, dest, linker, data1) {
  // Transfer in dest chain
  let params = GetChainTransferParameters(onerow, src);
  let data = {from: linker, to: params.to, quantity: params.quantity, memo: params.memo};
  
  if (!params || !params.to) {
    addFailIntoData(data1, onerow, retry_create_pair);
    return doTransferBack(src, onerow, params.reason);
  }
  
  return utils.GetAccount(dest.httpEndpoint, params.to).then((dat) => {
    return (dat && dat.account_name);
  }).catch((err) => {
    return false;
  }).then((hasAccount) => {
    if (hasAccount) {

      return doTransferTo(dest, linker, data);
    } else {
      let p1 = GetChainTransferParameters(onerow, src, true);
  
      if (!p1.quantity) {
        addFailIntoData(data1, onerow, retry_create_pair);
        return doTransferBack(src, onerow, p1.reason);
      } else {
        return createAccountFromSrc(src, onerow.from, dest, params.to, linker).catch(() => {
          console.log('Create account ' + params.to + ' failed.');
          throw new Error('Fail to create account');
        }).then(() => {
          addCostIntoData(data1, onerow);
          
          data.quantity = p1.quantity;
          console.log('Account ' + params.to + ' created.');
    
          return doTransferTo(dest, linker, data);
        });
      }
    }
  }).then(() => {
    addIntoData(data1, onerow);
  }).catch((err) => {
    addFailIntoData(data1, onerow);
  })
};

const CrossChainTransfer = function (src, dest, linker, data1) {
  return utils.GetTableItem(src.httpEndpoint, 'eoslocktoken', 'eoslocktoken', 'hashlockss', linker, '3').then(function (dat) {
    // Get all transactions send to linker with hash lock
    if (dat) {
      let list = [];
      
      for(let i in dat.rows) {
        let onerow = dat.rows[i];
  
        if (!isTransfered(data1, onerow) && !isFailed(data1, onerow) && canTransfer(data1, onerow)) {
          list.push(createTransferTo (onerow, src, dest, linker, data1));
        }
      }
      
      return Q.all(list);
    }
  });
};



const CrossChainConfirm = function (src, dest, linker, data1, index='2') {
    return utils.GetTableItem(dest.httpEndpoint, 'eoslocktoken', 'eoslocktoken', 'hashlockss', linker, index).then(function (dat) {
        if (dat && dat.rows) {
          
          let list = [];
          for(let i in dat.rows) {
            let onerow = dat.rows[i];
            if (onerow == null) continue;
            
            if (isTimeout(onerow.timeout)) {
              let data = {from: onerow.from, to: onerow.to, key: "", executer: linker};
    
              list.push(utils.PushTransaction(dest, 'eoslocktoken', 'confirm', linker, data).then(function () {
                cancelFromData(data1, onerow);
                console.log('Confirmed: ' + data);
              }));
            } else if (onerow.hash) {
              
              let key = getHashKey(onerow.hash);
              if (key && havePairs(data1, onerow)) {
                // Confirm in current chain
                let data = {from: onerow.from, to: onerow.to, key: key, executer: linker};
    
                list.push(utils.PushTransaction(dest, 'eoslocktoken', 'confirm', linker, data).then((d) => {
                  confirmFromData(data1, onerow);
                  console.log('Confirmed: ' + onerow.from + "->" + onerow.to);
                }).catch((err) => {
                  console.log('Confirm error:' + onerow.from + "->" + onerow.to + '\n' + err);
                }));
    
              }
            }
          }
          
          return Q.all(list);
        }
    });
};

const LoadData = function (file) {
  let result;
  try {
    result = JSON.parse(fs.readFileSync(file, 'utf-8'));
  } catch (ex) {
    result = {};
  }
  
  if (!result.pairs) result.pairs = [];
  if (!result.finish_pairs) result.finish_pairs = [];
  if (!result.failed) result.failed = {};
  if (!result.fail_timeout) result.fail_timeout = {};
  if (!result.users) result.users = {};
  if (!result.costs) result.costs = {};   // Like black list. List the amount of users created for certain user, but it do not confirm!

  return result;
};

const SaveData = function (file, data) {
  fs.writeFileSync(file,  JSON.stringify(data), 'utf-8');
};

// Do cross chain operation
const DoCrossChain = function () {
    return CrossChainTransfer(src, dest, linker, src_data)
    .then(() => {
      return CrossChainTransfer(dest, src, linker, dest_data);
    }).then(() => {
      return CrossChainGetHashKey(src);
    }).then(() => {
      return CrossChainGetHashKey(dest);
    }).then(() => {
      return CrossChainConfirm(src, dest, linker, dest_data, '2');
    }).then(() => {
      return CrossChainConfirm(src, dest, linker, dest_data, '3');
    }).then(() => {
      return CrossChainConfirm(dest, src, linker, src_data, '2');
    }).then(() => {
      return CrossChainConfirm(dest, src, linker, src_data, '3');
    }).catch((err) => {
      if (err.name === 'RequestError') {
        console.error(err.message);
      } else {
        console.error(err);
      }
    }).finally(() => {
        SaveData('src.data', src_data);
        SaveData('dest.data', dest_data);
        setTimeout(DoCrossChain, check_interval); // Run it again
    });
};

src_data = LoadData('src.data');
dest_data = LoadData('dest.data');


DoCrossChain();
