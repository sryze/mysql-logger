FROM mariadb

RUN apt-get update
RUN apt-get install -y curl vim

COPY --chown=root:root scripts/docker/logger.cnf /etc/mysql/conf.d/logger.cnf
RUN chmod ag-w /etc/mysql/conf.d/logger.cnf

ADD scripts/init.sql /docker-entrypoint-initdb.d
