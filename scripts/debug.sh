#!/bin/sh

if [ -z "$DATA_DIR"]; then
  DATA_DIR=$HOME/mysql-data
fi

lldb mysqld -- \
  --datadir $DATA_DIR \
  --socket /tmp/mysql.sock \
  --pid-file /tmp/mysql.pid \
  --plugin-dir $PWD \
  --plugin-load logger=logger.so \
  --gdb \
