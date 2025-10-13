# Upgrading Icinga 2 <a id="upgrading-icinga-2"></a>

Upgrading Icinga 2 is usually quite straightforward.
Ordinarily the only manual steps involved
are scheme updates for the IDO database.

Specific version upgrades are described below. Please note that version
updates are incremental. An upgrade from v2.6 to v2.8 requires to
follow the instructions for v2.7 too.

## Upgrading to v2.14.7 <a id="upgrading-to-2-14-7"></a>

This version includes a fix to the logrotate configuration in `/etc/logrotate.d/icinga2`. As this file is tracked as a
configuration file by package managers, it may not be updated automatically if it was modified locally. After upgrading,
make sure to check if there are any files with an extension like `.dpkg-dist` or `.rpmnew` next to it. If so, you need
to incorporate the changes into your configuration manually.

To verify that the fix was applied correctly, check the contents of `/etc/logrotate.d/icinga2`: If the file uses the
command `"$DAEMON" internal signal --sig SIGHUP --pid "$pid"` (instead of `kill -HUP "$pid"`), it was upgraded
correctly.

## Upgrading to v2.14 <a id="upgrading-to-2-14"></a>

### Dependencies and Redundancy Groups <a id="upgrading-to-2-14-dependencies"></a>

Before Icinga v2.12 all dependencies were cumulative.
I.e. the child was considered reachable only if no dependency was violated.
In v2.12 and v2.13, all dependencies were redundant.
I.e. the child was considered unreachable only if no dependency was fulfilled.

v2.14 restores the pre-v2.12 behavior, but allows to override it.
I.e. you can still make any number of your dependencies redundant, as you wish.
For details read the docs' [redundancy groups section](03-monitoring-basics.md#dependencies-redundancy-groups).

### Email Notification Scripts <a id="upgrading-to-2-14-email-notification"></a>

The email notification scripts shipped with Icinga 2 (/etc/icinga2/scripts)
now link to Icinga DB Web, not the monitoring module.
Both new and existing installations are affected unless you've altered the scripts.

In the latter case package managers won't upgrade those "config" files in-place,
but just put files with similar names into the same directory.
This allows you to patch them by yourself based on diff(1).

On the other hand, if you want to stick to the monitoring module for now,
add any comments to the notification scripts before upgrading.
This way package managers won't touch those files.

## Upgrading to v2.13 <a id="upgrading-to-2-13"></a>

### DB IDO Schema Update <a id="upgrading-to-2-13-db-ido"></a>

There is an optional schema update on MySQL which increases the max length of object names from 128 to 255 characters.

Please proceed here for the [MySQL upgrading docs](16-upgrading-icinga-2.md#upgrading-mysql-db).

### Behavior changes <a id="upgrading-to-2-13-behavior-changes"></a>

#### Deletion of child downtimes on services

Service downtimes created while using the `all_services` flag on the [schedule-downtime](12-icinga2-api.md#schedule-downtime) API action
will now automatically be deleted when deleting the hosts downtime.

#### Windows Event Log

Icinga 2.13 now supports logging to the Windows Event Log. Icinga will now also log messages from the early
startup phase to the Windows Event Log. These were previously missing from the log file and you could only
see them by manually starting Icinga in the foreground.

This feature is now enabled and replaces the existing mainlog feature logging to a file. When upgrading, the installer
will enable the windowseventlog feature and disable the mainlog feature. Logging to a file is still possible.
If you don't want this configuration migration on upgrade, you can opt-out by installing
the `%ProgramData%\icinga2\etc\icinga2\features-available\windowseventlog.conf` file before upgrading to Icinga 2.13.

#### Broken API package name validation

This version has replaced a broken regex in the API package validation code which results in package names
now being validated correctly. Package names should now only consist of alphanumeric characters, dashes and underscores.

This change only applies to newly created packages to support already existing ones.

## Upgrading to v2.12 <a id="upgrading-to-2-12"></a>

* CLI
    * New `pki verify` CLI command for better [TLS certificate troubleshooting](15-troubleshooting.md#troubleshooting-certificate-verification)

### Behavior changes <a id="upgrading-to-2-12-behavior-changes"></a>

The behavior of multi parent [dependencies](03-monitoring-basics.md#dependencies) was fixed to e.g. render hosts unreachable when both router uplinks are down.

Previous behaviour:

1) parentHost1 DOWN, parentHost2 UP => childHost **not reachable**
2) parentHost1 DOWN, parentHost2 DOWN => childHost **not reachable**

New behavior:

1) parentHost1 DOWN, parentHost2 UP => childHost **reachable**
2) parentHost1 DOWN, parentHost2 DOWN => childHost **not reachable**

Please review your [Dependency](09-object-types.md#objecttype-dependency) configuration as 1) may lead to
different results for

- `last_reachable` via REST API query
- Notifications not suppressed by faulty reachability calculation anymore

### Breaking changes <a id="upgrading-to-2-12-breaking-changes"></a>

As of v2.12 our [API](12-icinga2-api.md) URL endpoint [`/v1/actions/acknowledge-problem`](12-icinga2-api.md#icinga2-api-actions-acknowledge-problem) refuses acknowledging an already acknowledged checkable by overwriting the acknowledgement.
To replace an acknowledgement you have to remove the old one before adding the new one.

The deprecated parameters `--cert` and `--key` for the `pki save-cert` CLI command
have been removed from the command and documentation.

## Upgrading to v2.11 <a id="upgrading-to-2-11"></a>

### Bugfixes for 2.11 <a id="upgrading-to-2-11-bugfixes"></a>

2.11.1 on agents/satellites fixes a problem where 2.10.x as config master would send out an unwanted config marker file,
thus rendering the agent to think it is autoritative for the config, and never accepting any new
config files for the zone(s). **If your config master is 2.11.x already, you are not affected by this problem.**

In order to fix this, upgrade to at least 2.11.1, and purge away the local config sync storage once, then restart.

```bash
yum install icinga2

rm -rf /var/lib/icinga2/api/zones/*
rm -rf /var/lib/icinga2/api/zones-stage/*

systemctl restart icinga2
```

2.11.2 fixes a problem where the newly introduced config sync "check-change-then-reload" functionality
could cause endless reload loops with agents. The most visible parts are failing command endpoint checks
with "not connected" UNKNOWN state. **Only applies to HA enabled zones with 2 masters and/or 2 satellites.**

In order to fix this, upgrade all agents/satellites to at least 2.11.2 and restart them.

### Packages <a id="upgrading-to-2-11-packages"></a>

EOL distributions where no packages are available with this release:

* SLES 11
* Ubuntu 14 LTS
* RHEL/CentOS 6 x86

Raspbian Packages are available inside the `icinga-buster` repository
on [https://packages.icinga.com](https://packages.icinga.com/raspbian/).
Please note that Stretch is not supported suffering from compiler
regressions. Upgrade to Raspbian Buster is highly recommended.

#### Added: Boost 1.66+

The rewrite of our core network stack for cluster and REST API
requires newer Boost versions, specifically >= 1.66. For technical
details, please continue reading in [this issue](https://github.com/Icinga/icinga2/issues/7041).

Distribution         | Repository providing Boost Dependencies
---------------------|-------------------------------------
CentOS 7             | [EPEL repository](02-installation.md#centos-repository)
RHEL 7               | [EPEL repository](02-installation.md#rhel-repository)
RHEL/CentOS 6 x64    | [packages.icinga.com](https://packages.icinga.com)
Fedora               | Fedora Upstream
Debian 10 Buster     | Debian Upstream
Debian 9 Stretch     | [Backports repository](02-installation.md#debian-backports-repository) **New since 2.11**
Debian 8 Jessie      | [packages.icinga.com](https://packages.icinga.com)
Ubuntu 18 Bionic     | [packages.icinga.com](https://packages.icinga.com)
Ubuntu 16 Xenial     | [packages.icinga.com](https://packages.icinga.com)
SLES 15              | SUSE Upstream
SLES 12              | [packages.icinga.com](https://packages.icinga.com) (replaces the SDK repository requirement)

On platforms where EPEL or Backports cannot satisfy this dependency,
we provide Boost as package on our [package repository](https://packages.icinga.com)
for your convenience.

After upgrade, you may remove the old Boost packages (1.53 or anything above)
if you don't need them anymore.

#### Added: .NET Framework 4.6

We modernized the graphical Windows wizard to use the more recent .NET Framework 4.6. This requires that Windows versions
older than Windows 10/Windows Server 2016 installs at least [.NET Framework 4.6](https://www.microsoft.com/en-US/download/details.aspx?id=53344). Starting with Windows 10/Windows Server 2016 a .NET Framework 4.6 or higher is installed by default.

The MSI-Installer package checks if the .NET Framework 4.6 or higher is present, if not the installation wizard will abort with an error message telling you to install at least .NET Framework 4.6.

#### Removed: YAJL

Our JSON library, namely [YAJL](https://github.com/lloyd/yajl), isn't maintained anymore
and may cause [crashes](https://github.com/Icinga/icinga2/issues/6684).

It is replaced by [JSON for Modern C++](https://github.com/nlohmann/json) by Niels Lohmann
and compiled into the binary as header only include. It helps our way to C++11 and allows
to fix additional UTF8 issues more easily. Read more about its [design goals](https://github.com/nlohmann/json#design-goals)
and [benchmarks](https://github.com/miloyip/nativejson-benchmark#parsing-time).

### Core <a id="upgrading-to-2-11-core"></a>

#### Reload Handling <a id="upgrading-to-2-11-core-reload-handling"></a>

2.11 provides fixes for unwanted notifications during restarts.
The updated systemd service file now uses the `KillMode=mixed` setting.

The reload handling was improved with an umbrella process, which means
that normal runtime operations include **3 processes**. You may need to
adjust the local instance monitoring of the [procs](08-advanced-topics.md#monitoring-icinga) check.

More details can be found in the [technical concepts](19-technical-concepts.md#technical-concepts-core-reload) chapter.

#### Downtime Notifications <a id="upgrading-to-2-11-core-downtime-notifications"></a>

Imagine that a host/service changes to a HARD NOT-OK state,
and its check interval is set to a high interval e.g. 1 hour.

A maintenance downtime prevents the notification being sent,
but once it ends and the host/service is still in a downtime,
no immediate notification is re-sent but you'll have to wait
for the next check.

Another scenario is with one-shot notifications (interval=0)
which would never notify again after the downtime ends and
the problem state being intact. The state change logic requires
to recover and become HARD NOT-OK to notify again.

In order to solve these problems with filtered/suppressed notifications
in downtimes, v2.11 changes the behaviour like this:

- If there was a notification suppressed in a downtime, the core stores that information
- Once the downtime ends and the problem state is still intact, Icinga checks whether a re-notification should be sent immediately

A new cluster message was added to keep this in sync amongst HA masters.

> **Important**
>
> In order to properly use this new feature, all involved endpoints
> must be upgraded to v2.11.

### Network Stack <a id="upgrading-to-2-11-network-stack"></a>

The core network stack has been rewritten in 2.11 (some say this could be Icinga 3).

You can read the full story [here](https://github.com/Icinga/icinga2/issues/7041).

The only visible changes for users are:

- No more dead-locks with hanging TLS connections (Cluster, REST API)
- Better log messages in error cases
- More robust and stable with using external libraries instead of self-written socket I/O

Coming with this release, we've also updated TLS specific requirements
explained below.

#### TLS 1.2 <a id="upgrading-to-2-11-network-stack-tls-1-2"></a>

v2.11 raises the minimum required TLS version to 1.2.
This is available since OpenSSL 1.0.1 (EL6 & Debian Jessie).

Older Icinga satellites/agents need to support TLS 1.2 during the TLS
handshake.

The `api` feature attribute `tls_protocolmin` now only supports the
value `TLSv1.2` being the default.

#### Hardened Cipher List <a id="upgrading-to-2-11-network-stack-cipher-list"></a>

The previous default cipher list allowed weak ciphers. There's no sane way
other than explicitly setting the allowed ciphers.

The new default sets this to:

```
ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:AES256-GCM-SHA384:AES128-GCM-SHA256
```

You can override this setting in the [api](09-object-types.md#objecttype-apilistener)
feature with the `cipher_list` attribute.

In case that one of these ciphers is marked as insecure in the future,
please let us know with an issue on GitHub.

### Cluster <a id="upgrading-to-2-11-cluster"></a>

#### Agent Hosts with Command Endpoint require a Zone <a id="upgrading-to-2-11-cluster-agent-hosts-command-endpoint-zone"></a>

2.11 fixes bugs where agent host checks would never be scheduled on
the master. One definite requirement is that the checkable host/service
is put into a zone.

By default, the Director puts the agent host in `zones.d/master`
and you're good to go. If you manually manage the configuration,
the config compiler now throws an error with `command_endpoint`
being set but no `zone` defined.

The most convenient way with e.g. managing the objects in `conf.d`
is to move them into the `master` zone. Please continue in the
[troubleshooting docs](15-troubleshooting.md#troubleshooting-cluster-command-endpoint-errors-agent-hosts-command-endpoint-zone)
for further instructions.

#### Config Sync <a id="upgrading-to-2-11-cluster-config-sync"></a>

2.11 overhauls the cluster config sync in many ways. This includes the
following under the hood:

- Synced configuration files are not immediately put into production, but left inside a stage.
- Unsuccessful config validation never puts the config into production, additional logging and API states are available.
- Zone directories which are not configured in zones.conf, are not included anymore on secondary master/satellites/clients.
- Synced config change calculation use checksums instead of timestamps to trigger validation/reload. This is more safe, and the usage of timestamps is now deprecated.
- Don't allow parallel cluster syncs to avoid race conditions with overridden files.
- Deleted directories and files are now purged, previous versions had a bug.

Whenever a newer child endpoint receives a configuration update without
checksums, it will log a warning.

```
Received configuration update without checksums from parent endpoint satellite1. This behaviour is deprecated. Please upgrade the parent endpoint to 2.11+
```

This is a gentle reminder to upgrade the master and satellites first,
prior to installing new clients/agents.

Technical details are available in the [technical concepts](19-technical-concepts.md#technical-concepts-cluster-config-sync) chapter.

Since the config sync change detection now uses checksums, this may fail
with anything else than syncing configuration text files. Syncing binary
files were never supported, but rumors say that some users do so.

This is now prohibited and logged.

```
[2019-08-02 16:03:19 +0200] critical/ApiListener: Ignoring file '/etc/icinga2/zones.d/global-templates/forbidden.exe' for cluster config sync: Does not contain valid UTF8. Binary files are not supported.
Context:
	(0) Creating config update for file '/etc/icinga2/zones.d/global-templates/forbidden.exe'
	(1) Activating object 'api' of type 'ApiListener'
```

Such binaries wrapped into JSON-RPC cluster messages may always cause changes
and trigger reload loops. In order to prevent such harm in production,
use infrastructure tools such as Foreman, Puppet, Ansible, etc. to install
plugins on the masters, satellites and agents.

##### Config Sync: Zones in Zones <a id="upgrading-to-2-11-cluster-config-sync-zones-in-zones"></a>

The cluster config sync works in the way that configuration
put into `/etc/icinga2/zones.d` only is included when configured
outside in `/etc/icinga2/zones.conf`.

If you for example create a "Zone Inception" with defining the
`satellite` zone in `zones.d/master`, the config compiler does not
re-run and include this zone config recursively from `zones.d/satellite`.

Since v2.11, the config compiler is only including directories where a
zone has been configured. Otherwise it would include renamed old zones,
broken zones, etc. and those long-lasting bugs have been now fixed.

Please consult the [troubleshoot docs](15-troubleshooting.md#troubleshooting-cluster-config-zones-in-zones)
for concrete examples and solutions.

#### HA-aware Features <a id="upgrading-to-2-11-cluster-ha-aware-features"></a>

v2.11 introduces additional HA functionality similar to the DB IDO feature.
This enables the feature being active only on one endpoint while the other
endpoint is paused. When one endpoint is shut down, automatic failover happens.

This feature is turned off by default keeping the current behaviour. If you need
it active on just one endpoint, set `enable_ha = true` on both endpoints in the
feature configuration.

This affects the following features:

* [Elasticsearch](09-object-types.md#objecttype-elasticsearchwriter)
* [Gelf](09-object-types.md#objecttype-gelfwriter)
* [Graphite](09-object-types.md#objecttype-graphitewriter)
* [InfluxDB](09-object-types.md#objecttype-influxdbwriter)
* [OpenTsdb](09-object-types.md#objecttype-opentsdbwriter)
* [Perfdata](09-object-types.md#objecttype-perfdatawriter) (for PNP)

### HA Failover <a id="upgrading-to-2-11-ha-failover"></a>

The reconnect failover has been improved, and the default `failover_timeout`
for the DB IDO features has been lowered from 60 to 30 seconds.
Object authority updates (required for balancing in the cluster) happen
more frequenty (was 30, is 10 seconds).
Also the cold startup without object authority updates has been reduced
from 60 to 30 seconds. This is to allow cluster reconnects (lowered from 60s to 10s in 2.10)
before actually considering a failover/split brain scenario.

The [IdoMysqlConnection](09-object-types.md#objecttype-idomysqlconnection) and [IdoPgsqlConnection](09-object-types.md#objecttype-idopgsqlconnection)
objects provide a new attribute named `last_failover` which shows the last failover timestamp.
This value also is available in the [ido](10-icinga-template-library.md#itl-icinga-ido) CheckCommand output.


### CLI Commands <a id="upgrading-to-2-11-cli-commands"></a>

The `troubleshoot` CLI command has been removed. It was never completed,
and turned out not to provide required details for GitHub issues anyways.

We didn't ask nor endorse users on GitHub/Discourse in the past 2 years, so
we're removing it without deprecation.

Issue templates, the troubleshooting docs and support knowledge has
proven to be better.

#### Permissions <a id="upgrading-to-2-11-cli-commands-permissions"></a>

CLI commands such as `api setup`, `node wizard/setup`, `feature enable/disable/list`
required root permissions previously. Since the file permissions allow
the Icinga user to change things already, and users kept asking to
run Icinga on their own webspace without root permissions, this is now possible
with 2.11.

If you are running the commands with a different user than the
compiled `ICINGA_USER` and `ICINGA_GROUP` CMake settings (`icinga` everywhere,
except Debian with `nagios` for historical reasons), ensure that this
user has the capabilities to change to a different user.

If you still encounter problems, run the aforementioned CLI commands as root,
or with sudo.

#### CA List Behaviour Change <a id="upgrading-to-2-11-cli-commands-ca-list"></a>

`ca list` only shows the pending certificate signing requests by default.

You can use the new `--all` parameter to show all signing requests.
Note that Icinga automatically purges signed requests older than 1 week.

#### New: CA Remove/Restore <a id="upgrading-to-2-11-cli-commands-ca-remove-restore"></a>

`ca remove` allows you to remove pending signing requests. Once the
master receives a CSR, and it is marked as removed, the request is
denied.

`ca restore` allows you to restore a removed signing request. You
can list removed signing requests with the new `--removed` parameter
for `ca list`.

### Configuration <a id="upgrading-to-2-11-configuration"></a>

The deprecated `concurrent_checks` attribute in the [checker feature](09-object-types.md#objecttype-checkercomponent)
has no effect anymore if set. Please use the [MaxConcurrentChecks](17-language-reference.md#icinga-constants-global-config)
constant in [constants.conf](04-configuration.md#constants-conf) instead.

### REST API <a id="upgrading-to-2-11-api"></a>

#### Actions <a id="upgrading-to-2-11-api-actions"></a>

The [schedule-downtime](12-icinga2-api.md#icinga2-api-actions-schedule-downtime-host-all-services)
action supports the `all_services` parameter for Host types. Defaults to false.

#### Config Packages <a id="upgrading-to-2-11-api-config-packages"></a>

Deployed configuration packages require an active stage, with many previous
allowed. This mechanism is used by the Icinga Director as external consumer,
and Icinga itself for storing runtime created objects inside the `_api`
package.

This includes downtimes and comments, which where sometimes stored in the wrong
directory path, because the active-stage file was empty/truncated/unreadable at
this point.

2.11 makes this mechanism more stable and detects broken config packages.
It will also attempt to fix them, the following log entry is perfectly fine.

```
[2019-05-10 12:12:09 +0200] information/ConfigObjectUtility: Repairing config package '_api' with stage 'dbe0bef8-c72c-4cc9-9779-da7c4527c5b2'.
```

If you still encounter problems, please follow [this troubleshooting entry](15-troubleshooting.md#troubleshooting-api-missing-runtime-objects).

### DB IDO MySQL Schema <a id="upgrading-to-2-11-db-ido"></a>

The schema for MySQL contains an optional update which
drops unneeded indexes. You don't necessarily need to apply
this update.

### Documentation <a id="upgrading-to-2-11-documentation"></a>

* `Custom attributes` have been renamed to `Custom variables` following the name `vars` and their usage in backends and web interfaces.
The term `custom attribute` still applies, but referring from the web to the core docs is easier.
* The distributed environment term `client` has been refined into `agent`. Wordings and images have been adjusted, and a `client` only is used as
general term when requesting something from a parent server role.
* The images for basics, modes and scenarios in the distributed monitoring chapter have been re-created from scratch.
* `02-getting-started.md` was renamed to `02-installation.md`, `04-configuring-icinga-2.md` into `04-configuration.md`. Apache redirects will be in place.

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
because systemd [does not support](https://github.com/systemd/systemd/issues/2123)
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

Restart Icinga 2 afterwards, the systemd service file automatically puts the
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
  `/etc/icinga2/pki/icinga2-agent1.localdomain.crt` | `/var/lib/icinga2/certs/icinga2-agent1.localdomain.crt`
  `/etc/icinga2/pki/icinga2-agent1.localdomain.key` | `/var/lib/icinga2/certs/icinga2-agent1.localdomain.key`
  `/etc/icinga2/pki/ca.crt`                          | `/var/lib/icinga2/certs/ca.crt`

This applies to Windows clients in the same way: `%ProgramData%\etc\icinga2\pki`
was moved to `%ProgramData%\var\lib\icinga2\certs`.

  Old Path                                                        | New Path
  ----------------------------------------------------------------|----------------------------------------------------------------
  `%ProgramData%\etc\icinga2\pki\icinga2-agent1.localdomain.crt` | `%ProgramData%\var\lib\icinga2\certs\icinga2-agent1.localdomain.crt`
  `%ProgramData%\etc\icinga2\pki\icinga2-agent1.localdomain.key` | `%ProgramData%\var\lib\icinga2\certs\icinga2-agent1.localdomain.key`
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

```bash
mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/2.5.0.sql
mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/2.6.0.sql
mysql -u root -p icinga < /usr/share/icinga2-ido-mysql/schema/upgrade/2.8.0.sql
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

```bash
export PGPASSWORD=icinga
psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/2.5.0.sql
psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/2.6.0.sql
psql -U icinga -d icinga < /usr/share/icinga2-ido-pgsql/schema/upgrade/2.8.0.sql
```
