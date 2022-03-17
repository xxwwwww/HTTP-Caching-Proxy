#!/bin/bash
make clean
make
./proxy_daemon &
while true
do
    sleep 1
done
