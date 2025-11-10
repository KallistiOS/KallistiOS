#!/bin/sh

cd `dirname $0`/../utils/dc-chain/profiles

ls -1 */*.mk |awk -F'/' '{gsub(/\.mk$/, "", $2); print "{\"platform\":\""$1"\",\"profile\":\""$2"\"}"}' | jq -R -s -c 'split("\n")[:-1]'
