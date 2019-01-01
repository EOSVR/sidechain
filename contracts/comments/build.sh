#!/bin/sh

npm test || exit

cleos set contract eosvrcomment ../comments/ || exit

echo OK



