#!/bin/sh

if [ -z "$DATA_DIR"]; then
  DATA_DIR=$PWD/data
fi

mysql_install_db --datadir=$DATA_DIR --user=$USER
