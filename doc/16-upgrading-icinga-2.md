# Upgrading Icinga 2 <a id="upgrading-icinga-2"></a>

Upgrading Icinga 2 is usually quite straightforward. Ordinarily the only manual steps involved
are scheme updates for the IDO database.

## Upgrading to v2.8 <a id="upgrading-to-2-8"></a>

The default certificate path was changed from `/etc/icinga2/pki` to
`/var/lib/icinga2/certs`.

This applies to Windows clients in the same way: `%ProgramData%\etc\icinga2\pki`
was moved to `%ProgramData%`\var\lib\icinga2\certs`.

The [setup CLI commands](06-distributed-monitoring.md#distributed-monitoring-setup-master) and the
default [ApiListener configuration](06-distributed-monitoring.md#distributed-monitoring-apilistener)
have been adjusted to these paths too.

## Upgrading to v2.7 <a id="upgrading-to-2-7"></a>

v2.7.0 provided new notification scripts and commands. Please ensure to
update your configuration accordingly. An advisory has been published [here](https://www.icinga.com/2017/08/23/advisory-for-icinga-2-v2-7-update-and-mail-notification-scripts/).

In case are having troubles with OpenSSL 1.1.0 and the
public CA certificates, please read [this advisory](https://www.icinga.com/2017/08/30/advisory-for-ssl-problems-with-leading-zeros-on-openssl-1-1-0/)
and check the [troubleshooting chapter](15-troubleshooting.md#troubleshooting).

If Icinga 2 fails to start with an empty reference to `$ICINGA2_CACHE_DIR`
ensure to set it inside `/etc/sysconfig/icinga2` (RHEL) or `/etc/default/icinga2` (Debian).

RPM packages will put a file called `/etc/sysconfig/icinga2.rpmnew`
if you have modified the original file.

Example on CentOS 7:

```
vim /etc/sysconfig/icinga2

ICINGA2_CACHE_DIR=/var/cache/icinga2

systemctl restart icinga2
```

## Upgrading the MySQL database <a id="upgrading-mysql-db"></a>

If you're upgrading an existing Icinga 2 instance, you should check the
`/usr/share/icinga2-ido-mysql/schema/upgrade` directory for an incremental schema upgrade file.

> **Note**
>
> If there isn't an upgrade file for your current version available, there's nothing to do.

Apply all database schema upgrade files incrementally.

    # mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/<version>.sql

The Icinga 2 DB IDO module will check for the required database schema version on startup
and generate an error message if not satisfied.


**Example:** You are upgrading Icinga 2 from version `2.0.2` to `2.3.0`. Look into
the *upgrade* directory:

    $ ls /usr/share/icinga2-ido-mysql/schema/upgrade/
    2.0.2.sql  2.1.0.sql 2.2.0.sql 2.3.0.sql

There are two new upgrade files called `2.1.0.sql`, `2.2.0.sql` and `2.3.0.sql`
which must be applied incrementally to your IDO database.

## Upgrading the PostgreSQL database <a id="upgrading-postgresql-db"></a>

If you're updating an existing Icinga 2 instance, you should check the
`/usr/share/icinga2-ido-pgsql/schema/upgrade` directory for an incremental schema upgrade file.

> **Note**
>
> If there isn't an upgrade file for your current version available, there's nothing to do.

Apply all database schema upgrade files incrementally.

    # export PGPASSWORD=icinga
    # psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/<version>.sql

The Icinga 2 DB IDO module will check for the required database schema version on startup
and generate an error message if not satisfied.

**Example:** You are upgrading Icinga 2 from version `2.0.2` to `2.3.0`. Look into
the *upgrade* directory:

    $ ls /usr/share/icinga2-ido-pgsql/schema/upgrade/
    2.0.2.sql  2.1.0.sql 2.2.0.sql 2.3.0.sql

There are two new upgrade files called `2.1.0.sql`, `2.2.0.sql` and `2.3.0.sql`
which must be applied incrementally to your IDO database.
