#!/bin/bash

make >/dev/null

./server >/tmp/server123.log &
server_pid=$!

./generate_numbers

sleep 1

for i in {1..3};
do
    ./run_clients.sh 0.1 100 >/dev/null

    echo -e "The output of the $i client sending a zero:"

    ./client 0 <<< "0"

    echo ""
done

echo -e "\nThe first connection log record:"

grep "New client connected" server123.log | head -n 1

echo -e "\nThe last connection log record:"

grep "New client connected" server123.log | tail -n 1

kill -1 $server_pid

sleep 0.25

rm /tmp/client123.log

for clients_count in {1,10,100};
do
    for delay in {0,0.2,0.4,0.6,0.8,1};
    do

        ./server >/tmp/server123.log &
        server_pid=$!

        sleep 1

        ./run_clients.sh $delay $clients_count
        kill -1 $server_pid

        sleep 0.25

        echo "Effecient speed of $clients_count clients with $delay delay: `./get_efficient_speed.sh` seconds"

        rm /tmp/client123.log
    done
done
