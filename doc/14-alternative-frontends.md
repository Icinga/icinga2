# <a id="alternative-frontends"></a> Alternative Frontends

## <a id="setting-up-icinga-classic-ui"></a> Setting up Icinga Classic UI 1.x

Icinga 2 can write `status.dat` and `objects.cache` files in the format that
is supported by the Icinga 1.x Classic UI. [External commands](4-advanced-topics.md#external-commands)
(a.k.a. the "command pipe") are also supported. It also supports writing Icinga 1.x
log files which are required for the reporting functionality in the Classic UI.

### <a id="installing-icinga-classic-ui"></a> Installing Icinga Classic UI 1.x

The Icinga package repository has both Debian and RPM packages. You can install
the Classic UI using the following packages:

  Distribution  | Packages
  --------------|---------------------
  Debian        | icinga2-classicui
  RHEL/SUSE     | icinga2-classicui-config icinga-gui

The Debian packages require additional packages which are provided by the
[Debian Monitoring Project](http://www.debmon.org) (`debmon`) repository.

`libjs-jquery-ui` requires at least version `1.10.*` which is not available
in Debian 7 (Wheezy) and Ubuntu 12.04 LTS (Precise). Add the following repositories
to satisfy this dependency:

  Distribution  		| Package Repositories
  ------------------------------|------------------------------
  Debian Wheezy 		| [wheezy-backports](http://backports.debian.org/Instructions/) or [debmon](http://www.debmon.org)
  Ubuntu 12.04 LTS (Precise)    | [Icinga PPA](https://launchpad.net/~formorer/+archive/icinga)

On all distributions other than Debian you may have to restart both your web
server as well as Icinga 2 after installing the Classic UI package.

Icinga Classic UI requires the [StatusDataWriter](4-advanced-topics.md#status-data), [CompatLogger](4-advanced-topics.md#compat-logging)
and [ExternalCommandListener](4-advanced-topics.md#external-commands) features.
Enable these features and restart Icinga 2.

    # icinga2 feature enable statusdata compatlog command

In order for commands to work you will need to [setup the external command pipe](2-getting-started.md#setting-up-external-command-pipe).

### <a id="setting-up-icinga-classic-ui-summary"></a> Setting Up Icinga Classic UI 1.x Summary

Verify that your Icinga 1.x Classic UI works by browsing to your Classic
UI installation URL:

  Distribution  | URL                                                                      | Default Login
  --------------|--------------------------------------------------------------------------|--------------------------
  Debian        | [http://localhost/icinga2-classicui](http://localhost/icinga2-classicui) | asked during installation
  all others    | [http://localhost/icinga](http://localhost/icinga)                       | icingaadmin/icingaadmin

For further information on configuration, troubleshooting and interface documentation
please check the official [Icinga 1.x user interface documentation](http://docs.icinga.org/latest/en/ch06.html).

## <a id="setting-up-icinga-web"></a> Setting up Icinga Web 1.x

Icinga 2 can write to the same schema supplied by `Icinga IDOUtils 1.x` which
is an explicit requirement to run `Icinga Web` next to the external command pipe.
Therefore you need to setup the [DB IDO feature](2-getting-started.md#configuring-db-ido-mysql) remarked in the previous sections.

### <a id="installing-icinga-web"></a> Installing Icinga Web 1.x

The Icinga package repository has both Debian and RPM packages. You can install
Icinga Web using the following packages:

  Distribution  | Packages
  --------------|-------------------------------------------------------
  RHEL/SUSE     | icinga-web icinga-web-{mysql,pgsql}
  Debian        | icinga-web icinga-web-config-icinga2-ido-{mysql,pgsql}

### <a id="icinga-web-rpm-notes"></a> Icinga Web 1.x on RPM based systems

Additionally you need to setup the `icinga_web` database and import the database schema.
Details can be found in the package `README` files, for example [README.RHEL](https://github.com/Icinga/icinga-web/blob/master/doc/README.RHEL)

The Icinga Web RPM packages install the schema files into
`/usr/share/doc/icinga-web-*/schema` (`*` means package version).

On SuSE-based distributions the schema files are installed in
`/usr/share/doc/packages/icinga-web/schema`.

Example for RHEL and MySQL:

    # mysql -u root -p

    mysql> CREATE DATABASE icinga_web;
           GRANT SELECT, INSERT, UPDATE, DELETE, DROP, CREATE VIEW, INDEX, EXECUTE ON icinga_web.* TO 'icinga_web'@'localhost' IDENTIFIED BY 'icinga_web';
           quit

    # mysql -u root -p icinga_web <  /usr/share/doc/icinga-web-<version>/schema/mysql.sql

Icinga Web requires the IDO feature as database backend using MySQL or PostgreSQL.
Enable that feature, e.g. for MySQL.

    # icinga2 feature enable ido-mysql

If you've changed your default credentials you may either create a read-only user
or use the credentials defined in the IDO feature for Icinga Web backend configuration.
Edit `databases.xml` accordingly and clear the cache afterwards. Further details can be
found in the [Icinga Web documentation](http://docs.icinga.org/latest/en/icinga-web-config.html).

    # vim /etc/icinga-web/conf.d/databases.xml

    # icinga-web-clearcache

Additionally you need to enable the `command` feature for sending [external commands](4-advanced-topics.md#external-commands):

    # icinga2 feature enable command

In order for commands to work you will need to [setup the external command pipe](2-getting-started.md#setting-up-external-command-pipe).

Then edit the Icinga Web configuration for sending commands in `/etc/icinga-web/conf.d/access.xml`
(RHEL) or `/etc/icinga-web/access.xml` (SUSE) setting the command pipe path
to the default used in Icinga 2. Make sure to clear the cache afterwards.

    # vim /etc/icinga-web/conf.d/access.xml

                <write>
                    <files>
                        <resource name="icinga_pipe">/var/run/icinga2/cmd/icinga2.cmd</resource>
                    </files>
                </write>

    # icinga-web-clearcache

> **Note**
>
> The path to the Icinga Web `clearcache` script may differ. Please check the
> [Icinga Web documentation](https://docs.icinga.org) for details.

### <a id="icinga-web-debian-notes"></a> Icinga Web on Debian systems

Since Icinga Web `1.11.1-2` the IDO auto-configuration has been moved into
additional packages on Debian and Ubuntu.

The package `icinga-web` no longer configures the IDO connection. You must now
use one of the config packages:

 - `icinga-web-config-icinga2-ido-mysql`
 - `icinga-web-config-icinga2-ido-pgsql`

These packages take care of setting up the [DB IDO](2-getting-started.md#configuring-db-ido-mysql) configuration,
enabling the external command pipe for Icinga Web and depend on
the corresponding packages of Icinga 2.

Please read the `README.Debian` files for details and advanced configuration:

 - `/usr/share/doc/icinga-web/README.Debian`
 - `/usr/share/doc/icinga-web-config-icinga2-ido-mysql/README.Debian`
 - `/usr/share/doc/icinga-web-config-icinga2-ido-pgsql/README.Debian`

When changing Icinga Web configuration files make sure to clear the config cache:

    # /usr/lib/icinga-web/bin/clearcache.sh

> **Note**
>
> If you are using an older version of Icinga Web, install it like this and adapt
> the configuration manually as shown in [the RPM notes](14-alternative-frontends.md#icinga-web-rpm-notes):
>
> `apt-get install --no-install-recommends icinga-web`

### <a id="setting-up-icinga-web-summary"></a> Setting Up Icinga Web 1.x Summary

Verify that your Icinga 1.x Web works by browsing to your Web installation URL:

  Distribution  | URL                                                         | Default Login
  --------------|-------------------------------------------------------------|--------------------------
  Debian        | [http://localhost/icinga-web](http://localhost/icinga-web)  | asked during installation
  all others    | [http://localhost/icinga-web](http://localhost/icinga-web)  | root/password

For further information on configuration, troubleshooting and interface documentation
please check the official [Icinga 1.x user interface documentation](http://docs.icinga.org/latest/en/ch06.html).

