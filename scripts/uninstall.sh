#!/bin/sh

plugin_dir=$(mysql_config --plugindir)
if [ $? -ne 0 ]; then
  echo "uninstall: Failed to determine plugin directory path (mysql_config exited with non-zero status)"
  exit 1
fi

echo "uninstall: Uninstalling plugin"
mysql -uroot -e "uninstall plugin logger"

echo "uninstall: Stopping MySQL server"
service mysql stop || exit 1

echo "uninstall: Removing plugin from $plugin_dir"
rm "$plugin_dir/logger.so" || exit 1

echo "uninstall: Starting MySQL server"
service mysql start || exit 1
