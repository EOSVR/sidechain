## EVD 奖励合约（eosvrrewards）

[English](README.md)

此合约用来给 [评论用合约](../comments/README-cn.md) 设置奖励，并给在评论用合约中被支持得最多的前几名发送奖励。


### 设置奖励

直接将 EVD 发送给奖励合约（eosvrrewards），可以设置奖励。memo 中可以设置奖励的具体参数：

参数依次为：
  一次可发放的最多代币数量，评价合约，奖励间隔（秒），每次得到奖励的最大人数，奖励种类，每一千支持能得到的最多奖励

它们的默认值为：
  30 EVD, eosvrcomment, 86400*7(7 days), 10(10个用户), 0(平均分配、可回撤), 10 (1%支持)

示例：

```
# guest1111111 将用 10万 EVD 来奖励在 eosvrcomment 合约中最受支持的 10个人，每天奖励一次，每次共有1万 EVD发放，平分给10个人（type：0）
# 所以，每人每次最多得到 1000 EVD。
# 同时，guest1111111 在之后随时可以终止这个奖励活动(type=0时, 可回撤).
cleos push action eoslocktoken transfer '{"from":"guest1111111", "to":"eosvrrewards","quantity":"100000.0000 EVD","memo":"10000,eosvrcomment,84600,10,0"}' -p guest1111111

# 发起者收回了奖励. ( rewardtype 类型必须是偶数 (0, 2, 4...) )
cleos push action eosvrrewards withdraw '{"to":"account1"}' -p account1


# account1 想要获得奖励，那么需要执行下列命令
# 并且，这个命令将刷新 eosvrrewards 中所有的奖励状态
cleos push action eosvrrewards reward '{"to":"account1"}' -p account1


```


### 奖励类型

奖励的发起者如果在奖励运行了一段时间后觉得不满意，可以选择撤回还没有发出的奖励。但只有偶数的类型可回撤，奇数的不可回撤。比如：0可以撤回，1不行。

- 奖励类型为 2 或者 3时, 第一名 50%, 其余平均；

- 奖励类型为 4 或者 5时, 第一名 50%, 第二名 25%，第三名 12.5% ，依次类推；

- 其他奖励类型都是平均分配。

当 type >= 10 时:

- 必须先申请奖励，否则不会有奖励；并且每次申请只能维持 33 days，之后需要再次申请；

```
# account1 申请了 rewardsender 发出的基于 eosvrcomment 合约排名的奖励，申请100EVD。即最多一次可以获取 100 EVD。
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"100.0000 EVD","memo":"#APPLY#eosvrcomment,rewardsender"}' -p account1

# 申请不能修改，但申请可以撤销。只是在撤销时，所有的当前支持都将被自动退回(反对不会)。
# account1 申请撤销：
cleos push action eoslocktoken transfer  '{"from":"account1", "to":"eosvrrewards","quantity":"0.0001 EVD","memo":"#CANCEL#"}' -p account1

```

- 最大收益 = (上次支持 - 现在反对 * ((rewardtype - 10) / 2 + 1)) * 支持转化率(默认1%)

上次支持指的是上次 reward 结算时的支持数量，现在反对指的是当前这一次 reward 结算时的反对数量，这是为了防止在reward即将结算的最后一刻前投支持票的情况。
这样的话，反对者有足够的时间进行投票。

在 type < 10 时, 最大收益 = （支持 - 反对）* 支持转化率。

```
# 比如：0.1G ，1M 每三天, 一共 100 次, 大约1年, 最多10人
cleos push action eoslocktoken transfer '{"from":"eosvrmanager", "to":"eosvrrewards","quantity":"100000000.0000 EVD","memo":"1000000,eosvrcomment,259200,10,13,10"}' -p eosvrmanager


```



