#!/bin/sh

DIR=$(realpath $(dirname $0))
. $DIR/include/data-dir.sh
. $DIR/include/mysqld.sh

case "$1" in
  --logger-lib)
    shift
    logger_lib="$1"
    if [ -z "$logger_lib" ]; then
      echo "Option --logger-lib requires a value"
      exit 1
    fi
    shift
    ;;
  *)
    echo "Unknown option: $1"
    exit 1
esac

plugin_dir=$(realpath $(dirname "$logger_lib"))
plugin_name=$(basename $logger_lib)

"$MYSQLD" \
  --no-defaults \
  --console \
  --gdb \
  --skip-grant-tables \
  --port 3307 \
  --datadir "$DATA_DIR" \
  --socket "$DATA_DIR/mysql.sock" \
  --pid-file "$DATA_DIR/mysql.pid" \
  --plugin-dir "$plugin_dir" \
  --plugin-load logger="$plugin_name" \
  --plugin-maturity unknown
