#!/bin/sh

plugin_dir=$(mysql_config --plugindir)
if [ $? -ne 0 ]; then
  echo "install: Failed to determine plugin directory path (mysql_config exited with non-zero status)"
  exit 1
fi

echo "install: Copying files and restarting MySQL server"
service mysql stop && \
  cp -v logger.so $plugin_dir/ && \
  chmod ag+r $plugin_dir/logger.so && \
  service mysql start

echo "install: Installing plugin"
mysql -uroot -e "install plugin logger soname 'logger.so'"
