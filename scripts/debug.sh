#!/bin/sh

TMPDIR=/tmp

if [ -z "$DATA_DIR" ]; then
  DATA_DIR="$TMPDIR/mysql-logger/data"
fi

if [ ! -d "$DATA_DIR" ]; then
  mkdir -p "$DATA_DIR"
fi

lldb mysqld -- \
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
