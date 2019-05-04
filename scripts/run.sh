#!/bin/sh

if [ -z "$DATA_DIR"]; then
  DATA_DIR=$HOME/mysql-data
fi

mysqld \
  --skip-grant-tables \
  --datadir $DATA_DIR \
  --socket /tmp/mysql.sock \
  --pid-file /tmp/mysql.pid \
  --plugin-dir $PWD \
  --plugin-load logger=logger.so \
  --gdb \
