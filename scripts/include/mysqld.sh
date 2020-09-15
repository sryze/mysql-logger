MYSQLD=$(which mysqld)
if [ -z "$MYSQLD" ] || [ ! -f "$MYSQLD" ]; then
  MYSQLD=/usr/libexec/mysqld
fi
if [ ! -f "$MYSQLD" ]; then
  MYSQLD=/usr/local/libexec/mysqld
fi
if [ ! -f "$MYSQLD" ]; then
  echo "Sorry, could not find mysqld"
  exit 1
fi
