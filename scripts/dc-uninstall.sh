#!/bin/sh

docker-compose exec mysql mysql -uroot -plogger -e "uninstall plugin logger"
