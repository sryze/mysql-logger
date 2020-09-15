TMPDIR=/tmp

if [ -z "$DATA_DIR" ]; then
  DATA_DIR="$TMPDIR/mysql-logger/data"
fi

if [ ! -d "$DATA_DIR" ]; then
  mkdir -p "$DATA_DIR"
fi
