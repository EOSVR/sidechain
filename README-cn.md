## 侧链搭建

[English](README.md)

### 前提条件

为了搭建一条EOS侧链，需要如下前提条件：

- 搭建一条侧链，并搭建上所需要的合约。详见：[侧链初始化与合约搭建](setup_chain.md)

- 设置连接者，让连接帐号在两条链（主链和侧链）上都有帐号，名字一样，比如: eoslinker111。
最好连接者的两个帐号都用同一个 active 公钥，可以用类似如下命令修改 active 公钥：

    ```
    cleos set account permission eoslinker111 active '{"threshold": 1,"keys": [{"key": "EOS6noQsmvdoddJicNsTQD6sMhKxT5JPvFh7J61Na31oigR94ZWS2","weight": 1}],"accounts": []}' owner -p eoslinker111
    ```


- 主链上，eoslinker111 将一些 EVD 发送(焚毁)到 eoslocktoken ，并带上侧链的 chain_id 。Chain_id 可以通过命令 ```cleos get info``` 获取。
    比如：

    ```
    cleos push action eoslocktoken transfer '{"from":"eoslinker111", "to":"eoslocktoken","quantity":"1000.0000 EVD","memo":"b6a3a2e75f6fc47e7ef8b413ae4ee6eb3a8fefcd01c0b0ecdf688563cfa5f493"}' -p eoslinker111

    # 检查发送情况
    cleos get table eoslocktoken eoslocktoken depositss

    ```

    在侧链上，给这个连接帐号 issue 同样多的EVD；

    其他人检查侧链是否可信时，用如下命令在两条链上都运行，对比一下就知道了。

    ```
    cleos get table eoslocktoken eoslocktoken depositss
    ```



### 服务搭建

1, 运行 "npm i";

2, 将 "config_example.js" 复制为 "config.js" 并将 "config.js" 修改为你的设置；

3, 运行 "node linker.js" ，就可以两边互相传输了；

注意：连接者在两边都要抵押足够的 CPU 和一点RAM 来维持操作，并且两边都要有足够 EVD。



### 展示用侧链

可以用这个侧链来实验：http://s1.eosvr.io:8888/

它的连接者是 : eoslinker111

查询所有链上帐号的网站： http://id.eosvr.io/

网站源码在 html 目录下。



### 联系方式

Twitter: https://twitter.com/EVD89490917

Medium: https://medium.com/@eosvr

Mail: contact@eosvr.io
