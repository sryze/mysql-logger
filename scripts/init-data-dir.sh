#!/bin/sh

DIR=$(realpath $(dirname $0))
. $DIR/include/data-dir.sh

mysql_install_db --datadir="$DATA_DIR" --user=$USER \
    && chown -R $USER:$USER "$DATA_DIR"
