#!/bin/sh

npm test || exit

cleos set contract eosvrmarkets ../sidebancor/ || exit

echo OK



