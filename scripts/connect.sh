#!/bin/sh

SOCKET=
if [ `uname -s` = 'Linux' ]; then
  SOCKET="-S /tmp/mysql.sock"
fi

mysql -P3307 -uroot $SOCKET $*
