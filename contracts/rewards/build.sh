#!/bin/sh

npm test || exit

cleos set contract eosvrrewards ../rewards/ || exit

echo OK



