const fs = require('fs');
const Q = require('bluebird');
const utils = require('./utils.js');

const conf = require('./config.js');

const pair = require('./pair.js');

const linker = conf.linker;
const src = conf.src;
const dest = conf.dest;


const time_buffer = 1800; // 0.5 hour buffer for one operation (transaction)
const check_interval = 15000; // Check status every 15 seconds
let exit_times = 240; // Exit every 1 hour
const force_exit_interval = 4800000; // Force exit after 1 hour 20 minutes.

const retry_create_pair = pair.retry_create_pair;


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


const doTransferTo = function (dest, linker, data) {
  return utils.PushTransaction(dest, 'eoslocktoken', 'transfer', linker, data).then(function () {
    console.log(new Date() + ':' + dest.httpEndpoint + ' linked transfer: ' + data.from + "->" + data.to + ' with ' + data.quantity + ' created.');
  });
};

// When fail, transfer back with reason
const doTransferBack = function (src, row, reason) {
  if (row.to !== linker) {
    console.dir(row);
    throw new Error('Invalid transfer back!');
  }
  console.log(new Date() + ':' + src.httpEndpoint + ' transfer ' + row.from + '->' + row.to + ' can not link because: ' + reason);
  return;   // Can not transfer back because it is same Index in contract. (from^to == to^from)
  
  let quantity = (row.quantity / 10000).toFixed(4) + ' EVD';
  
  let remain_seconds = ((+row.timeout) / 1000 - new Date()) / 1000 - time_buffer;
  if (remain_seconds < time_buffer)
    return; // Do not transfer back if timeout is too short.
  
  let seconds = Math.floor(remain_seconds);
  let memo = "#HASH#" + row.hash + "," + seconds + ',' + reason;
  
  let data = { from: row.to, to: row.from, quantity: quantity, memo: memo };
  
  return utils.PushTransaction(src, 'eoslocktoken', 'transfer', linker, data).then(function () {
   console.log(new Date() + ':' + src.httpEndpoint + ' transfer ' + row.from + '->' + row.to + ' can not link because: ' + reason);
  });
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
    data1.addFailIntoData(onerow, retry_create_pair);
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
        data1.addFailIntoData(onerow, retry_create_pair);
        return doTransferBack(src, onerow, p1.reason);
      } else {
        return createAccountFromSrc(src, onerow.from, dest, params.to, linker).catch(() => {
          throw new Error(dest.httpEndpoint + ' fail to create account' + params.to);
        }).then(() => {
          data1.addCreateUserCostIntoData(onerow);
          
          data.quantity = p1.quantity;
          console.log(dest.httpEndpoint + ' account ' + params.to + ' created.');
    
          return doTransferTo(dest, linker, data);
        });
      }
    }
  }).then(() => {
    data1.addIntoData(onerow);
  }).catch((err) => {
    console.log(err);
    data1.addFailIntoData(onerow);
  })
};

const CrossChainTransfer = function (src, dest, linker, data1) {
  return utils.GetTableItem(src.httpEndpoint, 'eoslocktoken', 'eoslocktoken', 'hashlockss', linker, '3').then(function (dat) {
    // Get all transactions send to linker with hash lock
    if (dat) {
      let list = [];
      
      for(let i in dat.rows) {
        let onerow = dat.rows[i];
        if (onerow.to !== linker) continue;
        
        if (data1.canTransfer(onerow)) {
          list.push(createTransferTo(onerow, src, dest, linker, data1));
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
            if (onerow.to !== linker && onerow.from !== linker) continue;
    
              if (isTimeout(onerow.timeout)) {
              let data = {from: onerow.from, to: onerow.to, key: "", executer: linker};
    
              list.push(utils.PushTransaction(dest, 'eoslocktoken', 'confirm', linker, data).then(function () {
                data1.cancelFromData(onerow);
                console.log(new Date() + ':' + dest.httpEndpoint + ' confirmed: ' + data);
              }));
            } else if (onerow.hash) {
              
              let key = getHashKey(onerow.hash);
              if (key && data1.canConfirm(onerow)) {
                // Confirm in current chain
                let data = {from: onerow.from, to: onerow.to, key: key, executer: linker};
    
                list.push(utils.PushTransaction(dest, 'eoslocktoken', 'confirm', linker, data).then((d) => {
                  data1.confirmFromData(onerow);
                  console.log(dest.httpEndpoint + ' confirmed: ' + onerow.from + "->" + onerow.to);
                }).catch((err) => {
                  console.log(dest.httpEndpoint + ' confirm error:' + onerow.from + "->" + onerow.to + '\n' + err);
                }));
    
              }
            }
          }
          
          return Q.all(list);
        }
    });
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
        pair.SaveData('src.data', src_data);
        pair.SaveData('dest.data', dest_data);

	exit_times--;
	if (exit_times <= 0) {
		console.log("Check 1 hours and exit");
        	process.exit();
	} else
        	setTimeout(DoCrossChain, check_interval); // Run it again
    });
};

setTimeout(()=> {
	console.log("Force exit after 80 minutes!!!");
	process.exit();
}, force_exit_interval);

src_data = pair.LoadData('src.data', linker);
dest_data = pair.LoadData('dest.data', linker);

DoCrossChain();

console.log('Linker is working... Try the following in main chain: (hash key is "hello")');
console.log('cleos push action eoslocktoken transfer  \'{"from":"guest1111113", "to":"'
  + linker
  + '","quantity":"0.1000 EVD","memo":"#HASH#2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824,86400,guest1111113"}\' -p guest1111113');

console.log('-----');
console.log('And confirm command like:');
console.log('cleos push action eoslocktoken confirm  \'{"from":"'
  + linker + '", "to":"guest1111113","key":"hello", "executer":"guest1111113"}\' -p guest1111113');

console.log('-----');