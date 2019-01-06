// === EOS SERVER ===
var current_id = '', current_linker = '';
var current_locked_token = 0, current_evd_token = 0;
var current_limit = 0;
var current_is_linker = false;

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
  if (current_limit <= 0 || current_limit > 50) return user_limit_strs[0];
  
  if (current_limit > 25) return user_limit_strs[1];
  
  if (current_limit > 10) return user_limit_strs[2];
  
  if (current_limit > 1) return user_limit_strs[3];
  
  return user_limit_strs[4];
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
      if (oneline.startsWith('http')) {
        oneline = '<a href="' + oneline + '">' + oneline + '</a>'
      } else if (oneline.startsWith('@')) {
        var ind = oneline.indexOf(' ');
        if (ind < 0) ind = oneline.length;
  
        if (ind > 1) {
          var account = oneline.substring(1, ind);
          oneline = '<a href="?id=' + account + '">' + account + '</a>' + oneline.substring(ind);
        }
      }
      
      result += oneline + '<br />';
    }
  }
  
  return result;
  
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
    
    // Description
    getTable(server, "eosvrcomment", current_id, "commentss", current_id, 1, (dat)=> {
      if (dat && dat.rows && dat.rows.length > 0) {
        var memo = dat.rows[0].memo;
        desc.innerHTML = parseMemo(memo);
        current_is_linker = (memo.indexOf('chain:') >= 0);
      } else {
        desc.innerHTML = '';
      }
    });
    
    // Portrait
    getTable(server, "evrportraits", current_id, "commentss", current_id, 1, (dat)=> {
      if (dat && dat.rows && dat.rows.length > 0) {
        portrait.src = "data:image/jpg;base64," + dat.rows[0].memo;
      } else {
        portrait.src = "";
      }
    });
    
    // Limit
    getTable(server, "eoslocktoken", current_id, "timelockss", 0, 1, (dat)=> {
      if (dat && dat.rows && dat.rows.length > 0) {
        current_limit = dat.rows[0].quantity;
      }
      limit_status.innerText = getLimitStr(user_limit_strs);
    });
    
    // EVD
    getTable(server, "eoslocktoken", "eoslocktoken", "lockss", current_id, 1, (locked)=> {
      current_locked_token = 0;
      if (locked && locked.rows && locked.rows.length > 0) {
        if (locked.rows[0].to === current_id) {
          current_locked_token = locked.rows[0].quantity;
        }
      }
      
      getTable(server, "eoslocktoken", current_id, "accounts", "EVD", 1, (tokens) => {
        current_evd_token = 0;
        if (tokens && tokens.rows && tokens.rows.length > 0) {
          current_evd_token = +(tokens.rows[0].balance.split(' ')[0]) * 10000;
        }
        
        evd.innerText = get_token();
      });
    });
    
  });
};

var extractId = function (id) {
  var ind = id.indexOf('@');
  var p1 = id;
  var p2 = '';
  if (ind > 0) {
    p1 = id.substring(0, ind);
    p2 = id.substring(ind + 1);
  }
  
  current_id = expandRepeat(p1);
  current_linker = expandRepeat(p2);
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
