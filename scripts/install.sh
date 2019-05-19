#!/bin/sh

plugin_dir=$(mysql_config --plugindir)
if [ $? -ne 0 ]; then
  echo "install: Failed to determine plugin directory path (mysql_config exited with non-zero status)"
  exit 1
fi

echo "install: Copying files and restarting MySQL server"
service mysql stop \
  cp -v logger.so $plugin_dir/ && \
  chmod ag+r /usr/lib/mysql/plugin/logger.so && \
  service mysql start

echo "install: Installing plugin (you will be asked for your MySQL root passsword)"
mysql -uroot -p -e "install plugin logger soname 'logger.so'"
