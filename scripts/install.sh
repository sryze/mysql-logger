#!/bin/sh

plugin_dir=$(mysql_config --plugindir)
if [ $? -ne 0 ]; then
  echo "install: Failed to determine plugin directory path (mysql_config exited with non-zero status)"
  exit 1
fi

echo "install: Stopping MySQL server"
service mysql stop || exit 1

echo "install: Copying plugin to $plugin_dir"
cp logger.so $plugin_dir/ && chmod ag+r $plugin_dir/logger.so || exit 1

echo "install: Starting MySQL server"
service mysql start || exit 1

echo "install: Installing plugin"
mysql -uroot -e "install plugin logger soname 'logger.so'"
