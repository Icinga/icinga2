#!/bin/sh

TIMEOUT=30

case $1 in
    mysql)
        TYPE='MySQL'
        CMD='/usr/bin/mysql -t -D icinga -u icinga --password=icinga -e'
        ;;
    pgsql)
        TYPE='PostgreSQL'
        CMD='/usr/bin/psql -nq -U icinga -d icinga -c'
        export PGPASSWORD='icinga'
        ;;
    *)
        echo "No IDO type specifier given!"
        exit 1
        ;;
esac

tries=1
while true
do
    out="`$CMD 'select * from icinga_hosts'`"

    if [ $tries -lt $TIMEOUT ] && [ "$out" == "" ];
    then
        sleep 1
        tries=$(($tries + 1))
    else
        if [ $tries -eq $TIMEOUT ];
        then
            echo "IDO ($TYPE) does not have any hosts or is not responding" >&2
        fi

        break
    fi
done
