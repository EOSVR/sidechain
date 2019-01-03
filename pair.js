const fs = require('fs');

const TransactionCost = 100;
const CreateAccountCost = 100;
const CancelCost = 100;
const MaxCostForUser = 300; // If the cost of a user is greater than it, will not operate it.
const retry_create_pair = 5;
const retry_fail_timeout = 3600; // After 1 hour , will retry create pair again.


exports.retry_create_pair = retry_create_pair;

const setUserCredit = function (users, linker, row, cost=0) {
  let account = row.from;
  if (row.from === linker) account = row.to;
  
  if (!users[account]) {
    users[account] = { cost: 0 };
  }
  
  let new_cost = cost + users[account].cost;
  if (new_cost < 0) new_cost = 0;
  
  users[account].cost = new_cost;
  users[account].lastupdated = +new Date();
};

exports.LoadData = function (file, linker) {
  let result;
  try {
    result = JSON.parse(fs.readFileSync(file, 'utf-8'));
  } catch (ex) {
    result = {};
  }
  
  result.linker = linker;
  
  // List of all hash lock transactions status that send to linker. (Content: mix_timeout)
  // Status:
  //    0:NA,
  //    1:Paired Transaction Created
  //    2:Current Transaction Confirmed
  if (!result.txlist) result.txlist = {};
  
  // If any steps failed, fail times will record here
  if (!result.failed) result.failed = {};
  
  // Fail retry time
  if (!result.fail_timeout) result.fail_timeout = {};
  
  // List the cost because of this user, will block user if the cost exceed a limit. Example: 0.1 EVD.
  // Create a transaction or create a user will increase the cost, confirm a transaction will reduce the cost.
  if (!result.users) result.users = {};
  
  // List the cost of one hash lock transaction sent to linker
  if (!result.costs) result.costs = {};
  
  result.getTxStatus = function (row) {
    let mix = row.mix + '_' + row.timeout;
    if (!this.txlist[mix]) return 0;
    
    return this.txlist[mix];
  };
  
  result.confirmFromData = function (row) {
    if (!this.txlist) return;
    let mix = row.mix + '_' + row.timeout;
  
    this.txlist[mix] = 2; // Change status to confirmed (2)
    
    this.failed[mix] = 0;
    
    let cost_change = 0;
    if (this.costs[mix])
        cost_change = -this.costs[mix];
    
    setUserCredit(this.users, this.linker, row, cost_change);
  };
  
  result.addIntoData = function (row) {
    let mix = row.mix + '_' + row.timeout;
  
    this.txlist[mix] = 1; // Change status to paired TX created (1)
    this.failed[mix] = 0;
  
    if (!this.costs[mix])
      this.costs[mix] = 0;
    
    this.costs[mix] += TransactionCost;
    setUserCredit(this.users, this.linker, row, TransactionCost);
  };
  
  result.addCreateUserCostIntoData = function (row) {
    let mix = row.mix + '_' + row.timeout;
    
    if (!this.costs[mix])
      this.costs[mix] = 0;
      
    this.costs[mix] += CreateAccountCost;
    setUserCredit(this.users, this.linker, row, TransactionCost);
  };

  // This user send a hash lock, but do not reveal the lock.
  // Maybe it is malicious. Should block it.
  result.cancelFromData = function (row) {
    let mix = row.mix + '_' + row.timeout;
  
    this.txlist[mix] = 3; // Status: Cancelled
  
    if (this.failed[mix])
      delete this.failed[mix];
      
    setUserCredit(this.users, this.linker, row, CancelCost);
  };
  
  result.addFailIntoData = function (row, times=1) {
    let mix = row.mix + '_' + row.timeout;
    
    if (this.failed[mix])
      this.failed[mix] += times;
    else
      this.failed[mix] = times;
    
    if (this.failed[mix] >= retry_create_pair) {
      this.fail_timeout[mix] = new Date() / 1000 + retry_fail_timeout; // Retry after 30 minutes
    }
  };
  
  // Return if need transfer in another chain
  result.canTransfer = function (row) {
    let mix = row.mix + '_' + row.timeout;
  
    // Check if pair-created / confirmed / cancelled
    if (this.txlist[mix]) return false;
  
    // Check if failed
    if (this.failed[mix] >= retry_create_pair) {
      if (this.fail_timeout && this.fail_timeout[mix] < new Date() / 1000) {
        this.failed[mix] = 0;
      } else {
        return false;
      }
    }
    
    let account = row.from;
    if (row.from === linker) account = row.to;
    
    if (this.users[account] && this.users[account].cost > MaxCostForUser) {
      return false;
    }
    
    return true;
  };
  
  result.canConfirm = function (row) {
    let mix = row.mix + '_' + row.timeout;
    
    if (row.from === this.linker) return true; // Always can confirm the transfer from linker
    
    return (this.txlist[mix] === 1); // In other case, only can confirm the transaction that created paired transaction.
  };
  
  return result;
};



exports.SaveData = function (file, data) {
  fs.writeFileSync(file,  JSON.stringify(data), 'utf-8');
};
