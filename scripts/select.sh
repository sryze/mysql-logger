#!/bin/sh

DIR=$(realpath $(dirname $0))
. $DIR/include/data-dir.sh

QUERY="select * from fruits where name = 'banana' or name = 'apple' or name = 'orange' and price > 0 and price < 1000 and now() > '2018-01-01' and true = true;"
mysql -h 127.0.0.1 -P 3307 -D logger_testing -e "$QUERY" $*
