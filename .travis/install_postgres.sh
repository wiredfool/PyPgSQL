#!/bin/sh

sudo service postgresql stop
sudo apt-get remove --force-yes default-jre-headless
if [ "$POSTGRESQL_VERSION" = "9.4" ]; then
    sudo apt-get update
    sudo apt-get -y --force-yes install libpq5 libpq-dev postgresql-client-$POSTGRESQL_VERSION
fi
sudo apt-get -y --force-yes install postgresql-$POSTGRESQL_VERSION 

sudo pg_ctlcluster $POSTGRESQL_VERSION main start
sudo -u postgres createuser --cluster $POSTGRESQL_VERSION/main --superuser $USER
createdb --cluster $POSTGRESQL_VERSION/main pypgsql
export PG_PORT=`pg_lsclusters -h | grep $POSTGRESQL_VERSION | cut -d ' ' -f 3`