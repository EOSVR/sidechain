// === EOS SERVER ===
// 当前用户名, 连接者名称，指向者名称，评论合约名称
var current_id = '', current_linker = '', current_to = '', current_commenter = '';

var current_locked_token = 0, current_evd_token = 0;
var current_limit = 0;
var current_is_linker = false;

const COMMENT_HASH = "359adaabf4ee648e8234e7ff6a8fb160b1b8d9ffd4f40da400599cbc002e7a0b";

var getTable = function (server1, code1, scope1, table_name, lower_bound, limit1, callback) {
  let url = server1 + '/v1/chain/get_table_rows';
  
  let body = JSON.stringify({
    "json": true, "code": code1, "scope": scope1, "table": table_name,
    "lower_bound": lower_bound, "limit": limit1
  });
  
  $.ajax({
    "type": "POST",
    "url": url,
    "data": body
  }).done(function (data) {
    console.log(data);
    callback(data);
  }).fail(function () {
    callback(null);
  });
};

// indexNo = "2"
var getTable2 = function (server1, code1, scope1, table_name, index_position, limit1, callback) {
  let url = server1 + '/v1/chain/get_table_rows';
  
  let body = JSON.stringify({
    json: true, "code": code1, "scope": scope1, "table": table_name,
    key_type: "i64",
    index_position: index_position,
    "limit": limit1
  });
  
  $.ajax({
    "type": "POST",
    "url": url,
    "data": body
  }).done(function (data) {
    console.log(data);
    callback(data);
  }).fail(function () {
    callback(null);
  });
};

// Get side chain server
var getSideChainServer = function (server, linker, callback) {
  // cleos push action eosvrcomment comment '{"from":"eoslinker111", "to":"eoslinker111", "memo":"hello, good."}' -p eoslinker111
  // cleos get table eosvrcomment eoslinker111 commentss
  
  if (linker) {
    getTable(server, "eosvrcomment", linker, "commentss", linker, 1, (data) => {
      if (data && data.rows && data.rows.length > 0) {
        // "memo": "chain:http://s1.eosvr.io:8888/\nname:Test chain",
        var memo = data.rows[0].memo;
        
        var lines = memo.split('\n');
        
        for (var one in lines) {
          if (typeof lines[one] !== 'string') continue;
          if (lines[one].indexOf('chain:') === 0) {
            var side_server = lines[one].substring(6);
            if (side_server[side_server.length - 1] === '/') {
              side_server = side_server.substring(0, side_server.length - 1);
              
              console.log("Find server: " + side_server);
              callback(side_server);
              return;
            }
          }
        }
      }
      
      callback();
    });
  } else {
    callback(server);
  }
};

var get_token = function () {
  var token = current_evd_token + current_locked_token;
  return get_token2(token);
};

var getLimitStr = function (user_limit_strs) {
  if (current_evd_token + current_locked_token < 0) return user_limit_strs[0];
  
  if (current_limit <= 0 || current_limit > 50) return user_limit_strs[1];
  
  if (current_limit > 25) return user_limit_strs[2];
  
  if (current_limit > 10) return user_limit_strs[3];
  
  if (current_limit > 1) return user_limit_strs[4];
  
  return user_limit_strs[5];
};

var getLimitMulti = function () {
  if (current_limit <= 0 || current_limit > 50)
    return 1;
  
  var m = 10;
  if (current_limit > 10)
    m = 5;

  return m * 100 / current_limit;
};

var get_token2 = function (token) {
  var abstoken = Math.abs(token);
  
  if (abstoken === 0) {
    return '0 EVD';
  } else if (abstoken < 100) {
    return (token / 10000).toFixed(4) + ' EVD';
  } else if (abstoken < 10000) {
    return (token / 10000).toFixed(2) + ' EVD';
  } else if (abstoken < 10000000) {
    return (token / 10000).toFixed(0) + ' EVD';
  } else if (abstoken < 10000000000) {
    return (token / 10000000).toFixed(0) + 'K EVD';
  } else {
    return (token / 10000000000).toFixed(0) + 'M EVD';
  }
};

var fixserver = function (server) {
  if (window.location.href.indexOf("https://") === 0) {
    return server.replace('http://', 'https://').replace(":8888", ":8889");
  }
  
  return server;
}

var REGX_HTML_ENCODE = /"|<|>|[\x00-\x19]|[\x7F-\xFF]/g;
var encodeHtml = function(s){
  return (typeof s != "string") ? s :
    s.replace(REGX_HTML_ENCODE,
      function($0) {
        var c = $0.charCodeAt(0), r = ["&#"];
        c = (c == 0x20) ? 0xA0 : c;
        r.push(c);
        r.push(";");
        return r.join("");
      });
};


// Parse memo to html mode.
var parseMemo = function (memo) {
  var lines = memo.split('\n');
  var result = '';
  
  for (var one in lines) {
    if (lines.hasOwnProperty(one)) {
      var oneline = encodeHtml(lines[one]);
      if (oneline === '---') {
        result += '<hr />';
        continue;
      } else if (oneline.startsWith('http')) {
        oneline = '<a href="' + oneline + '">' + oneline + '</a>'
      } else if (oneline.startsWith('@')) {
        var ind = oneline.indexOf(' ');
        if (ind < 0) ind = oneline.length;
  
        if (ind > 1) {
          var account = oneline.substring(1, ind);
          if (account.indexOf('+') > 0 || account.indexOf('-') > 0) {
            oneline = '<a href="?id=' + account + '">' + oneline.substring(ind) + '</a>';
          } else {
            oneline = '<a href="?id=' + account + '">' + account + '</a>' + oneline.substring(ind);
          }
        }
      }
      
      result += oneline + '<br />';
    }
  }
  
  return result;
  
};

var getCode = function (server1, account, callback) {
  let url = server1 + "/v1/chain/get_code_hash";
  
  let body = JSON.stringify({
    "json": true,
    "account_name": account
  });
  
  $.ajax({
    "type": "POST",
    "url": url,
    "data": body
  }).done(function (data) {
    callback(data.code_hash);
  }).fail(function (err) {
    callback(null);
  });
};

var append_locker = function (server1, account, desc) {
  getTable2(server1, "eoslocktoken", account, "lockss", "2", 20, (locked)=> {
    if (locked && locked.rows) {
      var append_str = '\n--------- LOCKS ---------\n';
      for(var i = 0; i < locked.rows.length; i++ ) {
        var onerow = locked.rows[i];
  
        var memo = onerow.memo;
        var ind = memo.indexOf("#", 1);
        if (ind > 0) memo = memo.substring(ind + 1);
        
        append_str += '@' + onerow.from + ' ' + get_token2(onerow.quantity);
  
        if (memo.length > 0)
          append_str += memo + ', ' + onerow.memo + '\n';
        else
          append_str += '\n';
      }
      
      desc.innerHTML += parseMemo(append_str);
    }
  });
};

var append_comments_from = function (server1, current_commenter, account, desc) {
  getTable2(server1, current_commenter, account, "commentss", "2", 20, (data) => { // Get top 20 commenter
    if (data && data.rows && data.rows.length > 0) {
      var append_str = '\n--------- COMMENTS ---------\n';
      for (var i = 0; i < data.rows.length; i++) {
        var onerow = data.rows[i];
        if (onerow.from === account && current_commenter === 'eosvrcomment') continue;
        
        append_str += '@' + onerow.from;
        
        if (current_linker) {
          append_str += '@' + current_linker;
        }
        
        append_str += ' ' + get_token2(onerow.deposit) + ',' + onerow.memo + '\n';
  
        // ===== Details =====
        if (onerow.memo) {
          var details = '@' + onerow.from;
  
          if (current_linker) {
            details += '@' + current_linker;
          }
          details += '+' + current_commenter + ' ... \n';
  
          append_str += details;
        }
        
        append_str += '\n';
      }
      
      desc.innerHTML += parseMemo(append_str);
    }
  });
};

var append_comments = function (server1, account, desc) {
  getTable2(server1, account, account, "gcommentss", "2", 20, (data) => { // Get top 20 commenter
    if (data && data.rows && data.rows.length > 0) {
      var append_str = '\n--------- RANK ---------\n';
      for (var i = 0; i < data.rows.length; i++) {
        var onerow = data.rows[i];
        append_str += '@' + onerow.account;
        
        if (current_linker) {
          append_str += '@' + current_linker;
        }
  
        append_str += ' ' + get_token2(onerow.total);
        
        if (onerow.totaldown !== 0) {
          append_str += '(' + get_token2(onerow.totaldown) + ')';
        }
        append_str += '\n';
        
        // ===== Details =====
        var details = '@' + onerow.account;
        
        if (current_linker) {
          details += '@' + current_linker;
        }
        details += '+' + account + ' ... \n';
  
        append_str += details;
      }
      
      desc.innerHTML += parseMemo(append_str);
    }
  });
};

var append_extra = function (server1, account, desc) {
  getCode(server1, account, (code) => {
    if (code === COMMENT_HASH) { // Comment Contract
      append_comments(server1, account, desc);
    } else if (current_commenter) {
      append_comments_from(server1, current_commenter, account, desc);
    } else {
      append_locker(server1, account, desc);
    }
  })
};


var changeId = function (id, name1, limit_status, desc, evd, portrait, user_limit_strs, eosserver) {
  if (!id) return;
  
  eosserver = fixserver(eosserver);
  
  extractId(id);
  
  if (!current_id) id = 'NA';
  
  var shortId = id.length > 12 ? (id.substring(0, 10) + '..') : id;
  name1.innerText = shortId;
  desc.innerHTML = '...';
  limit_status.innerText = '...';
  evd.innerText = '...';
  portrait.src = "data:image/jpg;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==";
  current_limit = 0;
  current_is_linker = false;
  
  if (!current_id) return;
  
  getSideChainServer(eosserver, current_linker, (server) => {
    if (!server) {
      desc.innerHTML = 'Invalid Side Chain';
      return;
    }
  
    server = fixserver(server);
    
    let comment_contract = current_commenter || "eosvrcomment";
    let comment_to = current_to || current_id;
    
    // Description
    getTable(server, comment_contract, current_id, "commentss", comment_to, 1, (dat)=> {
      if (dat && dat.rows && dat.rows.length > 0 && dat.rows[0].from === comment_to) {
        var memo = dat.rows[0].memo;
        desc.innerHTML = parseMemo(memo);
        current_is_linker = (memo.indexOf('chain:') >= 0);
      } else {
        desc.innerHTML = '';
      }
      
      append_extra(server, current_id, desc);
    });
    
    // Portrait
    getTable(server, "evrportraits", current_id, "commentss", current_id, 1, (dat)=> {
      if (dat && dat.rows && dat.rows.length > 0) {
        portrait.src = "data:image/jpg;base64," + dat.rows[0].memo;
      } else {
        portrait.src = "";
      }
    });
    
    // EVD
    getTable(server, "eoslocktoken", "eoslocktoken", "lockss", current_id, 1, (locked)=> {
      current_locked_token = 0;
      if (locked && locked.rows && locked.rows.length > 0) {
        if (locked.rows[0].from === current_id) {
          current_locked_token = locked.rows[0].quantity;
        }
      }
      
      getTable(server, "eoslocktoken", current_id, "accounts", "EVD", 1, (tokens) => {
        current_evd_token = 0;
        if (tokens && tokens.rows && tokens.rows.length > 0) {
          current_evd_token = +(tokens.rows[0].balance.split(' ')[0]) * 10000;
        }
        
        evd.innerText = get_token();
  
        // Limit
        getTable(server, "eoslocktoken", current_id, "timelockss", 0, 1, (dat)=> {
          if (dat && dat.rows && dat.rows.length > 0) {
            current_limit = dat.rows[0].quantity;
          }
          limit_status.innerText = getLimitStr(user_limit_strs);
        });
        
      });
    });
    
  });
};

// id like: aaa@bbb+ccc-ddd
// aaa: id,
// bbb: linker,
// ccc: commenter
// ddd: to
var extractId = function (id) {
  console.log("ID:", id);
  
  var ind3 = id.indexOf('-');
  
  var p1, p2, p3, p4;
  if (ind3 > 0) {
    p3 = id.substring(0, ind3);
    p4 = id.substring(ind3 + 1);
  } else {
    p3 = id;
    p4 = '';
  }
  
  var ind2 = p3.indexOf('+');
  if (ind2 > 0) {
    p2 = p3.substring(0, ind2);
    p3 = p3.substring(ind2 + 1);
  } else {
    p2 = p3;
    p3 = '';
  }
  
  var ind = p2.indexOf('@');
  if (ind > 0) {
    p1 = p2.substring(0, ind);
    p2 = p2.substring(ind + 1);
  } else {
    p1 = p2;
    p2 = '';
  }
  
  current_id = expandRepeat(p1);
  current_linker = expandRepeat(p2);
  current_commenter = expandRepeat(p3);
  current_to = expandRepeat(p4);
  
  console.log('id:', current_id);
  console.log('linker:', current_linker);
  console.log('commenter:', current_commenter);
  console.log('to:', current_to);
};

var expandRepeat = function (id) {
  var l1 = id.split('_');
  if (l1.length <= 1) {
    if (id.length <= 12)
      return id;
    else
      return '';
  }
  
  var a = l1[0];
  var b = l1[1];
  
  if (a.length > 12 || b.length > 12) {
    return '';
  }
  
  if (b.length === 0) {
    b = a;
    a = '';
  }
  
  var result = a;
  while (result.length < 12) {
    result = result + b;
  }
  
  return result.substring(0, 12);
};
