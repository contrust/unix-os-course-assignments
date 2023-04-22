#!/bin/bash

set -e

run_lock_forever()
{
    sigint_handler()
    {
        echo $successful_locks_count >> $logfile
        exit 0
    }

    trap sigint_handler SIGINT

    successful_locks_count=0
    while true;
    do
        ./myprogram $filename && successful_locks_count=$((successful_locks_count+1))
    done
}

filename=testfile
lock_filename="${filename}.lck"
logfile=successful_locks.log

pids=()

for i in {1..10};
do
    run_lock_forever &
    pids+=($!)
done

sleep 3
echo error > $lock_filename
sleep 1
rm $lock_filename
sleep 296

for pid in ${pids[@]};
do
    kill -INT $pid
done
