#!/bin/sh

DIR=$(realpath $(dirname $0))
. $DIR/include/data-dir.sh

mysql_install_db --datadir="$DATA_DIR"

if [ ! -z "$USER" ]; then
    chown -R $USER:$USER "$DATA_DIR"
fi
