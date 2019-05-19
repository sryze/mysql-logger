#!/bin/sh

docker-compose exec mysql mysql -uroot -plogger -e "show variables like '%plugin%'"
