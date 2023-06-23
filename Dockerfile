FROM ghcr.io/userver-framework/ubuntu-userver-build-base:v1 as build

COPY . /project
WORKDIR /project
RUN git submodule update --init && make build-release

RUN apt-get -y update && apt-get install -y postgresql-14

USER postgres

RUN /etc/init.d/postgresql start &&\
    psql --command "CREATE USER docker WITH SUPERUSER PASSWORD 'docker';" &&\
    createdb -O docker forum &&\
    /etc/init.d/postgresql stop


USER root

EXPOSE 5000
ENV PGPASSWORD docker

CMD service postgresql start &&  psql -h localhost -d forum -U docker -p 5432 -a -q -f ./db/db.sql && \
    cd build_release && ./vkpg --config ../configs/static_config.yaml