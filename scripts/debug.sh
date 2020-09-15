#!/bin/sh

DIR=$(realpath $(dirname $0))
. $DIR/include/data-dir.sh
. $DIR/include/mysqld.sh

lldb $MYSQLD -- \
  --no-defaults \
  --console \
  --gdb \
  --skip-grant-tables \
  --port 3307 \
  --datadir "$DATA_DIR" \
  --socket "$DATA_DIR/mysql.sock" \
  --pid-file "$DATA_DIR/mysql.pid" \
  --plugin-dir "$PWD" \
  --plugin-load logger=logger.so \
  --plugin-maturity unknown
