#!/bin/bash

make
config_path="/home/artem/projects/c/unix-os-course-assignments/3/test-config"
cat <3-proc-config >$config_path
./myinit $config_path
myinit_pid=$(pidof myinit)
sleep 1
printf "ps after the start: \n\n"
ps --ppid $myinit_pid
pkill program2.sh
sleep 1
printf "\nps after killing the second process: \n\n"
ps --ppid $myinit_pid
cat <1-proc-config >$config_path
kill -HUP $myinit_pid
printf "\nps after changing the configuration file: \n\n"
ps --ppid $myinit_pid
printf "\nthe content of the log file: \n\n"
cat /tmp/my_init.log
kill -9 $myinit_pid