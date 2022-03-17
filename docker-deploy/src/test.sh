#! /usr/bin/bash
make clean
make
./proxy_daemon
nc localhost 12345 < tests/valid_http_get.txt
cat /var/log/erss/proxy.log > output.txt
