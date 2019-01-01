#!/bin/sh

npm test || exit

cleos set contract eoslocktoken ../locktoken/ || exit

echo OK

