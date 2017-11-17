# Upgrading Icinga 2 <a id="upgrading-icinga-2"></a>

Upgrading Icinga 2 is usually quite straightforward. Ordinarily the only manual steps involved
are scheme updates for the IDO database.

Specific version upgrades are described below. Please note that version
updates are incremental. An upgrade from v2.6 to v2.8 requires to
follow the instructions for v2.7 too.

## Upgrading to v2.8 <a id="upgrading-to-2-8"></a>

### DB IDO Schema Update to 2.8.0 <a id="upgrading-to-2-8-db-ido"></a>

There are additional indexes and schema fixes which require an update.

Please proceed here for [MySQL](16-upgrading-icinga-2.md#upgrading-mysql-db) or [PostgreSQL](16-upgrading-icinga-2.md#upgrading-postgresql-db).

### Changed Certificate Paths <a id="upgrading-to-2-8-certificate-paths"></a>

The default certificate path was changed from `/etc/icinga2/pki` to
`/var/lib/icinga2/certs`.

  Old Path                                           | New Path
  ---------------------------------------------------|---------------------------------------------------
  `/etc/icinga2/pki/icinga2-client1.localdomain.crt` | `/var/lib/icinga2/certs/icinga2-client1.localdomain.crt`
  `/etc/icinga2/pki/icinga2-client1.localdomain.key` | `/var/lib/icinga2/certs/icinga2-client1.localdomain.key`
  `/etc/icinga2/pki/ca.crt`                          | `/var/lib/icinga2/certs/ca.crt`

This applies to Windows clients in the same way: `%ProgramData%\etc\icinga2\pki`
was moved to `%ProgramData%\var\lib\icinga2\certs`.

  Old Path                                                        | New Path
  ----------------------------------------------------------------|----------------------------------------------------------------
  `%ProgramData%\etc\icinga2\pki\icinga2-client1.localdomain.crt` | `%ProgramData%\var\lib\icinga2\certs\icinga2-client1.localdomain.crt`
  `%ProgramData%\etc\icinga2\pki\icinga2-client1.localdomain.key` | `%ProgramData%\var\lib\icinga2\certs\icinga2-client1.localdomain.key`
  `%ProgramData%\etc\icinga2\pki\ca.crt`                          | `%ProgramData%\var\lib\icinga2\certs\ca.crt`


> **Note**
>
> The default expected path for client certificates is `/var/lib/icinga2/certs/ + NodeName + {.crt,.key}`.
> The `NodeName` constant is usually the FQDN and certificate common name (CN). Check the [conventions](06-distributed-monitoring.md#distributed-monitoring-conventions)
> section inside the Distributed Monitoring chapter.

The [setup CLI commands](06-distributed-monitoring.md#distributed-monitoring-setup-master) and the
default [ApiListener configuration](06-distributed-monitoring.md#distributed-monitoring-apilistener)
have been adjusted to these paths too.

The [ApiListener](09-object-types.md#objecttype-apilistener) object attributes `cert_path`, `key_path`
and `ca_path` have been deprecated and removed from the example configuration.

#### Migration Path <a id="upgrading-to-2-8-certificate-paths-migration-path"></a>

> **Note**
>
> Icinga 2 automatically migrates the certificates to the new default location if they
> are configured and detected in `/etc/icinga2/pki`.

During startup, the migration kicks in and ensures to copy the certificates to the new
location. This will also happen if someone updates the certificate files in `/etc/icinga2/pki`
to ensure that the new certificate location always has the latest files.

This has been implemented in the Icinga 2 binary to ensure it works on both Linux/Unix
and the Windows platform.

If you are not using the built-in CLI commands and setup wizards to deploy the client certificates,
please ensure to update your deployment tools/scripts. This mainly affects

* Puppet modules
* Ansible playbooks
* Chef cookbooks
* Salt recipes
* Custom scripts, e.g. Windows Powershell or self-made implementations

In order to support a smooth migration between versions older than 2.8 and future releases,
the built-in certificate migration path is planned to exist as long as the deprecated
`ApiListener` object attributes exist.

You are safe to use the existing configuration paths inside the `api` feature. If you plan your migration,
look at the following example taken from the Director Linux deployment script for clients.

* Ensure that the default certificate path is changed from `/etc/icinga2/pki` to `/var/lib/icinga2/certs`.

```
-ICINGA2_SSL_DIR="${ICINGA2_CONF_DIR}/pki"
+ICINGA2_SSL_DIR="${ICINGA2_STATE_DIR}/lib/icinga2/certs"
```

* Remove the ApiListener configuration attributes.

```
object ApiListener "api" {
-  cert_path = SysconfDir + "/icinga2/pki/${ICINGA2_NODENAME}.crt"
-  key_path = SysconfDir + "/icinga2/pki/${ICINGA2_NODENAME}.key"
-  ca_path = SysconfDir + "/icinga2/pki/ca.crt"
  accept_commands = true
  accept_config = true
}
```

Test the script with a fresh client installation before putting it into production.

> **Tip**
>
> Please support module and script developers in their migration. If you find
> any project which would require these changes, create an issue or a patchset in a PR
> and help them out. Thanks in advance!

### On-Demand Signing and CA Proxy <a id="upgrading-to-2-8-on-demand-signing-ca-proxy"></a>

Icinga 2 v2.8 supports the following features inside the cluster:

* Forward signing requests from clients through a satellite instance to a signing master ("CA Proxy").
* Signing requests without a ticket. The master instance allows to list and sign CSRs ("On-Demand Signing").

In order to use these features, **all instances must be upgraded to v2.8**.

More details in [this chapter](06-distributed-monitoring.md#distributed-monitoring-setup-sign-certificates-master).

### Windows Client <a id="upgrading-to-2-8-windows-client"></a>

Windows versions older than Windows 10/Server 2016 require the [Universal C Runtime for Windows](https://support.microsoft.com/en-us/help/2999226/update-for-universal-c-runtime-in-windows).

### Removed Bottom Up Client Mode <a id="upgrading-to-2-8-removed-bottom-up-client-mode"></a>

This client mode was deprecated in 2.6 and was removed in 2.8.

The node CLI command does not provide `list` or `update-config` anymore.

> **Note**
>
> The old migration guide can be found on [GitHub](https://github.com/Icinga/icinga2/blob/v2.7.0/doc/06-distributed-monitoring.md#bottom-up-migration-to-top-down-).

The clients don't need to have a local `conf.d` directory included.

Icinga 2 continues to run with the generated and imported configuration.
You are advised to [migrate](https://github.com/Icinga/icinga2/issues/4798)
any existing configuration to the "top down" mode with the help of the
Icinga Director or config management tools such as Puppet, Ansible, etc.


### Removed Classic UI Config Package <a id="upgrading-to-2-8-removed-classicui-config-package"></a>

The config meta package `classicui-config` and the configuration files
have been removed. You need to manually configure
this legacy interface. Create a backup of the configuration
before upgrading and re-configure it afterwards.


### Flapping Configuration <a id="upgrading-to-2-8-flapping-configuration"></a>

Icinga 2 v2.8 implements a new flapping detection algorithm which splits the
threshold configuration into low and high settings.

`flapping_threshold` is deprecated and does not have any effect when flapping
is enabled. Please remove `flapping_threshold` from your configuration. This
attribute will be removed in v2.9.

Instead you need to use the `flapping_threshold_low` and `flapping_threshold_high`
attributes. More details can be found [here](08-advanced-topics.md#check-flapping).

### Deprecated Configuration Attributes <a id="upgrading-to-2-8-deprecated-configuration"></a>

  Object        | Attribute
  --------------|------------------
  ApiListener   | cert\_path (migration happens)
  ApiListener   | key\_path (migration happens)
  ApiListener   | ca\_path (migration happens)
  Host, Service | flapping\_threshold (has no effect)

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

If you want to upgrade an existing Icinga 2 instance, check the
`/usr/share/icinga2-ido-mysql/schema/upgrade` directory for incremental schema upgrade file(s).

> **Note**
>
> If there isn't an upgrade file for your current version available, there's nothing to do.

Apply all database schema upgrade files incrementally.

```
# mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/<version>.sql
```

The Icinga 2 DB IDO feature checks the required database schema version on startup
and generates an log message if not satisfied.


**Example:** You are upgrading Icinga 2 from version `2.4.0` to `2.8.0`. Look into
the `upgrade` directory:

```
$ ls /usr/share/icinga2-ido-mysql/schema/upgrade/
2.0.2.sql 2.1.0.sql 2.2.0.sql 2.3.0.sql 2.4.0.sql 2.5.0.sql 2.6.0.sql 2.8.0.sql
```

There are two new upgrade files called `2.5.0.sql`, `2.6.0.sql` and `2.8.0.sql`
which must be applied incrementally to your IDO database.

```
# mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/2.5.0.sql
# mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/2.6.0.sql
# mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/2.8.0.sql
```

## Upgrading the PostgreSQL database <a id="upgrading-postgresql-db"></a>

If you want to upgrade an existing Icinga 2 instance, check the
`/usr/share/icinga2-ido-mysql/schema/upgrade` directory for incremental schema upgrade file(s).

> **Note**
>
> If there isn't an upgrade file for your current version available, there's nothing to do.

Apply all database schema upgrade files incrementally.

```
# export PGPASSWORD=icinga
# psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/<version>.sql
```

The Icinga 2 DB IDO feature checks the required database schema version on startup
and generates an log message if not satisfied.

**Example:** You are upgrading Icinga 2 from version `2.4.0` to `2.8.0`. Look into
the `upgrade` directory:

```
$ ls /usr/share/icinga2-ido-pgsql/schema/upgrade/
2.0.2.sql 2.1.0.sql 2.2.0.sql 2.3.0.sql 2.4.0.sql 2.5.0.sql 2.6.0.sql 2.8.0.sql
```

There are two new upgrade files called `2.5.0.sql`, `2.6.0.sql` and `2.8.0.sql`
which must be applied incrementally to your IDO database.

```
# export PGPASSWORD=icinga
# psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/2.5.0.sql
# psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/2.6.0.sql
# psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/2.8.0.sql
```
