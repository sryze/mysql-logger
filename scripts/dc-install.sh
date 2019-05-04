#!/bin/sh

docker-compose exec mysql mysql -uroot -plogger -e "install plugin logger soname 'logger.so'"
