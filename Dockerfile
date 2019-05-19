FROM mariadb

RUN apt-get update
RUN apt-get install -y gdb vim curl

COPY --chown=root:root scripts/docker/logger.cnf /etc/mysql/conf.d/logger.cnf
RUN chmod ag-w /etc/mysql/conf.d/logger.cnf

COPY scripts/init.sql /docker-entrypoint-initdb.d
