#!/bin/sh

docker-compose exec mysql cat /var/lib/mysql/logger.log
