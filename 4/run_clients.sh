#!/bin/bash

clients_pids=()

for i in $(seq 1 $2);
do
    ./client $1 </tmp/numbers.txt >>/tmp/client123.log &
    clients_pids+=($!)
done

for client_pid in ${clients_pids[*]}; do
    wait $client_pid
done