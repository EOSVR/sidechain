#!/bin/sh

npm test || exit

cleos set contract eoscardcards ../cards/ || exit

echo OK



