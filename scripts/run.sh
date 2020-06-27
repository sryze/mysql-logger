#!/bin/sh

if [ -z "$DATA_DIR" ]; then
  DATA_DIR=$PWD/data
fi

mysqld \
  --console \
  --gdb \
  --skip-grant-tables \
  --port 3307 \
  --datadir $DATA_DIR \
  --socket /tmp/mysql.sock \
  --pid-file /tmp/mysql.pid \
  --plugin-dir $PWD \
  --plugin-load logger=logger.so \
  --plugin-maturity unknown
