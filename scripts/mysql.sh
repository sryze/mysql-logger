#!/bin/sh

SOCKET=
if [ `uname -s` = 'Linux' ]; then
  SOCKET="-S /tmp/mysql.sock"
fi

mysql -uroot $SOCKET $*
