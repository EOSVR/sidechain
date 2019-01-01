
#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/multi_index.hpp>
#include <string>

#include "../locktoken/locktoken.hpp"

using namespace eosio;

using std::string;


struct comments {
    account_name    to;

    int64_t    deposit;
    string     memo;

    int64_t    weight;  // [1..100], default: 1.  gcomments.total += deposit * weight;
    int64_t    lastupdate; // Last time use EVD to support or against someone

    account_name primary_key()const { return to; }

    EOSLIB_SERIALIZE( comments, (to)(deposit)(memo)(weight)(lastupdate) );
};

typedef eosio::multi_index<N(commentss), comments> commentTable;

struct gcomments {
    account_name account;
    int64_t total;
    int64_t totaldown;
    int64_t lastupdate;     // Last time a comment have been supported (ms)

    auto primary_key() const { return account; }

    uint64_t get_total() const { return INT64_MAX - total - lastupdate / 8640000; } // 0x7fffffffffffffff, -1 EVD / day

    EOSLIB_SERIALIZE( gcomments, (account)(total)(totaldown)(lastupdate) );
};

typedef eosio::multi_index<
    N(gcommentss), gcomments,
    indexed_by< N( bytotal ), const_mem_fun< gcomments, uint64_t, &gcomments::get_total> >
    > gcommentTable;


struct comment {
    account_name from;
    account_name to;
    string memo;

    EOSLIB_SERIALIZE( comment, (from)(to)(memo));
};

struct withdraw {
    account_name from;
    account_name to;

    EOSLIB_SERIALIZE( withdraw, (from)(to));
};

// Dismiss all supports to account.
// Can only call by eosvrrewards
// It will be called when a user want to withdraw its need. At that time, all supports to it will be sent back.
struct dismiss {
    account_name account;

    EOSLIB_SERIALIZE( dismiss, (account));
};
