# Upgrading Icinga 2 <a id="upgrading-icinga-2"></a>

Upgrading Icinga 2 is usually quite straightforward. Ordinarily the only manual steps involved
are scheme updates for the IDO database.

Specific version upgrades are described below. Please note that version
updates are incremental. An upgrade from v2.6 to v2.8 requires to
follow the instructions for v2.7 too.

## Upgrading to v2.11 <a id="upgrading-to-2-11"></a>

### HA-aware Features <a id="upgrading-to-2-11-ha-aware-features"></a>

v2.11 introduces additional HA functionality similar to the DB IDO feature.
This enables the feature being active only on one endpoint while the other
endpoint is paused. When one endpoint is shut down, automatic failover happens.

This feature is turned on by default. If you need one of the features twice,
please use `enable_ha = false` to restore the old behaviour.

This affects the following features:

* [Elasticsearch](09-object-types.md#objecttype-elasticsearchwriter)
* [Gelf](09-object-types.md#objecttype-gelfwriter)
* [Graphite](09-object-types.md#objecttype-graphitewriter)
* [InfluxDB](09-object-types.md#objecttype-influxdbwriter)
* [OpenTsdb](09-object-types.md#objecttype-opentsdbwriter)
* [Perfdata](09-object-types.md#objecttype-perfdatawriter) (for PNP)

## Upgrading to v2.10 <a id="upgrading-to-2-10"></a>

### Path Constant Changes <a id="upgrading-to-2-10-path-constant-changes"></a>

During package upgrades you may see a notice that the configuration
content of features has changed. This is due to a more general approach
with path constants in v2.10.

The known constants `SysconfDir` and `LocalStateDir` stay intact and won't
break on upgrade.
If you are using these constants in your own custom command definitions
or other objects, you are advised to revise them and update them according
to the [documentation](17-language-reference.md#icinga-constants).

Example diff:

```
object NotificationCommand "mail-service-notification" {
-  command = [ SysconfDir + "/icinga2/scripts/mail-service-notification.sh" ]
+  command = [ ConfigDir + "/scripts/mail-service-notification.sh" ]
```

If you have the `ICINGA2_RUN_DIR` environment variable configured in the
sysconfig file, you need to rename it to `ICINGA2_INIT_RUN_DIR`. `ICINGA2_STATE_DIR`
has been removed and this setting has no effect.

> **Note**
>
> This is important if you rely on the sysconfig configuration in your own scripts.

### New Constants <a id="upgrading-to-2-10-path-new-constants"></a>

New [Icinga constants](17-language-reference.md#icinga-constants) have been added in this release.

* `Environment` for specifying the Icinga environment. Defaults to not set.
* `ApiBindHost` and `ApiBindPort` to allow overriding the default ApiListener values. This will be used for an Icinga addon only.

### Configuration: Namespaces <a id="upgrading-to-2-10-configuration-namespaces"></a>

The keywords `namespace` and `using` are now [reserved](17-language-reference.md#reserved-keywords) for the namespace functionality provided
with v2.10. Read more about how it works [here](17-language-reference.md#namespaces).

### Configuration: ApiListener <a id="upgrading-to-2-10-configuration-apilistener"></a>

Anonymous JSON-RPC connections in the cluster can now be configured with `max_anonymous_clients`
attribute.
The corresponding REST API results from `/v1/status/ApiListener` in `json_rpc` have been renamed
from `clients` to `anonymous_clients` to better reflect their purpose. Authenticated clients
are counted as connected endpoints. A similar change is there for the performance data metrics.

The TLS handshake timeout defaults to 10 seconds since v2.8.2. This can now be configured
with the configuration attribute `tls_handshake_timeout`. Beware of performance issues
with setting a too high value.

### API: schedule-downtime Action <a id="upgrading-to-2-10-api-schedule-downtime-action"></a>

The attribute `child_options` was previously accepting 0,1,2 for specific child downtime settings.
This behaviour stays intact, but the new proposed way are specific constants as values (`DowntimeNoChildren`, `DowntimeTriggeredChildren`, `DowntimeNonTriggeredChildren`).

### Notifications: Recovery and Acknowledgement <a id="upgrading-to-2-10-notifications"></a>

When a user should be notified on `Problem` and `Acknowledgement`, v2.10 now checks during
the `Acknowledgement` notification event whether this user has been notified about a problem before.

```
  types = [ Problem, Acknowledgement, Recovery ]
```

If **no** `Problem` notification was sent, and the types filter includes problems for this user,
the `Acknowledgement` notification is **not** sent.

In contrast to that, the following configuration always sends `Acknowledgement` notifications.

```
  types = [ Acknowledgement, Recovery ]
```

This change also restores the old behaviour for `Recovery` notifications. The above configuration
leaving out the `Problem` type can be used to only receive recovery notifications. If `Problem`
is added to the types again, Icinga 2 checks whether it has notified a user of a problem when
sending the recovery notification.

More details can be found in [this PR](https://github.com/Icinga/icinga2/pull/6527).

### Stricter configuration validation

Some config errors are now fatal. While it never worked before, icinga2 refuses to start now!

For example the following started to give a fatal error in 2.10:

```
  object Zone "XXX" {
    endpoints = [ "master-server" ]
    parent = "global-templates"
  }
```

```critical/config: Error: Zone 'XXX' can not have a global zone as parent.```

### Package Changes <a id="upgrading-to-2-10-package-changes"></a>

Debian/Ubuntu drops the `libicinga2` package. `apt-get upgrade icinga2`
won't remove such packages leaving the upgrade in an unsatisfied state.

Please use `apt-get full-upgrade` or `apt-get dist-upgrade` instead, as explained [here](https://github.com/Icinga/icinga2/issues/6695#issuecomment-430585915).

On RHEL/CentOS/Fedora, `icinga2-libs` has been obsoleted. Unfortunately yum's dependency
resolver doesn't allow to install older versions than 2.10 then. Please
read [here](https://github.com/Icinga/icinga-packaging/issues/114#issuecomment-429264827)
for details.

RPM packages dropped the [Classic UI](16-upgrading-icinga-2.md#upgrading-to-2-8-removed-classicui-config-package)
package in v2.8, Debian/Ubuntu packages were forgotten. This is now the case with this
release. Icinga 1.x is EOL by the end of 2018, plan your migration to [Icinga Web 2](https://icinga.com/docs/icingaweb2/latest/).

## Upgrading to v2.9 <a id="upgrading-to-2-9"></a>

### Deprecation and Removal Notes <a id="upgrading-to-2-9-deprecation-removal-notes"></a>

- Deprecation of 1.x compatibility features: `StatusDataWriter`, `CompatLogger`, `CheckResultReader`. Their removal is scheduled for 2.11.
Icinga 1.x is EOL and will be out of support by the end of 2018.
- Removal of Icinga Studio. It always has been experimental and did not satisfy our high quality standards. We've therefore removed it.

### Sysconfig Changes <a id="upgrading-to-2-9-sysconfig-changes"></a>

The security fixes in v2.8.2 required moving specific runtime settings
into the Sysconfig file and environment. This included that Icinga 2
would itself parse this file and read the required variables. This has generated
numerous false-positive log messages and led to many support questions. v2.9.0
changes this in the standard way to read these variables from the environment, and use
sane compile-time defaults.

> **Note**
>
> In order to upgrade, remove everything in the sysconfig file and re-apply
> your changes.

There is a bug with existing sysconfig files where path variables are not expanded
because Systemd [does not support](https://github.com/systemd/systemd/issues/2123)
shell variable expansion. This worked with SysVInit though.

Edit the sysconfig file and either remove everything, or edit this line
on RHEL 7. Modify the path for other distributions.

```
vim /etc/sysconfig/icinga2

-ICINGA2_PID_FILE=$ICINGA2_RUN_DIR/icinga2/icinga2.pid
+ICINGA2_PID_FILE=/run/icinga2/icinga2.pid
```

If you want to adjust the number of open files for the Icinga application
for example, you would just add this setting like this on RHEL 7:

```
vim /etc/sysconfig/icinga2

ICINGA2_RLIMIT_FILES=50000
```

Restart Icinga 2 afterwards, the Systemd service file automatically puts the
value into the application's environment where this is read on startup.

### Setup Wizard Changes <a id="upgrading-to-2-9-setup-wizard-changes"></a>

Client and satellite setups previously had the example configuration in `conf.d` included
by default. This caused trouble on config sync, or with locally executed checks generating
wrong check results for command endpoint clients.

In v2.9.0 `node wizard`, `node setup` and the graphical Windows wizard will disable
the inclusion by default. You can opt-out and explicitly enable it again if needed.

In addition to the default global zones `global-templates` and `director-global`,
the setup wizards also offer to specify your own custom global zones and generate
the required configuration automatically.

The setup wizards also use full qualified names for Zone and Endpoint object generation,
either the default values (FQDN for clients) or the user supplied input. This removes
the dependency on the `NodeName` and `ZoneName` constant and helps to immediately see
the parent-child relationship. Those doing support will also see the benefit in production.

### CLI Command Changes <a id="upgrading-to-2-9-cli-changes"></a>

The [node setup](06-distributed-monitoring.md#distributed-monitoring-automation-cli-node-setup)
parameter `--master_host` was deprecated and replaced with `--parent_host`.
This parameter is now optional to allow connection-less client setups similar to the `node wizard`
CLI command. The `parent_zone` parameter has been added to modify the parent zone name e.g.
for client-to-satellite setups.

The `api user` command which was released in v2.8.2 turned out to cause huge problems with
configuration validation, windows restarts and OpenSSL versions. It is therefore removed in 2.9,
the `password_hash` attribute for the ApiUser object stays intact but has no effect. This is to ensure
that clients don't break on upgrade. We will revise this feature in future development iterations.

### Configuration Changes <a id="upgrading-to-2-9-config-changes"></a>

The CORS attributes `access_control_allow_credentials`, `access_control_allow_headers` and
`access_control_allow_methods` are now controlled by Icinga 2 and cannot be changed anymore.

### Unique Generated Names <a id="upgrading-to-2-9-unique-name-changes"></a>

With the removal of RHEL 5 as supported platform, we can finally use real unique IDs.
This is reflected in generating names for e.g. API stage names. Previously it was a handcrafted
mix of local FQDN, timestamps and random numbers.

### Custom Vars not updating <a id="upgrading-to-2-9-custom-vars-not-updating"></a>

A rare issue preventing the custom vars of objects created prior to 2.9.0 being updated when changed may occur. To
remedy this, truncate the customvar tables and restart Icinga 2. The following is an example of how to do this with mysql:

```
$ mysql -uroot -picinga icinga
MariaDB [icinga]> truncate icinga_customvariables;
Query OK, 0 rows affected (0.05 sec)
MariaDB [icinga]> truncate icinga_customvariablestatus;
Query OK, 0 rows affected (0.03 sec)
MariaDB [icinga]> exit
Bye
$ sudo systemctl restart icinga2
```

Custom vars should now stay up to date.


## Upgrading to v2.8.2 <a id="upgrading-to-2-8-2"></a>

With version 2.8.2 the location of settings formerly found in `/etc/icinga2/init.conf` has changed. They are now
located in the sysconfig, `/etc/sysconfig/icinga2` (RPM) or `/etc/default/icinga2` (DPKG) on most systems. The
`init.conf` file has been removed and its settings will be ignored. These changes are only relevant if you edited the
`init.conf`. Below is a table displaying the new names for the affected settings.

 Old `init.conf` | New `sysconfig/icinga2`
 ----------------|------------------------
 RunAsUser       | ICINGA2\_USER
 RunAsGroup      | ICINGA2\_GROUP
 RLimitFiles     | ICINGA2\_RLIMIT\_FILES
 RLimitProcesses | ICINGA2\_RLIMIT\_PROCESSES
 RLimitStack     | ICINGA2\_RLIMIT\_STACK

## Upgrading to v2.8 <a id="upgrading-to-2-8"></a>

### DB IDO Schema Update to 2.8.0 <a id="upgrading-to-2-8-db-ido"></a>

There are additional indexes and schema fixes which require an update.

Please proceed here for [MySQL](16-upgrading-icinga-2.md#upgrading-mysql-db) or [PostgreSQL](16-upgrading-icinga-2.md#upgrading-postgresql-db).

> **Note**
>
> `2.8.1.sql` fixes a unique constraint problem with fresh 2.8.0 installations.
> You don't need this update if you are upgrading from an older version.

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

You are safe to use the existing configuration paths inside the `api` feature.

**Example**

Look at the following example taken from the Director Linux deployment script for clients.

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
update your configuration accordingly. An advisory has been published [here](https://icinga.com/2017/08/23/advisory-for-icinga-2-v2-7-update-and-mail-notification-scripts/).

In case are having troubles with OpenSSL 1.1.0 and the
public CA certificates, please read [this advisory](https://icinga.com/2017/08/30/advisory-for-ssl-problems-with-leading-zeros-on-openssl-1-1-0/)
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
`/usr/share/icinga2-ido-pgsql/schema/upgrade` directory for incremental schema upgrade file(s).

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
