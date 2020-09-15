#!/bin/sh

DIR=$(realpath $(dirname $0))

mysql -h 127.0.0.1 -P 3307 -e "create database logger_testing" && \
mysql -h 127.0.0.1 -P 3307 -D logger_testing < $DIR/init.sql