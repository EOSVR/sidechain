
#include "../locktoken/locktoken.hpp"


using namespace eosio;

using std::string;


struct cards {
    int64_t   id;  // = create_time
    name creator;
    int64_t   mark;
    int64_t   max_supply;      // Max number of this item (Only when creator = tableName)
    int64_t   total;           // Total number of this item
    int64_t   sell;            // Total number of this item to sell
    int64_t   price;
    int64_t   lastupdate;
    string    content;
    int64_t   rampayed; // 0.1 EOS / K , EOS:EVD = 1: 100 => 100EVD = 10K => 1 EVD = 100byte => 100 EVDS = 1 byte

    // 当使用 reg '{"from":"guest1111111", "content":"XXXXX", "id":1558446691000000, "total":1000, "sell":-50, "price":10000}' -p guest1111111 时，
    // 一张矿山卡将建立（sell < 0），这时 creator 账户中：
    //      rampayed < 0, 表示矿量，记录的是 eoscardcards 收到的所有EVD的数量（-XXX），初始-1。
    //      max_supply 表示的是总共卖出的数量。
    //      total 表示每张每天的分红数量。（比如：1000，表示每张每天可以分 0.1 EVD）。
    //      sell 表示矿山大概持续时间（天）
    // 挖矿 = Min(total, -rampayed / max_supply / sell)  (当 now - lastupdate > 1 day 时)
    // 执行命令为： dig '{"card": "123184920012", "owner": "user111111111"}'
    // Service 命令为: transfer {..., "to": "eoscardcards", ... , "memo": "guest1111111,123184920012"}
    // 表示直接加 rampayed 中的量。
    //
    // 限制：
    // 1, 矿山卡无法销毁 (total < 0 不起作用)
    // 2, creator无法买自己
    // 3, 之后的 reg 无法修改 total 和 sell ，只能修改 price
    // 4, price 影响挖矿的允许间隔。 间隔为[1, price / 10,0000]day，price 为 0 的话无上限;
    // 5, 如果不想加人的话，price 改为 0，将永久封闭加入(之前卖出的还可以流通)。

    uint64_t primary_key()const { return id; }

    uint64_t second_key()const { return mark; }

    uint64_t third_key()const { return lastupdate; }

    EOSLIB_SERIALIZE( cards, (id)(creator)(mark)(max_supply)(total)(sell)(price)(lastupdate)(content)(rampayed) );
};

typedef eosio::multi_index<"cardss"_n, cards,
        indexed_by< "bymark"_n,
                const_mem_fun< cards, uint64_t, &cards::second_key>>,
        indexed_by< "byupdate"_n,
                const_mem_fun< cards, uint64_t, &cards::third_key>>
> cardTable;

struct reg {
    int64_t   id;
    name from;
    int64_t   total;
    int64_t   sell;
    int64_t   price;
    string    content;

    EOSLIB_SERIALIZE( reg, (id)(from)(total)(sell)(price)(content));
};

class currency {
public:
    struct transfer
    {
        uint64_t from;
        uint64_t to;
        asset        quantity;
        string       memo;

        EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(memo) )
    };
};
