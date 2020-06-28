#!/bin/sh

TMPDIR=/tmp

if [ -z "$DATA_DIR" ]; then
  DATA_DIR="$TMPDIR/mysql-logger/data"
fi

if [ ! -d "$DATA_DIR" ]; then
  mkdir -p "$DATA_DIR"
fi

mysql_install_db --datadir="$DATA_DIR" --user=$USER \
    && chown -R $USER:$USER "$DATA_DIR"
