# Icinga 2 CHANGELOG

**The latest release announcements are available on [https://icinga.com/blog/](https://icinga.com/blog/).**

Please read the [upgrading](https://icinga.com/docs/icinga2/latest/doc/16-upgrading-icinga-2/)
documentation before upgrading to a new release.

Released closed milestones can be found on [GitHub](https://github.com/Icinga/icinga2/milestones?state=closed).

## 2.13.4 (2022-06-30)

This release brings the final changes needed for the Icinga DB 1.0 release.
Addtionally, it includes some fixes and a performance improvement resulting
in faster config validation and reload times.

### Bugfixes

* Fix a race-condition involving object attribute updates that could result in a crash. #9395
* After a host recovered, only send problem notifications for services after
  they have been rechecked afterwards to avoid false notifications. #9348
* Speed up config validation by avoiding redundant serialization of objects. #9400
* Add a `separator` attribute to allow using arguments like `--key=value` as required by some
  check plugins. This fixes the `--upgrade` and `--dist-upgrade` arguments of `check_apt`. #9397
* Windows: Update bundled versions of Boost and OpenSSL. #9360 #9415

### Icinga DB

* Add an `icingadb` CheckCommand to allow checking if Icinga DB is healthy. #9417
* Update documentation related to Icinga DB. #9423
* Fix a bug where history events could miss the environment ID. #9396
* Properly serialize attributes of command arguments when explicitly set to `null`. #9398
* Rename some attributes to make the database schema more consistent. #9399 #9419 #9421
* Make the error message more helpful if the API isn't set up #9418

## 2.13.3 (2022-04-14)

This version includes bugfixes for many features of Icinga 2, including fixes for multiple crashes.
It also includes a number of fixes and improvements for Icinga DB.

### API

* The /v1/config/stages endpoint now immediately rejects parallel config updates
  instead of accepting and then later failing to verify and activate them. #9328

### Certificates

* The lifetime of newly issued node certificates is reduced from 15 years to 397 days. #9337
* Compare cluster certificate tickets in constant time. #9333

### Notifications

* Fix a crash that could happen while sending notifications shortly after Icinga 2 started. #9124
* Fix missing or redundant notifications after certain combinations of state changes happened
  while notifications were suppressed, for example during a downtime. #9285

### Checks and Commands

* Fix a deadlock when processing check results for checkables with dependencies. #9228
* Fix a message routing loop that can happen for event commands that are executed within a zone
  using `command_endpoint` that resulted in excessive execution of the command. #9260

### Downtimes

* Fix scheduling of downtimes for all services on child hosts. #9159
* Creating fixed downtimes starting immediately now send a corresponding notification. #9158
* Fix some issues involving daylight saving time changes that could result in an hour missing
  from scheduled downtimes. This fix applies to time periods as well. #9238

### Configuration

* Fix the evaluation order of default templates when used in combination with apply rules.
  Now default templates are imported first as stated in the documentation and
  as it already happens for objects defined without using apply. #9290

### IDO

* Fix an issue where contacts were not written correctly to the notification history
  if multiple IDO instances are active on the same node. #9242
* Explicitly set the encoding for MySQL connections as a workaround for changed defaults
  in Debian bullseye. #9312
* Ship a MySQL schema upgrade that fixes inconsistent version information in the
  full schema file and upgrade files which could have resulted in inaccurate reports
  of an outdated schema version. #9139

### Performance Data Writers

* Fix a race condition in the InfluxDB Writers that could result in a crash. #9237
* Fix a log message where Influxdb2Writer logged as InfluxdbWriter. #9315
* All writers no longer send metrics multiple times after HA failovers. #9322

### Build

* Fix the order of linker flags to fix builds on some ARM platforms. #9164
* Fix a regression introduced in 2.13.2 preventing non-unity builds. #9094
* Fix an issue when building within an unrelated Git repository,
  version information from that repository could incorrectly be used for Icinga 2. #9155
* Windows: Update bundled Boost version to 1.78.0 and OpenSSL to 1.1.1n #9325

### Internals

* Fix some race conditions due to missing synchronization.
  These race conditions should not have caused any practical problems
  besides incorrect numbers in debug log message. #9306
* Move the startup.log and status files created when validating incoming cluster config updates
  to /var/lib/icinga2/api and always keep the last failed startup.log to ease debugging. #9335

### Icinga DB

* The `severity` attribute was updated to match the sort order Icinga Web 2 uses for the IDO.
  The documentation for this attribute was already incorrect before and was updated
  to reflect the current functionality. #9239 #9240
* Fix the `is_sticky` attribute for comments. #9303
* Fix missing updates of `is_reachable` and `severity` in the state tables. #9241
* Removing an acknowledgement no longer incorrectly writes comment history. #9302
* Fix multiple issues so that in an HA zone, both nodes now write consistent history. #9157 #9182 #9190
* Fix that history events are no longer written when state information should be updated. #9252
* Fix an issue where incomplete comment history events were generated. #9301
  **Note:** when removing comments using the API, the dedicated remove-comment action
  should be used instead of the objects API, otherwise no history event will be generated.
* Fix handling of non-integer values for the order attribute of command arguments. #9181
  **Note:** You should only specify integer values for order, other values are converted to integer
  before use so using fractional numbers there has no effect.
* Add a dependency on icingadb-redis.service to the systemd service file
  so that Redis is stopped after Icinga 2. #9304
* Buffer history events in memory when the Redis connection is lost. #9271
* Add the previous soft state to the state tables. #9214
* Add missing locking on object runtime updates. #9300

## 2.13.2 (2021-11-12)

This version only includes changes needed for the release of Icinga DB 1.0.0 RC2 and doesn't include any other bugfixes or features.

### Icinga DB

* Prefix command_id with command type #9085
* Decouple environment from Icinga 2 Environment constant #9082
* Make icinga:history:stream:*#event_id deterministic #9076
* Add downtime.duration & service_state.host_id to Redis #9084
* Sync checkables along with their states first #9081
* Flush both buffered states and state checksums on initial dump #9079
* Introduce icinga:history:stream:downtime#scheduled_by #9080
* Actually write parent to parent_id of zones #9078
* Set value in milliseconds for program_start in stats/heartbeat #9077
* Clean up vanished objects from icinga:checksum:*:state #9074
* Remove usernotification history stream #9073
* Write IDs of notified users into notification history stream #9071
* Make CheckResult#scheduling_source available to Icinga DB #9072
* Stream runtime state updates only to icinga:runtime:state #9068
* Publish Redis schema version via XADD icinga:schema #9069
* Don't include checkable types in history IDs #9070
* Remove unused Redis key 'icinga:zone:parent' #9075
* Make sure object relationships are handled correctly during runtime updates #9089
* Only log queries at debug level #9088

## 2.13.1 (2021-08-19)

The main focus of this version is a security vulnerability in the TLS certificate verification of our metrics writers ElasticsearchWriter, GelfWriter, InfluxdbWriter and Influxdb2Writer.

Version 2.13.1 also fixes two issues indroduced with the 2.13.0 release.

### Security

* Add TLS server certificate validation to ElasticsearchWriter, GelfWriter, InfluxdbWriter and Influxdb2Writer ([GHSA-cxfm-8j5v-5qr2](https://github.com/Icinga/icinga2/security/advisories/GHSA-cxfm-8j5v-5qr2))

Depending on your setup, manual intervention beyond installing the new versions
may be required, so please read the more detailed information in the
[release blog post](https://icinga.com/blog/2021/08/19/icinga-2-13-1-security-release//)
carefully

### Bugfixes

* IDO PgSQL: Fix a string quoting regression introduced in 2.13.0 #8958
* ApiListener: Automatically fall back to IPv4 in default configuration on systems without IPv6 support #8961

## 2.13.0 (2021-08-03)

[Issues and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.13.0)

### Notes

Upgrading docs: https://icinga.com/docs/icinga2/snapshot/doc/16-upgrading-icinga-2/#upgrading-to-v213

Thanks to all contributors:
[andygrunwald](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aandygrunwald+milestone%3A2.13.0),
[BausPhi](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ABausPhi+milestone%3A2.13.0),
[bebehei](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abebehei+milestone%3A2.13.0),
[Bobobo-bo-Bo-bobo](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ABobobo-bo-Bo-bobo+milestone%3A2.13.0),
[efuss](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aefuss+milestone%3A2.13.0),
[froehl](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Afroehl+milestone%3A2.13.0),
[iustin](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aiustin+milestone%3A2.13.0),
[JochenFriedrich](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AJochenFriedrich+milestone%3A2.13.0),
[leeclemens](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aleeclemens+milestone%3A2.13.0),
[log1-c](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Alog1-c+milestone%3A2.13.0),
[lyknode](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Alyknode+milestone%3A2.13.0),
[m41kc0d3](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Am41kc0d3+milestone%3A2.13.0),
[MarcusCaepio](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AMarcusCaepio+milestone%3A2.13.0),
[mathiasaerts](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amathiasaerts+milestone%3A2.13.0),
[mcktr](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amcktr+milestone%3A2.13.0),
[MEschenbacher](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AMEschenbacher+milestone%3A2.13.0),
[Napsty](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ANapsty+milestone%3A2.13.0),
[netson](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Anetson+milestone%3A2.13.0),
[pdolinic](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Apdolinic+milestone%3A2.13.0),
[Ragnra](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ARagnra+milestone%3A2.13.0),
[RincewindsHat](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ARincewindsHat+milestone%3A2.13.0),
[sbraz](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asbraz+milestone%3A2.13.0),
[sni](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asni+milestone%3A2.13.0),
[sysadt](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asysadt+milestone%3A2.13.0),
[XnS](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AXnS+milestone%3A2.13.0),
[yayayayaka](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Ayayayayaka+milestone%3A2.13.0)

### Enhancements

* Core
  * PerfdataValue: Add units of measurement #7871
  * Flapping: Allow to ignore states in flapping detection #8600
* Cluster
  * Display log message if two nodes run on incompatible versions #8088
* API
  * /v1/actions/remove-downtime: Also remove child downtimes #8913
  * Add API endpoint: /v1/actions/execute-command #8040
  * /v1/actions/add-comment: Add param expiry #8035
  * API-Event StateChange & CheckResult: Add acknowledgement and downtime_depth #7736
  * Implement new API events ObjectCreated, ObjectDeleted and ObjectModified #8083
  * Implement scheduling_endpoint attribute to checkable #6326
* Windows
  * Add support for Windows Event Log and write early log messages to it #8710
* IDO
  * MySQL: support larger host and service names #8425
* ITL
  * Add -S parameter for esxi_hardware ITL #8814
  * Add CheckCommands for Thola #8683
  * Add option ignore-sct for ssl_cert to ITL #8625
  * Improve check_dns command when used with monitoring-plugins 2.3 #8589
  * Add parameter -f to snmp-process #8569
  * Add systemd CheckCommand #8568
  * Add new options for ipmi-sensor #8498
  * check_snmp_int: support -a #8003
  * check_fail2ban: Add parameter fail2ban_jail to monitor a specific jail only #7960
  * check_nrpe: Add parameters needed for PKI usage #7907
* Metrics
  * Support InfluxDB 2.0 #8719
  * Add support for InfluxDB basic auth #8314
* Docs
  * Add info about ongoing support for IDO #8446
  * Improve instructions on how to setup a Windows dev env #8400
  * Improve instructions for installing wixtoolset on Windows #8397
  * Add section about usage of satellites #8458
  * Document command for verifying the parent node's certificate #8221
  * Clarify TimePeriod/ScheduledDowntime time zone handling #8001
* Misc
  * Support TLS 1.3 #8718
  * Livestatus: append app name to program_version #7931
  * sd_notify() systemd about what we're doing right now #7874

### Bugfixes

* Core
  * Fix state not being UNKNOWN after process timeout #8937
  * Set a default severity for loggers #8846
  * Fix integer overflow when converting large unsigned integers to string #8742
  * StartUnixWorker(): don't exit() on fork() failure #8427
  * Fix perf data parser not recognizing scientific notation #8492
  * Close FDs based on /proc/self/fd #8442
  * Fix check source getting overwritten on passive check result #8158
  * Clean up temp files #8157
  * Improve perf data parser to allow for special output (e.g. ASCII tables) #8008
  * On check timeout first send SIGTERM #7918
* Cluster
  * Drop passive check results for unreachable hosts/services #8267
  * Fix state timestamps set by the same check result differing across nodes #8101
* API
  * Do not override status codes that are not 200 #8532
  * Update the SSL context after accepting incoming connections #8515
  * Allow to create API User with password #8321
  * Send Content-Type as API response header too #8108
  * Display a correct status when removing a downtime #8104
  * Display log message if a permission error occurs #8087
  * Replace broken package name validation regex #8825 #8946
* Windows
  * Fix Windows command escape for \" #7092
* Notifications/Downtimes
  * Fix no re-notification for non OK state changes with time delay #8562
  * TimePeriod/ScheduledDowntime: Improve DST handling #8921
  * Don't send notifications while suppressed by checkable #8513
  * Fix a crash while removing a downtime from a disappeared checkable #8229
* IDO
  * Update program status on stop #8730
  * Also mark objects inactive in memory on object deactivation #8626
  * IdoCheckTask: Don't override checkable critical with warn state #8613
  * PostgreSQL: Do not set standard_conforming_strings to off #8123
* ITL
  * check_http: Fix assignment of check_adress blocking check by hostname #8109
  * check_mysql: Don't set -H if -s is given #8020
* Metrics
  * OpenTSDB-Writer: Remove incorrect space causing missing tag error #8245

## 2.12.5 (2021-07-15)

Version 2.12.5 fixes two security vulnerabilities that may lead to privilege
escalation for authenticated API users. Other improvements include several
bugfixes related to downtimes, downtime notifications, and more reliable
connection handling.

### Security

* Don't expose the PKI ticket salt via the API. This may lead to privilege
  escalation for authenticated API users by them being able to request
  certificates for other identities (CVE-2021-32739)
* Don't expose IdoMysqlConnection, IdoPgsqlConnection, IcingaDB, and
  ElasticsearchWriter passwords via the API (CVE-2021-32743)
* Windows: Update bundled OpenSSL to version 1.1.1k #8885

Depending on your setup, manual intervention beyond installing the new versions
may be required, so please read the more detailed information in the
[release blog post](https://icinga.com/blog/2021/07/15/releasing-icinga-2-12-5-and-2-11-10/)
carefully.

### Bugfixes

* Don't send downtime end notification if downtime hasn't started #8877
* Don't let a failed downtime creation block the others #8863
* Support downtimes and comments for checkables with long names #8864
* Trigger fixed downtimes immediately if the current time matches
  (instead of waiting for the timer) #8889
* Add configurable timeout for full connection handshake #8866

### Enhancements

* Replace existing downtimes on ScheduledDowntime change #8879
* Improve crashlog #8865

## 2.12.4 (2021-05-27)

Version 2.12.4 is a maintenance release that fixes some crashes, improves error handling
and adds compatibility for systems coming with newer Boost versions.

### Bugfixes

* Fix a crash when notification objects are deleted using the API #8782
* Fix crashes that might occur during downtime scheduling if host or downtime objects are deleted using the API #8785
* Fix an issue where notifications may incorrectly be skipped after a downtime ends #8775
* Don't send reminder notification if the notification is still suppressed by a time period #8808
* Fix an issue where attempting to create a duplicate object using the API
  might result in the original object being deleted #8787
* IDO: prioritize program status updates #8809
* Improve exceptions handling, including a fix for an uncaught exception on Windows #8777
* Retry file rename operations on Windows to avoid intermittent locking issues #8771

### Enhancements

* Support Boost 1.74 (Ubuntu 21.04, Fedora 34) #8792

## 2.12.3 (2020-12-15)

Version 2.12.3 resolves a security vulnerability with revoked certificates being
renewed automatically ignoring the CRL.

This version also resolves issues with high load on Windows regarding the config sync
and not being able to disable/enable Icinga 2 features over the API.

### Security

* Fix that revoked certificates due for renewal will automatically be renewed ignoring the CRL (CVE-2020-29663)

When a CRL is specified in the ApiListener configuration, Icinga 2 only used it
when connections were established so far, but not when a certificate is requested.
This allows a node to automatically renew a revoked certificate if it meets the
other conditions for auto renewal (issued before 2017 or expires in less than 30 days).

Because Icinga 2 currently (v2.12.3 and earlier) uses a validity duration of 15 years,
this only affects setups with external certificate signing and revoked certificates
that expire in less then 30 days.

### Bugfixes

* Improve config sync locking - resolves high load issues on Windows #8511
* Fix runtime config updates being ignored for objects without zone #8549
* Use proper buffer size for OpenSSL error messages #8542

### Enhancements

* On checkable recovery: re-check children that have a problem #8506

## 2.12.2 (2020-12-01)

Version 2.12.2 fixes several issues to improve the reliability of the cluster functionality.

### Bugfixes

* Fix a connection leak with misconfigured agents #8483
* Properly sync changes of config objects in global zones done via the API #8474 #8470
* Prevent other clients from being disconnected when replaying the cluster log takes very long #8496
* Avoid duplicate connections between endpoints #8465
* Ignore incoming config object updates for unknown zones #8461
* Check timestamps before removing files in config sync #8495

### Enhancements

* Include HTTP status codes in log #8467

## 2.12.1 (2020-10-15)

Version 2.12.1 fixes several crashes, deadlocks and excessive check latencies.
It also addresses several bugs regarding IDO, API, notifications and checks.

### Bugfixes

* Core
  * Fix crashes during config update #8348 #8345
  * Fix crash while removing a downtime #8228
  * Ensure the daemon doesn't get killed by logrotate #8170
  * Fix hangup during shutdown #8211
  * Fix a deadlock in Icinga DB #8168
  * Clean up zombie processes during reload #8376
  * Reduce check latency #8276
* IDO
  * Prevent unnecessary IDO updates #8327 #8320
  * Commit IDO MySQL transactions earlier #8349
  * Make sure to insert IDO program status #8330
  * Improve IDO queue stats logging #8271 #8328 #8379
* Misc
  * Ensure API connections are closed properly #8293
  * Prevent unnecessary notifications #8299
  * Don't skip null values of command arguments #8174
  * Fix Windows .exe version #8234
  * Reset Icinga check warning after successful config update #8189

## 2.12.0 (2020-08-05)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.12.0)

### Notes

Upgrading docs: https://icinga.com/docs/icinga2/snapshot/doc/16-upgrading-icinga-2/#upgrading-to-v212

Thanks to all contributors:
[Ant1x](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AAnt1x+milestone%3A2.12.0),
[azthec](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aazthec+milestone%3A2.12.0),
[baurmatt](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abaurmatt+milestone%3A2.12.0),
[bootc](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abootc+milestone%3A2.12.0),
[Foxeronie](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AFoxeronie+milestone%3A2.12.0),
[ggzengel](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aggzengel+milestone%3A2.12.0),
[islander](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aislander+milestone%3A2.12.0),
[joni1993](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Ajoni1993+milestone%3A2.12.0),
[KAMI911](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AKAMI911+milestone%3A2.12.0),
[mcktr](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amcktr+milestone%3A2.12.0),
[MichalMMac](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AMichalMMac+milestone%3A2.12.0),
[sebastic](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asebastic+milestone%3A2.12.0),
[sthen](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asthen+milestone%3A2.12.0),
[unki](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aunki+milestone%3A2.12.0),
[vigiroux](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Avigiroux+milestone%3A2.12.0),
[wopfel](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Awopfel+milestone%3A2.12.0)

### Breaking changes

* Deprecate Windows plugins in favor of our
  [PowerShell plugins](https://github.com/Icinga/icinga-powershell-plugins) #8071
* Deprecate Livestatus #8051
* Refuse acknowledging an already acknowledged checkable #7695
* Config lexer: complain on EOF in heredocs, i.e. `{{{abc<EOF>` #7541

### Enhancements

* Core
  * Implement new database backend: Icinga DB #7571
  * Re-send notifications previously suppressed by their time periods #7816
* API
  * Host/Service: Add `acknowledgement_last_change` and `next_update` attributes #7881 #7534
  * Improve error message for POST queries #7681
  * /v1/actions/remove-comment: let users specify themselves #7646
  * /v1/actions/remove-downtime: let users specify themselves #7645
  * /v1/config/stages: Add 'activate' parameter #7535
* CLI
  * Add `pki verify` command for better TLS certificate troubleshooting #7843
  * Add OpenSSL version to 'Build' section in --version #7833
  * Improve experience with 'Node Setup for Agents/Satellite' #7835
* DSL
  * Add `get_template()` and `get_templates()` #7632
  * `MacroProcessor::ResolveArguments()`: skip null argument values #7567
  * Fix crash due to dependency apply rule with `ignore_on_error` and non-existing parent #7538
  * Introduce ternary operator (`x ? y : z`) #7442
  * LegacyTimePeriod: support specifying seconds #7439
  * Add support for Lambda Closures (`() use(x) => x and () use(x) => { return x }`) #7417
* ITL
  * Add notemp parameter to oracle health #7748
  * Add extended checks options to snmp-interface command template #7602
  * Add file age check for Windows command definition #7540
* Docs
  * Development: Update debugging instructions #7867
  * Add new API clients #7859
  * Clarify CRITICAL vs. UNKNOWN #7665
  * Explicitly explain how to disable freshness checks #7664
  * Update installation for RHEL/CentOS 8 and SLES 15 #7640
  * Add Powershell example to validate the certificate #7603
* Misc
  * Don't send `event::Heartbeat` to unauthenticated peers #7747
  * OpenTsdbWriter: Add custom tag support #7357

### Bugfixes

* Core
  * Fix JSON-RPC crashes #7532 #7737
  * Fix zone definitions in zones #7546
  * Fix deadlock during start on OpenBSD #7739
  * Consider PENDING not a problem #7685
  * Fix zombie processes after reload #7606
  * Don't wait for checks to finish during reload #7894
* Cluster
  * Fix segfault during heartbeat timeout with clients not yet signed #7970
  * Make the config update process mutually exclusive (Prevents file system race conditions) #7936
  * Fix `check_timeout` not being forwarded to agent command endpoints #7861
  * Config sync: Use a more friendly message when configs are equal and don't need a reload #7811
  * Fix open connections when agent waits for CA approval #7686
  * Consider a JsonRpcConnection alive on a single byte of TLS payload, not only on a whole message #7836
  * Send JsonRpcConnection heartbeat every 20s instead of 10s #8102
  * Use JsonRpcConnection heartbeat only to update connection liveness (m\_Seen) #8142
  * Fix TLS context not being updated on signed certificate messages on agents #7654
* API
  * Close connections w/o successful TLS handshakes after 10s #7809
  * Handle permission exceptions soon enough, returning 404 #7528
* SELinux
  * Fix safe-reload #7858
  * Allow direct SMTP notifications #7749
* Windows
  * Terminate check processes with UNKNOWN state on timeout #7788
  * Ensure that log replay files are properly renamed #7767
* Metrics
  * Graphite/OpenTSDB: Ensure that reconnect failure is detected #7765
  * Always send 0 as value for thresholds #7696
* Scripts
  * Fix notification scripts to stay compatible with Dash #7706
  * Fix bash line continuation in mail-host-notification.sh #7701
  * Fix notification scripts string comparison #7647
  * Service and host mail-notifications: Add line-breaks to very long output #6822
  * Set correct UTF-8 email subject header (RFC1342) #6369
* Misc
  * DSL: Fix segfault due to passing null as custom function to `Array#{sort,map,reduce,filter,any,all}()` #8053
  * CLI: `pki save-cert`: allow to specify --key and --cert for backwards compatibility #7995
  * Catch exception when trusted cert is not readable during node setup on agent/satellite #7838
  * CheckCommand ssl: Fix wrong parameter `-N` #7741
  * Code quality fixes
  * Small documentation fixes

## 2.12.0 RC1 (2020-03-13)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.12.0)

### Notes

Upgrading docs: https://icinga.com/docs/icinga2/snapshot/doc/16-upgrading-icinga-2/#upgrading-to-v212

Thanks to all contributors:
[Ant1x](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AAnt1x+milestone%3A2.12.0),
[azthec](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aazthec+milestone%3A2.12.0),
[baurmatt](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abaurmatt+milestone%3A2.12.0),
[bootc](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abootc+milestone%3A2.12.0),
[Foxeronie](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AFoxeronie+milestone%3A2.12.0),
[ggzengel](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aggzengel+milestone%3A2.12.0),
[islander](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aislander+milestone%3A2.12.0),
[joni1993](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Ajoni1993+milestone%3A2.12.0),
[KAMI911](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AKAMI911+milestone%3A2.12.0),
[mcktr](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amcktr+milestone%3A2.12.0),
[MichalMMac](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AMichalMMac+milestone%3A2.12.0),
[sebastic](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asebastic+milestone%3A2.12.0),
[sthen](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asthen+milestone%3A2.12.0),
[unki](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aunki+milestone%3A2.12.0),
[vigiroux](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Avigiroux+milestone%3A2.12.0),
[wopfel](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Awopfel+milestone%3A2.12.0),

### Breaking changes

* Refuse acknowledging an already acknowledged checkable #7695
* Config lexer: complain on EOF in heredocs, i.e. `{{{abc<EOF>` #7541

### Enhancements

* Core
  * Implement new database backend: Icinga DB #7571
* API
  * Host/Service: Add `acknowledgement_last_change` and `next_update` attributes #7881 #7534
  * Improve error message for POST queries #7681
  * /v1/actions/remove-comment: let users specify themselves #7646
  * /v1/actions/remove-downtime: let users specify themselves #7645
  * /v1/config/stages: Add 'activate' parameter #7535
* CLI
  * Add `pki verify` command for better TLS certificate troubleshooting #7843
  * Add OpenSSL version to 'Build' section in --version #7833
  * Improve experience with 'Node Setup for Agents/Satellite' #7835
* DSL
  * Add `get_template()` and `get_templates()` #7632
  * `MacroProcessor::ResolveArguments()`: skip null argument values #7567
  * Fix crash due to dependency apply rule with `ignore_on_error` and non-existing parent #7538
  * Introduce ternary operator (`x ? y : z`) #7442
  * LegacyTimePeriod: support specifying seconds #7439
  * Add support for Lambda Closures (`() use(x) => x and () use(x) => { return x }`) #7417
* ITL
  * Add notemp parameter to oracle health #7748
  * Add extended checks options to snmp-interface command template #7602
  * Add file age check for Windows command definition #7540
* Docs
  * Development: Update debugging instructions #7867
  * Add new API clients #7859
  * Clarify CRITICAL vs. UNKNOWN #7665
  * Explicitly explain how to disable freshness checks #7664
  * Update installation for RHEL/CentOS 8 and SLES 15 #7640
  * Add Powershell example to validate the certificate #7603
* Misc
  * Don't send `event::Heartbeat` to unauthenticated peers #7747
  * OpenTsdbWriter: Add custom tag support #7357

### Bugfixes

* Core
  * Fix JSON-RPC crashes #7532 #7737
  * Fix zone definitions in zones #7546
  * Fix deadlock during start on OpenBSD #7739
  * Consider PENDING not a problem #7685
  * Fix zombie processes after reload #7606
* Cluster
  * Fix `check_timeout` not being forwarded to agent command endpoints #7861
  * Config sync: Use a more friendly message when configs are equal and don't need a reload #7811
  * Fix open connections when agent waits for CA approval #7686
  * Fix TLS context not being updated on signed certificate messages on agents #7654
* API
  * Close connections w/o successful TLS handshakes after 10s #7809
  * Handle permission exceptions soon enough, returning 404 #7528
* SELinux
  * Fix safe-reload #7858
  * Allow direct SMTP notifications #7749
* Windows
  * Terminate check processes with UNKNOWN state on timeout #7788
  * Ensure that log replay files are properly renamed #7767
* Metrics
  * Graphite/OpenTSDB: Ensure that reconnect failure is detected #7765
  * Always send 0 as value for thresholds #7696
* Scripts
  * Fix notification scripts to stay compatible with Dash #7706
  * Fix bash line continuation in mail-host-notification.sh #7701
  * Fix notification scripts string comparison #7647
  * Service and host mail-notifications: Add line-breaks to very long output #6822
  * Set correct UTF-8 email subject header (RFC1342) #6369
* Misc
  * Catch exception when trusted cert is not readable during node setup on agent/satellite #7838
  * CheckCommand ssl: Fix wrong parameter `-N` #7741
  * Code quality fixes
  * Small documentation fixes

## 2.11.11 (2021-08-19)

The main focus of these versions is a security vulnerability in the TLS certificate verification of our metrics writers ElasticsearchWriter, GelfWriter and InfluxdbWriter.

### Security

* Add TLS server certificate validation to ElasticsearchWriter, GelfWriter and InfluxdbWriter

Depending on your setup, manual intervention beyond installing the new versions
may be required, so please read the more detailed information in the
[release blog post](https://icinga.com/blog/2021/08/19/icinga-2-13-1-security-release//)
carefully

## 2.11.10 (2021-07-15)

Version 2.11.10 fixes two security vulnerabilities that may lead to privilege
escalation for authenticated API users. Other improvements include several
bugfixes related to downtimes, downtime notifications, and more reliable
connection handling.

### Security

* Don't expose the PKI ticket salt via the API. This may lead to privilege
  escalation for authenticated API users by them being able to request
  certificates for other identities (CVE-2021-32739)
* Don't expose IdoMysqlConnection, IdoPgsqlConnection, and ElasticsearchWriter
  passwords via the API (CVE-2021-32743)
* Windows: Update bundled OpenSSL to version 1.1.1k #8888

Depending on your setup, manual intervention beyond installing the new versions
may be required, so please read the more detailed information in the
[release blog post](https://icinga.com/blog/2021/07/15/releasing-icinga-2-12-5-and-2-11-10/)
carefully.

### Bugfixes

* Don't send downtime end notification if downtime hasn't started #8878
* Don't let a failed downtime creation block the others #8871
* Support downtimes and comments for checkables with long names #8870
* Trigger fixed downtimes immediately if the current time matches
  (instead of waiting for the timer) #8891
* Add configurable timeout for full connection handshake #8872

### Enhancements

* Replace existing downtimes on ScheduledDowntime change #8880
* Improve crashlog #8869

## 2.11.9 (2021-05-27)

Version 2.11.9 is a maintenance release that fixes some crashes, improves error handling
and adds compatibility for systems coming with newer Boost versions.

### Bugfixes

* Fix a crash when notification objects are deleted using the API #8780
* Fix crashes that might occur during downtime scheduling if host or downtime objects are deleted using the API #8784
* Fix an issue where notifications may incorrectly be skipped after a downtime ends #8772
* Fix an issue where attempting to create a duplicate object using the API
  might result in the original object being deleted #8788
* IDO: prioritize program status updates #8810
* Improve exceptions handling, including a fix for an uncaught exception on Windows #8776
* Retry file rename operations on Windows to avoid intermittent locking issues #8770

### Enhancements

* Support Boost 1.74 (Ubuntu 21.04, Fedora 34) #8793 #8802

## 2.11.8 (2020-12-15)

Version 2.11.8 resolves a security vulnerability with revoked certificates being
renewed automatically ignoring the CRL.

This version also resolves issues with high load on Windows regarding the config sync
and not being able to disable/enable Icinga 2 features over the API.

### Security

* Fix that revoked certificates due for renewal will automatically be renewed ignoring the CRL (CVE-2020-29663)

When a CRL is specified in the ApiListener configuration, Icinga 2 only used it
when connections were established so far, but not when a certificate is requested.
This allows a node to automatically renew a revoked certificate if it meets the
other conditions for auto renewal (issued before 2017 or expires in less than 30 days).

Because Icinga 2 currently (v2.12.3 and earlier) uses a validity duration of 15 years,
this only affects setups with external certificate signing and revoked certificates
that expire in less then 30 days.

### Bugfixes

* Improve config sync locking - resolves high load issues on Windows #8510
* Fix runtime config updates being ignored for objects without zone #8550
* Use proper buffer size for OpenSSL error messages #8543

### Enhancements

* On checkable recovery: re-check children that have a problem #8560 

## 2.11.7 (2020-12-01)

Version 2.11.7 fixes several issues to improve the reliability of the cluster functionality.

### Bugfixes

* Fix a connection leak with misconfigured agents #8482
* Properly sync changes of config objects in global zones done via the API #8473 #8457
* Prevent other clients from being disconnected when replaying the cluster log takes very long #8475
* Avoid duplicate connections between endpoints #8399
* Ignore incoming config object updates for unknown zones #8459
* Check timestamps before removing files in config sync #8486

### Enhancements

* Include HTTP status codes in log #8454

## 2.11.6 (2020-10-15)

Version 2.11.6 fixes several crashes, prevents unnecessary notifications
and addresses several bugs in IDO and API.

### Bugfixes

* Crashes
  * Fix crashes during config update #8337 #8308
  * Fix crash while removing a downtime #8226
  * Ensure the daemon doesn't get killed by logrotate #8227
* IDO
  * Prevent unnecessary IDO updates #8316 #8305
  * Commit IDO MySQL transactions earlier #8298
  * Make sure to insert IDO program status #8291
  * Improve IDO queue stats logging #8270 #8325 #8378
* API
  * Ensure API connections are closed properly #8292
  * Fix open connections when agent waits for CA approval #8230
  * Close connections without successful TLS handshakes within 10s #8224
* Misc
  * Prevent unnecessary notifications #8300
  * Fix Windows .exe version #8235
  * Reset Icinga check warning after successful config update #8225

## 2.11.5 (2020-08-05)

Version 2.11.5 fixes file system race conditions
in the config update process occurring in large HA environments
and improves the cluster connection liveness mechanisms.

### Bugfixes

* Make the config update process mutually exclusive (Prevents file system race conditions) #8093
* Consider a JsonRpcConnection alive on a single byte of TLS payload, not only on a whole message #8094
* Send JsonRpcConnection heartbeat every 20s instead of 10s #8103
* Use JsonRpcConnection heartbeat only to update connection liveness (m\_Seen) #8097

## 2.11.4 (2020-06-18)

Version 2.11.4 fixes a crash during a heartbeat timeout with clients not yet signed. It also resolves
an issue with endpoints not reconnecting after a reload/deploy, which caused a lot of UNKNOWN states.

### Bugfixes

* Cluster
  * Fix segfault during heartbeat timeout with clients not yet signed #7997
  * Fix endpoints not reconnecting after reload (UNKNOWN hosts/services after reload) #8043
* Setup
  * Fix exception on trusted cert not readable during node setup #8044
  * prepare-dirs: Only set permissions during directory creation #8046
* DSL
  * Fix segfault on missing compare function in Array functions (sort, map, reduce, filter, any, all) #8054

## 2.11.3 (2020-03-02)

The 2.11.3 release fixes a critical crash in our JSON-RPC connections. This mainly affects large HA
enabled environments.

### Bugfixes

* Cluster
  * JSON-RPC Crashes with 2.11 #7532

## 2.11.2 (2019-10-24)

2.11.2 fixes a problem where the newly introduced config sync "check-change-then-reload" functionality
could cause endless reload loops with agents. The most visible parts are failing command endpoint checks
with "not connected" UNKNOWN state. **Only applies to HA enabled zones with 2 masters and/or 2 satellites.**

### Bugfixes

* Cluster Config Sync
  * Config sync checksum change detection may not work within high load HA clusters #7565

## 2.11.1 (2019-10-17)

This release fixes a hidden long lasting bug unveiled with 2.11 and distributed setups.
If you are affected by agents/satellites not accepting configuration
anymore, or not reloading, please upgrade.

### Bugfixes

* Cluster Config Sync
  * Never accept authoritative config markers from other instances #7552
  * This affects setups where agent/satellites are newer than the config master, e.g. satellite/agent=2.11.0, master=2.10.
* Configuration
  * Error message for `command_endpoint` should hint that zone is not set #7514
  * Global variable 'ActiveStageOverride' has been set implicitly via 'ActiveStageOverride ... #7521

### Documentation

* Docs: Add upgrading/troubleshooting details for repos, config sync, agents #7526
  * Explain repository requirements for 2.11: https://icinga.com/docs/icinga2/latest/doc/16-upgrading-icinga-2/#added-boost-166
  * `command_endpoint` objects require a zone: https://icinga.com/docs/icinga2/latest/doc/16-upgrading-icinga-2/#agent-hosts-with-command-endpoint-require-a-zone
  * Zones declared in zones.d are not loaded anymore: https://icinga.com/docs/icinga2/latest/doc/16-upgrading-icinga-2/#config-sync-zones-in-zones


## 2.11.0 (2019-09-19)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.11.0)

### Notes

Upgrading docs: https://icinga.com/docs/icinga2/snapshot/doc/16-upgrading-icinga-2/

Thanks to all contributors: [Obihoernchen](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AObihoernchen), [dasJ](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AdasJ), [sebastic](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Asebastic), [waja](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Awaja), [BarbUk](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ABarbUk), [alanlitster](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aalanlitster), [mcktr](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amcktr), [KAMI911](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AKAMI911), [peteeckel](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Apeteeckel), [breml](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abreml), [episodeiv](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aepisodeiv), [Crited](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ACrited), [robert-scheck](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Arobert-scheck), [west0rmann](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Awest0rmann), [Napsty](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ANapsty), [Elias481](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AElias481), [uubk](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Auubk), [miso231](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amiso231), [neubi4](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aneubi4), [atj](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aatj), [mvanduren-itisit](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amvanduren-itisit), [jschanz](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Ajschanz), [MaBauMeBad](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AMaBauMeBad), [markleary](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amarkleary), [leeclemens](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aleeclemens), [m4k5ym](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Am4k5ym)

### Enhancements

* Core
  * Rewrite Network Stack (cluster, REST API) based on Boost Asio, Beast, Coroutines
     * Technical concept: #7041
     * Requires package updates: Boost >1.66 (either from packages.icinga.com, EPEL or backports). SLES11 & Ubuntu 14 are EOL.
     * Require TLS 1.2 and harden default cipher list
  * Improved Reload Handling (umbrella process, now 3 processes at runtime)
    * Support running Icinga 2 in (Docker) containers natively in foreground
  * Quality: Use Modern JSON for C++ library instead of YAJL (dead project)
  * Quality: Improve handling of invalid UTF8 strings
* API
  * Fix crashes on Linux, Unix and Windows from Nessus scans #7431
  * Locks and stalled waits are fixed with the core rewrite in #7071
  * schedule-downtime action supports `all_services` for host downtimes
  * Improve storage handling for runtime created objects in the `_api` package
* Cluster
  * HA aware features & improvements for failover handling #2941 #7062
  * Improve cluster config sync with staging #6716
  * Fixed that same downtime/comment objects would be synced again in a cluster loop #7198
* Checks & Notifications
  * Ensure that notifications during a restart are sent
  * Immediately notify about a problem after leaving a downtime and still NOT-OK
  * Improve reload handling and wait for features/metrics
  * Store notification command results and sync them in HA enabled zones #6722
* DSL/Configuration
  * Add getenv() function
  * Fix TimePeriod range support over midnight
  * `concurrent_checks` in the Checker feature has no effect, use the global MaxConcurrentChecks constant instead
* CLI
  * Permissions: node wizard/setup, feature, api setup now run in the Icinga user context, not root
  * `ca list` shows pending CSRs by default, `ca remove/restore` allow to delete signing requests
* ITL
  * Add new commands and missing attributes
* Windows
  * Update bundled NSClient++ to 0.5.2.39
  * Refine agent setup wizard & update requirements to .NET 4.6
* Documentation
  * Service Monitoring: How to create plugins by example, check commands and a modern version of the supported plugin API with best practices
  * Features: Better structure on metrics, and supported features
  * Technical Concepts: TLS Network IO, Cluster Feature HA, Cluster Config Sync
  * Development: Rewritten for better debugging and development experience for contributors including a style guide.  Add nightly build setup instructions.
  * Packaging: INSTALL.md was integrated into the Development chapter, being available at https://icinga.com/docs too.




## 2.11.0 RC1 (2019-07-25)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.11.0)

### Notes

**This is the first release candidate for 2.11.**

Upgrading docs: https://icinga.com/docs/icinga2/snapshot/doc/16-upgrading-icinga-2/

Thanks to all contributors: [BarbUk](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ABarbUk), [alanlitster](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aalanlitster), [mcktr](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amcktr), [KAMI911](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AKAMI911), [peteeckel](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Apeteeckel), [breml](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Abreml), [episodeiv](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aepisodeiv), [Crited](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ACrited), [robert-scheck](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Arobert-scheck), [west0rmann](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Awest0rmann), [Napsty](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3ANapsty), [Elias481](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AElias481), [uubk](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Auubk), [miso231](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amiso231), [neubi4](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aneubi4), [atj](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aatj), [mvanduren-itisit](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amvanduren-itisit), [jschanz](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Ajschanz), [MaBauMeBad](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3AMaBauMeBad), [markleary](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Amarkleary), [leeclemens](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Aleeclemens), [m4k5ym](https://github.com/Icinga/icinga2/pulls?q=is%3Apr+author%3Am4k5ym)

### Enhancements

* Core
  * Rewrite Network Stack (cluster, REST API) based on Boost Asio, Beast, Coroutines
     * Technical concept: #7041
     * Requires package updates: Boost >1.66 (either from packages.icinga.com, EPEL or backports). SLES11 & Ubuntu 14 are EOL.
     * Require TLS 1.2 and harden default cipher list
  * Improved Reload Handling (umbrella process, now 3 processes at runtime)
    * Support running Icinga 2 in (Docker) containers natively in foreground
  * Quality: Use Modern JSON for C++ library instead of YAJL (dead project)
  * Quality: Improve handling of invalid UTF8 strings
* API
  * Fix crashes and problems with permission filters from recent Namespace introduction #6785 (thanks Elias Ohm) #6874 (backported to 2.10.5)
  * Locks and stalled waits are fixed with the core rewrite in #7071
  * schedule-downtime action supports `all_services` for host downtimes
  * Improve storage handling for runtime created objects in the `_api` package
* Cluster
  * HA aware features & improvements for failover handling #2941 #7062
  * Improve cluster config sync with staging #6716
* Checks & Notifications
  * Ensure that notifications during a restart are sent
  * Immediately notify about a problem after leaving a downtime and still NOT-OK
  * Improve reload handling and wait for features/metrics
  * Store notification command results and sync them in HA enabled zones #6722
* DSL/Configuration
  * Add getenv() function
  * Fix TimePeriod range support over midnight
  * `concurrent_checks` in the Checker feature has no effect, use the global MaxConcurrentChecks constant instead
* CLI
  * Permissions: node wizard/setup, feature, api setup now run in the Icinga user context, not root
  * `ca list` shows pending CSRs by default, `ca remove/restore` allow to delete signing requests
* ITL
  * Add new commands and missing attributes - thanks to all contributors!
* Windows
  * Update bundled NSClient++ to 0.5.2.39
  * Update agent installer and OpenSSL
* Documentation
  * Service Monitoring: How to create plugins by example, check commands and a modern version of the supported plugin API with best practices.
  * Features: Better structure on metrics, and supported features.
  * Basics: Rename `Custom Attributes` to `Custom Variables`.
  * Basics: Refine explanation of command arguments.
  * Distributed: Reword `Icinga client` into `Icinga agent` and add new images for scenarios and modes.
  * Security: Add TLS v1.2+ requirement, hardened cipher lists
  * Technical Concepts: TLS Network IO, Cluster Feature HA, Cluster Config Sync, Core Reload Handling.
  * Development: Rewritten for better debugging and development experience for contributors including a style guide.  Add nightly build setup instructions.
  * Packaging: INSTALL.md was integrated into the Development chapter available at https://icinga.com/docs too.




## 2.10.7 (2019-10-17)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.10.7)

### Bugfixes

* Cluster config master must not load/sync its marker to other instances #7544
  * This affects scenarios where the satellite/agent is newer than the master, e.g. master=2.10.x satellite=2.11.0


## 2.10.6 (2019-07-30)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.10.6)

### Bugfixes

* Fix el7 not loading ECDHE cipher suites #7247


## 2.10.5 (2019-05-23)

[Issues and PRs](https://github.com/Icinga/icinga2/milestone/81?closed=1)

### Bugfixes

* Core
  * Fix crashes with logrotate signals #6737 (thanks Elias Ohm)
* API
  * Fix crashes and problems with permission filters from recent Namespace introduction #6785 (thanks Elias Ohm) #6874 (backported from 2.11)
  * Reduce log spam with locked connections (real fix is the network stack rewrite in 2.11) #6877
* Cluster
  * Fix problems with replay log rotation and storage #6932 (thanks Peter Eckel)
* IDO DB
  * Fix that reload shutdown deactivates hosts and hostgroups (introduced in 2.9) #7157
* Documentation
  * Improve the [REST API](https://icinga.com/docs/icinga2/latest/doc/12-icinga2-api/) chapter: Unix timestamp handling, filters, unify POST requests with filters in the body
  * Better layout for the [features](https://icinga.com/docs/icinga2/latest/doc/14-features/) chapter, specifically metrics and events
  * Split [object types](https://icinga.com/docs/icinga2/latest/doc/09-object-types/) into monitoring, runtime, features
  * Add technical concepts for [cluster messages](https://icinga.com/docs/icinga2/latest/doc/19-technical-concepts/#json-rpc-message-api)


## 2.10.4 (2019-03-19)

### Notes

* Fix TLS connections in Influxdb/Elasticsearch features leaking file descriptors (#6989 #7018 ref/IP/12219)
* Fixes for delayed and one-time notifications (#5561 #6757)
* Improve performance for downtimes/comments added in HA clusters (#6885 ref/IP/9235)
* check_perfmon supports non-localized performance counter names (#5546 #6418)

### Enhancement

* [#6732](https://github.com/icinga/icinga2/issues/6732) (Windows, PR): Update Windows Agent with new design
* [#6729](https://github.com/icinga/icinga2/issues/6729) (Windows): Polish the Windows Agent design
* [#6418](https://github.com/icinga/icinga2/issues/6418) (Windows): check\_perfmon.exe: Add fallback support for localized performance counters

### Bug

* [#7020](https://github.com/icinga/icinga2/issues/7020) (Elasticsearch, PR): ElasticsearchWriter: don't leak sockets
* [#7018](https://github.com/icinga/icinga2/issues/7018) (Elasticsearch): ElasticsearchWriter not closing SSL connections on  Icinga2 2.10.3.1
* [#6991](https://github.com/icinga/icinga2/issues/6991) (CLI, PR): PkiUtility::NewCa\(\): just warn if the CA files already exist
* [#6990](https://github.com/icinga/icinga2/issues/6990) (InfluxDB, PR): InfluxdbWriter: don't leak sockets
* [#6989](https://github.com/icinga/icinga2/issues/6989) (InfluxDB): InfluxdbWriter not closing connections Icinga2 2.10.3 CentOS 7
* [#6976](https://github.com/icinga/icinga2/issues/6976) (Cluster, PR): Don't require OS headers to provide SO\_REUSEPORT
* [#6896](https://github.com/icinga/icinga2/issues/6896) (Notifications, PR): Notification\#BeginExecuteNotification\(\): SetNextNotification\(\) correctly
* [#6885](https://github.com/icinga/icinga2/issues/6885) (API, Configuration, PR): Don't run UpdateObjectAuthority for Comments and Downtimes
* [#6800](https://github.com/icinga/icinga2/issues/6800) (Plugins, Windows, PR): Fix check\_perfmon to support non-localized names
* [#6757](https://github.com/icinga/icinga2/issues/6757) (Notifications, PR): Fix that no\_more\_notifications gets reset when Recovery notifications are filtered away
* [#5561](https://github.com/icinga/icinga2/issues/5561) (Notifications): Set the notification mode times.begin is not 0, the first notification has a delay
* [#5546](https://github.com/icinga/icinga2/issues/5546) (Plugins, Windows): check\_perfmon.exe doesn't support cyrillic names of perf counters

### Documentation

* [#7033](https://github.com/icinga/icinga2/issues/7033) (Documentation, PR): Docs: Update supported package repos in Getting Started chapter
* [#7028](https://github.com/icinga/icinga2/issues/7028) (Documentation, PR): Fix heading level in development chapter
* [#7001](https://github.com/icinga/icinga2/issues/7001) (Documentation, PR): Assignment operators doc: tell what the { } are for
* [#6995](https://github.com/icinga/icinga2/issues/6995) (Documentation, PR): Typo and link fix
* [#6979](https://github.com/icinga/icinga2/issues/6979) (Documentation, PR): Doc: write systemd lower-case
* [#6975](https://github.com/icinga/icinga2/issues/6975) (Documentation, PR): Fix nested hostgroup example
* [#6949](https://github.com/icinga/icinga2/issues/6949) (Documentation, PR): Doc fix: update check\_rbl parameter
* [#6708](https://github.com/icinga/icinga2/issues/6708) (Documentation, PR): Docs: Alpine needs 'edge/main' repository too
* [#5430](https://github.com/icinga/icinga2/issues/5430) (Documentation): Documentation about dictionaries and assignements

### Support

* [#7032](https://github.com/icinga/icinga2/issues/7032) (code-quality, PR): Backport Defer class for 2.10
* [#7030](https://github.com/icinga/icinga2/issues/7030) (Packages, PR): SELinux: add unreserved\_port\_type attribute to icinga2\_port\_t
* [#7029](https://github.com/icinga/icinga2/issues/7029) (Packages): Add unreserved\_port\_type attribute to icinga2\_port\_t
* [#7002](https://github.com/icinga/icinga2/issues/7002) (Plugins, Windows, PR): check\_network -h: drop non-existent feature
* [#6987](https://github.com/icinga/icinga2/issues/6987) (Tests): base-base\_utility/comparepasswords\_issafe test fails on i386
* [#6977](https://github.com/icinga/icinga2/issues/6977) (Tests, PR): Ignore failure of unit test base\_utility/comparepasswords\_issafe

## 2.10.3 (2019-02-26)

### Notes

Bugfixes:

- Stalled TLS connections on reload/Director deployments (#6816 #6898 ref/NC/588119)
- 'Connection: close' header leading to unstable instance, affects Ruby clients (#6799)
- Server time in the future breaks check result processing (#6797 ref/NC/595861)
- ScheduledDowntimes: Generate downtime objects only on one HA endpoint (#2844 ref/IC/9673 ref/NC/590167 ref/NC/591721)
- Improve activation & syncing for downtime objects generated from ScheduledDowntimes (#6826 ref/IC/9673 ref/NC/585559)
- Generate a runtime downtime object from already running ScheduledDowntime objects (#6704)
- DB IDO: Don't enqueue queries when the feature is paused in HA zones (#5876)
- Crashes with localtime_r errors (#6887)

Documentation updates:

- Ephemeral port range blocking on Windows agents (ref/NC/597307)
- Technical concepts for the check scheduler (#6775)
- DB IDO cleanup (#6791)
- Unified development docs (#6819)

### Bug

* [#6971](https://github.com/icinga/icinga2/issues/6971) (Notifications, PR): Activate downtimes before any checkable object
* [#6968](https://github.com/icinga/icinga2/issues/6968) (API, PR): Secure ApiUser::GetByAuthHeader\(\) against timing attacks
* [#6940](https://github.com/icinga/icinga2/issues/6940) (Plugins, Windows, PR): Fix check\_swap percentage calculation
* [#6925](https://github.com/icinga/icinga2/issues/6925) (Plugins, Windows, PR): Fix check\_swap formatting
* [#6924](https://github.com/icinga/icinga2/issues/6924) (PR): Fix double to long conversions
* [#6922](https://github.com/icinga/icinga2/issues/6922) (API, DB IDO): IDO MySQL fails on start if check\_interval is a float \(Icinga 2.9.2\)
* [#6920](https://github.com/icinga/icinga2/issues/6920) (PR): Downtime::AddDowntime\(\): place Downtimes in the same zone as the origin ScheduledDowntimes
* [#6917](https://github.com/icinga/icinga2/issues/6917) (Cluster, Log, PR): Cluster: Delete object message should log that
* [#6916](https://github.com/icinga/icinga2/issues/6916) (PR): Don't allow retry\_interval \<= 0
* [#6914](https://github.com/icinga/icinga2/issues/6914) (Cluster, PR): ClusterEvents::AcknowledgementSet event should forward 'persistent' attribute
* [#6913](https://github.com/icinga/icinga2/issues/6913) (Plugins, Windows): check\_swap return value wrong when no swap file configured
* [#6901](https://github.com/icinga/icinga2/issues/6901) (API, PR): TcpSocket\#Bind\(\): also set SO\_REUSEPORT
* [#6899](https://github.com/icinga/icinga2/issues/6899) (PR): Log: Ensure not to pass negative values to localtime\(\)
* [#6898](https://github.com/icinga/icinga2/issues/6898) (API): API action restart-process fails on FreeBSD
* [#6894](https://github.com/icinga/icinga2/issues/6894) (Check Execution, PR): Fix checkresults from the future breaking checks
* [#6887](https://github.com/icinga/icinga2/issues/6887) (Check Execution, Windows): Icinga2 Windows Service does not start critical/checker: Exception occurred while checking 'hostname.tld'
* [#6883](https://github.com/icinga/icinga2/issues/6883) (Check Execution, PR): Allow Checkable\#retry\_interval to be 0
* [#6871](https://github.com/icinga/icinga2/issues/6871): Icinga2 crashes after localtime\_r call
* [#6857](https://github.com/icinga/icinga2/issues/6857) (Plugins, Windows, PR): Url\#m\_Query: preserve order
* [#6826](https://github.com/icinga/icinga2/issues/6826) (Configuration, PR): Downtime\#HasValidConfigOwner\(\): wait for ScheduledDowntimes
* [#6821](https://github.com/icinga/icinga2/issues/6821) (Cluster, Configuration, PR): Don't delete downtimes in satellite zones
* [#6820](https://github.com/icinga/icinga2/issues/6820) (Cluster, PR): Only create downtimes from non-paused ScheduledDowntime objects in HA enabled cluster zones
* [#6817](https://github.com/icinga/icinga2/issues/6817) (API, PR): HttpServerConnection\#DataAvailableHandler\(\): be aware of being called multiple times concurrently
* [#6816](https://github.com/icinga/icinga2/issues/6816) (API, Cluster): Stalled TLS connections and lock waits in SocketEventEngine
* [#6814](https://github.com/icinga/icinga2/issues/6814) (API, PR): Restore 'Connection: close' behaviour in HTTP responses
* [#6811](https://github.com/icinga/icinga2/issues/6811) (Plugins, Windows, PR): Fix state conditions in check\_memory and check\_swap
* [#6810](https://github.com/icinga/icinga2/issues/6810) (Plugins, Windows): Windows check\_memory never gets critical
* [#6808](https://github.com/icinga/icinga2/issues/6808) (API, PR): Remove redundand check for object existence on creation via API
* [#6807](https://github.com/icinga/icinga2/issues/6807) (API): \[2.10.2\] Director deploy crashes the Icinga service \[FreeBSD\]
* [#6799](https://github.com/icinga/icinga2/issues/6799) (API): "Connection: close" header leads to unstable instance
* [#6797](https://github.com/icinga/icinga2/issues/6797) (Check Execution): Servertime in the future breaks check results processing
* [#6750](https://github.com/icinga/icinga2/issues/6750) (Configuration, PR): \#6749 Wrong operator on stride variable causing incorrect behaviour
* [#6749](https://github.com/icinga/icinga2/issues/6749) (Configuration): Stride is misinterpreted in multi-date legacydatetime
* [#6748](https://github.com/icinga/icinga2/issues/6748) (CLI, PR): Fix api setup to automatically create the conf.d directory
* [#6718](https://github.com/icinga/icinga2/issues/6718) (API, Cluster, PR): Call SSL\_shutdown\(\) at least twice
* [#6704](https://github.com/icinga/icinga2/issues/6704) (Notifications, PR): Put newly configured already running ScheduledDowntime immediately in effect
* [#6542](https://github.com/icinga/icinga2/issues/6542) (Configuration, Log): /var/log/icinga2/icinga2.log is growing very fast on satellites 
* [#6536](https://github.com/icinga/icinga2/issues/6536) (Windows, help wanted): check\_nscp\_api: Query arguments are sorted on Url::Format\(\)
* [#4790](https://github.com/icinga/icinga2/issues/4790) (Notifications): Newly configured already running ScheduledDowntime not put into effect
* [#3937](https://github.com/icinga/icinga2/issues/3937) (API): Icinga2 API: PUT request fails at 0-byte file
* [#2844](https://github.com/icinga/icinga2/issues/2844) (Cluster): Duplicated scheduled downtimes created in cluster HA zone

### Documentation

* [#6956](https://github.com/icinga/icinga2/issues/6956) (Documentation, PR): Escape pipe symbol in api documentation
* [#6944](https://github.com/icinga/icinga2/issues/6944) (Documentation, PR): Troubleshooting: Add notes on ephemeral port range blocking on Windows agents
* [#6928](https://github.com/icinga/icinga2/issues/6928) (Documentation, PR): Doc: Add .NET 3.5 to the windows build stack
* [#6825](https://github.com/icinga/icinga2/issues/6825) (Documentation, PR): Document that retry\_interval is only used after an active check result
* [#6819](https://github.com/icinga/icinga2/issues/6819) (Documentation, PR): Enhance and unify development docs for debug, develop, package
* [#6791](https://github.com/icinga/icinga2/issues/6791) (Documentation, PR): Docs: Add a section for DB IDO Cleanup
* [#6776](https://github.com/icinga/icinga2/issues/6776) (Documentation, PR): Doc fix: update apache section
* [#6775](https://github.com/icinga/icinga2/issues/6775) (Documentation, PR): Add technical docs for the check scheduler \(general, initial check, offsets\)
* [#6751](https://github.com/icinga/icinga2/issues/6751) (Documentation, PR): Doc fix: documentation link for apt
* [#6743](https://github.com/icinga/icinga2/issues/6743) (Documentation, PR): Doc fix: error in example path.
* [#5341](https://github.com/icinga/icinga2/issues/5341) (Documentation): Enhance development documentation

### Support

* [#6972](https://github.com/icinga/icinga2/issues/6972) (PR): Fix formatting in development docs
* [#6958](https://github.com/icinga/icinga2/issues/6958) (code-quality, PR): Debug: Log calls to ConfigObject::Deactivate\(\)
* [#6897](https://github.com/icinga/icinga2/issues/6897) (PR): Validate Zone::GetLocalZone\(\) before using
* [#6872](https://github.com/icinga/icinga2/issues/6872) (Windows): 2.10 is unstable \(Windows Agent\)
* [#6843](https://github.com/icinga/icinga2/issues/6843) (Tests, Windows, PR): Improve AppVeyor builds
* [#6479](https://github.com/icinga/icinga2/issues/6479) (code-quality, PR): SocketEvents: inherit from Stream
* [#6477](https://github.com/icinga/icinga2/issues/6477) (code-quality): SocketEvents: inherit from Object

## 2.10.2 (2018-11-14)

### Bug

* [#6770](https://github.com/icinga/icinga2/issues/6770) (PR): Fix deadlock in GraphiteWriter
* [#6769](https://github.com/icinga/icinga2/issues/6769) (Cluster): Hanging TLS connections
* [#6759](https://github.com/icinga/icinga2/issues/6759) (Log, PR): Fix possible double free in StreamLogger::BindStream\(\)
* [#6753](https://github.com/icinga/icinga2/issues/6753): Icinga2.service state is reloading in systemd after safe-reload until systemd time-out
* [#6740](https://github.com/icinga/icinga2/issues/6740) (DB IDO, PR): DB IDO: Don't enqueue queries when the feature is paused \(HA\)
* [#6738](https://github.com/icinga/icinga2/issues/6738) (API, Cluster, PR): Ensure that API/JSON-RPC messages in the same session are processed and not stalled
* [#6736](https://github.com/icinga/icinga2/issues/6736) (Crash): Stability issues with Icinga 2.10.x
* [#6717](https://github.com/icinga/icinga2/issues/6717) (API, PR): Improve error handling for invalid child\_options for API downtime actions
* [#6712](https://github.com/icinga/icinga2/issues/6712) (API): Downtime name not returned when error occurs
* [#6711](https://github.com/icinga/icinga2/issues/6711) (API, Cluster): Slow API \(TLS-Handshake\)
* [#6709](https://github.com/icinga/icinga2/issues/6709) (PR):  Fix the Icinga2 version check for versions with more than 5 characters
* [#6707](https://github.com/icinga/icinga2/issues/6707) (Compat, PR): Fix regression for wrong objects.cache path overwriting icinga2.debug file
* [#6705](https://github.com/icinga/icinga2/issues/6705) (CLI, Compat, Configuration): Crash "icinga2 object list" command with 2.10.1-1 on CentOS 7
* [#6703](https://github.com/icinga/icinga2/issues/6703): Check command 'icinga' breaks when vars.icinga\_min\_version is defined \(2.10.x\)
* [#6635](https://github.com/icinga/icinga2/issues/6635) (API): API TLS session connection closed after 2 requests
* [#5876](https://github.com/icinga/icinga2/issues/5876) (DB IDO): IDO Work queue on the inactive node growing when switching connection between redundant master servers

### Documentation

* [#6714](https://github.com/icinga/icinga2/issues/6714) (Documentation, PR): Docs: Add package related changes to the upgrading docs

### Support

* [#6773](https://github.com/icinga/icinga2/issues/6773) (Installation, Packages, PR): Initialize ICINGA2\_ERROR\_LOG inside the systemd environment
* [#6771](https://github.com/icinga/icinga2/issues/6771) (Tests, PR): Implement unit tests for Dictionary initializers
* [#6760](https://github.com/icinga/icinga2/issues/6760) (Packages, Tests, PR): armhf: Apply workaround for timer tests with std::bind callbacks
* [#6710](https://github.com/icinga/icinga2/issues/6710) (Packages): Crash when upgrading from 2.10.0 to 2.10.1 \(SELinux related\)

## 2.10.1 (2018-10-18)

### Bug

* [#6696](https://github.com/icinga/icinga2/issues/6696) (PR): Remove default environment, regression from e678fa1aa5
* [#6694](https://github.com/icinga/icinga2/issues/6694): v2.10.0 sets a default environment "production" in SNI
* [#6691](https://github.com/icinga/icinga2/issues/6691) (PR): Add missing shutdown/program state dumps for SIGUSR2 reload handler
* [#6689](https://github.com/icinga/icinga2/issues/6689): State file not updated on reload
* [#6685](https://github.com/icinga/icinga2/issues/6685) (API, PR): Fix regression with API permission filters and namespaces in v2.10
* [#6682](https://github.com/icinga/icinga2/issues/6682) (API): API process-check-result fails in 2.10.0
* [#6679](https://github.com/icinga/icinga2/issues/6679) (Windows, PR): Initialize Configuration::InitRunDir for Windows and writing the PID file
* [#6624](https://github.com/icinga/icinga2/issues/6624) (Check Execution): Master Reload Causes Passive Check State Change
* [#6592](https://github.com/icinga/icinga2/issues/6592): Reloads seem to reset the check atempt count. Also notifications go missing shortly after a reload.

### Documentation

* [#6701](https://github.com/icinga/icinga2/issues/6701) (Documentation, PR): Add GitHub release tag to README
* [#6700](https://github.com/icinga/icinga2/issues/6700) (Documentation, PR): Enhance the addon chapter in the docs
* [#6699](https://github.com/icinga/icinga2/issues/6699) (Documentation, PR): Update to https://icinga.com/
* [#6692](https://github.com/icinga/icinga2/issues/6692) (Documentation, PR): Update release docs for Chocolatey
* [#6690](https://github.com/icinga/icinga2/issues/6690) (Documentation, PR): Extend 09-object-types.md with argument array
* [#6674](https://github.com/icinga/icinga2/issues/6674) (Documentation, PR): Add a note to the docs on \>2 endpoints in a zone
* [#6673](https://github.com/icinga/icinga2/issues/6673) (Documentation, PR): Update RELEASE docs
* [#6672](https://github.com/icinga/icinga2/issues/6672) (Documentation, PR): Extend upgrade docs
* [#6671](https://github.com/icinga/icinga2/issues/6671) (Documentation): Zone requirements changed in 2.10 - Undocumented Change

### Support

* [#6681](https://github.com/icinga/icinga2/issues/6681) (code-quality, PR): Fix spelling errors.
* [#6677](https://github.com/icinga/icinga2/issues/6677) (Packages, Windows): icinga does not start after Update to 2.10

## 2.10.0 (2018-10-11)

### Notes

* Support for namespaces, details in [this blogpost](https://icinga.com/2018/09/17/icinga-2-dsl-feature-namespaces-coming-in-v2-10/)
* Only send acknowledgement notification to users notified about a problem before, thanks for sponsoring to the [Max-Planck-Institut for Marine Mikrobiologie](https://www.mpi-bremen.de)
* More child options for scheduled downtimes
* Performance improvements and fixes for the TLS connections inside cluster/REST API
* Better logging for HTTP requests and less verbose object creation (e.g. downtimes via Icinga Web 2 & REST API)
* New configuration path constants, e.g. ConfigDir
* Fixed problem with dependencies rescheduling parent checks too fast
* Fixed problem with logging in systemd and syslog
* Improved vim syntax highlighting
* [Technical concepts docs](https://icinga.com/docs/icinga2/latest/doc/19-technical-concepts/) update with config compiler and TLS network IO

### Enhancement

* [#6663](https://github.com/icinga/icinga2/issues/6663) (API, Log, PR): Silence config compiler logging for runtime created objects
* [#6657](https://github.com/icinga/icinga2/issues/6657) (API, Log, PR): Enable the HTTP request body debug log entry for release builds
* [#6655](https://github.com/icinga/icinga2/issues/6655) (API, Log, PR): Improve logging for disconnected HTTP clients
* [#6651](https://github.com/icinga/icinga2/issues/6651) (Plugins, PR): Add 'used' feature to check\_swap
* [#6633](https://github.com/icinga/icinga2/issues/6633) (API, Cluster, PR): Use a dynamic thread pool for API connections
* [#6632](https://github.com/icinga/icinga2/issues/6632) (Cluster, PR): Increase the cluster reconnect frequency to 10s
* [#6616](https://github.com/icinga/icinga2/issues/6616) (API, Cluster, PR): Add ApiListener\#tls\_handshake\_timeout option
* [#6611](https://github.com/icinga/icinga2/issues/6611) (Notifications): Allow types = \[ Recovery \] to always send recovery notifications 
* [#6595](https://github.com/icinga/icinga2/issues/6595) (API, Cluster, PR): Allow to configure anonymous clients limit inside the ApiListener object
* [#6532](https://github.com/icinga/icinga2/issues/6532) (Configuration, PR): Add child\_options to ScheduledDowntime
* [#6531](https://github.com/icinga/icinga2/issues/6531) (API, PR): Expose Zone\#all\_parents via API
* [#6527](https://github.com/icinga/icinga2/issues/6527) (Notifications, PR): Acknowledgment notifications should only be send if problem notification has been send
* [#6521](https://github.com/icinga/icinga2/issues/6521) (Configuration, PR): Implement references
* [#6512](https://github.com/icinga/icinga2/issues/6512) (Cluster, PR): Refactor environment for API connections
* [#6511](https://github.com/icinga/icinga2/issues/6511) (Cluster, PR): ApiListener: Add support for dynamic port handling
* [#6509](https://github.com/icinga/icinga2/issues/6509) (Configuration, PR): Implement support for namespaces
* [#6508](https://github.com/icinga/icinga2/issues/6508) (Configuration, PR): Implement the Dictionary\#clear script function
* [#6506](https://github.com/icinga/icinga2/issues/6506) (PR): Improve path handling in cmake and daemon
* [#6460](https://github.com/icinga/icinga2/issues/6460) (Log, help wanted): Feature suggestion: Do not log warnings when env elements are undefined in CheckCommand objects
* [#6455](https://github.com/icinga/icinga2/issues/6455) (Log, PR): Log something when the Filelogger has been started
* [#6379](https://github.com/icinga/icinga2/issues/6379) (Configuration, PR): Throw config error when using global zones as parent
* [#6356](https://github.com/icinga/icinga2/issues/6356) (Log, PR): Fix logging under systemd
* [#6339](https://github.com/icinga/icinga2/issues/6339) (Log, help wanted): On systemd, icinga2 floods the system log, and this cannot simply be opted out of
* [#6110](https://github.com/icinga/icinga2/issues/6110) (Configuration, PR): Implement support for optionally specifying the 'var' keyword in 'for' loops
* [#6047](https://github.com/icinga/icinga2/issues/6047) (Notifications): Acknowledgment notifications should only be sent if the user already received a problem notification
* [#4282](https://github.com/icinga/icinga2/issues/4282) (API, Log): Icinga should log HTTP bodies for API requests

### Bug

* [#6658](https://github.com/icinga/icinga2/issues/6658) (API, PR): Ensure that HTTP/1.0 or 'Connection: close' headers are properly disconnecting the client
* [#6652](https://github.com/icinga/icinga2/issues/6652) (Plugins, PR): Fix check\_memory thresholds in 'used' mode
* [#6647](https://github.com/icinga/icinga2/issues/6647) (CLI, PR): node setup: always respect --accept-config and --accept-commands
* [#6643](https://github.com/icinga/icinga2/issues/6643) (Check Execution, Notifications, PR): Fix that check\_timeout was used for Event/Notification commands too
* [#6639](https://github.com/icinga/icinga2/issues/6639) (Windows, PR): Ensure to \_unlink before renaming replay log on Windows
* [#6622](https://github.com/icinga/icinga2/issues/6622) (DB IDO, PR): Ensure to use UTC timestamps for IDO PgSQL cleanup queries
* [#6603](https://github.com/icinga/icinga2/issues/6603) (Check Execution, Cluster): CheckCommand 'icinga' seems to ignore retry interval via command\_endpoint
* [#6575](https://github.com/icinga/icinga2/issues/6575): LTO builds fail on Linux
* [#6566](https://github.com/icinga/icinga2/issues/6566) (Cluster): Master disconnects during signing process
* [#6546](https://github.com/icinga/icinga2/issues/6546) (API, CLI, PR): Overridden path constants not passed to config validation in /v1/config/stages API call
* [#6530](https://github.com/icinga/icinga2/issues/6530) (DB IDO, PR): IDO/MySQL: avoid empty queries
* [#6519](https://github.com/icinga/icinga2/issues/6519) (CLI, PR): Reset terminal on erroneous console exit
* [#6517](https://github.com/icinga/icinga2/issues/6517) (Cluster): Not all Endpoints can't reconnect due to "Client TLS handshake failed" error after "reload or restart"
* [#6514](https://github.com/icinga/icinga2/issues/6514) (API): API using "Connection: close" header results in infinite threads
* [#6507](https://github.com/icinga/icinga2/issues/6507) (Cluster): Variable name conflict in constants.conf / Problem with TLS verification, CN and Environment variable
* [#6503](https://github.com/icinga/icinga2/issues/6503) (Log, PR): Reduce the log level for missing env macros to debug
* [#6485](https://github.com/icinga/icinga2/issues/6485) (Log): Icinga logs discarding messages still as warning and not as notice
* [#6475](https://github.com/icinga/icinga2/issues/6475) (Compat, PR): lib-\>compat-\>statusdatawriter: fix notifications\_enabled
* [#6430](https://github.com/icinga/icinga2/issues/6430) (Log, PR): Fix negative 'empty in' value in WorkQueue log message
* [#6427](https://github.com/icinga/icinga2/issues/6427) (Configuration, Crash, PR): Improve error message for serializing objects with recursive references
* [#6409](https://github.com/icinga/icinga2/issues/6409) (Configuration, Crash): Assigning vars.x = vars causes Icinga 2 segfaults
* [#6408](https://github.com/icinga/icinga2/issues/6408) (PR): ObjectLock\#Unlock\(\): don't reset m\_Object-\>m\_LockOwner too early
* [#6386](https://github.com/icinga/icinga2/issues/6386) (Configuration, PR): Fix that TimePeriod segments are not cleared on restart
* [#6382](https://github.com/icinga/icinga2/issues/6382) (CLI, help wanted): icinga2 console breaks the terminal on errors
* [#6313](https://github.com/icinga/icinga2/issues/6313) (Plugins, Windows, PR): Fix wrong calculation of check\_swap windows plugin
* [#6304](https://github.com/icinga/icinga2/issues/6304) (Configuration, Notifications): Timeout defined in NotificationCommand is ignored and uses check\_timeout
* [#5815](https://github.com/icinga/icinga2/issues/5815) (Plugins, Windows): swap-windows check delivers wrong result
* [#5375](https://github.com/icinga/icinga2/issues/5375) (Check Execution, PR): Parents who are non-active should not be rescheduled
* [#5052](https://github.com/icinga/icinga2/issues/5052) (Cluster, Windows): Replay log not working with Windows client
* [#5022](https://github.com/icinga/icinga2/issues/5022) (Check Execution): Dependencies may reschedule passive checks, triggering freshness checks

### ITL

* [#6646](https://github.com/icinga/icinga2/issues/6646) (ITL, PR): Update ITL and Docs for memory-windows - show used
* [#6640](https://github.com/icinga/icinga2/issues/6640) (ITL): Update ITL and Docs for memory-windows - show used
* [#6563](https://github.com/icinga/icinga2/issues/6563) (ITL, PR): \[Feature\] Cloudera service health CheckCommand
* [#6561](https://github.com/icinga/icinga2/issues/6561) (ITL, PR): \[Feature\] Ceph health CheckCommand
* [#6504](https://github.com/icinga/icinga2/issues/6504) (ITL, PR): squashfs ignored
* [#6491](https://github.com/icinga/icinga2/issues/6491) (ITL, PR): Feature/itl vmware health
* [#6481](https://github.com/icinga/icinga2/issues/6481) (ITL): command-plugins.conf check\_disk exclude squashfs

### Documentation

* [#6670](https://github.com/icinga/icinga2/issues/6670) (Documentation, PR): Add technical concepts for the config compiler and daemon CLI command
* [#6665](https://github.com/icinga/icinga2/issues/6665) (Documentation, PR): Make the two modes of check\_http more obvious.
* [#6615](https://github.com/icinga/icinga2/issues/6615) (Documentation, PR): Update distributed monitoring docs for 2.10
* [#6610](https://github.com/icinga/icinga2/issues/6610) (Documentation, PR): Add "TLS Network IO" into technical concepts docs
* [#6607](https://github.com/icinga/icinga2/issues/6607) (Documentation, PR): Enhance development docs with GDB backtrace and thread list
* [#6606](https://github.com/icinga/icinga2/issues/6606) (Documentation, PR): Enhance contributing docs
* [#6598](https://github.com/icinga/icinga2/issues/6598) (Documentation, PR): doc/09-object-types: states filter ignored for Acknowledgements
* [#6597](https://github.com/icinga/icinga2/issues/6597) (Documentation, PR): Add Fedora to development docs for debuginfo packages
* [#6593](https://github.com/icinga/icinga2/issues/6593) (Documentation, help wanted): Include CA Proxy in 3rd scenario in Distributed Monitoring docs
* [#6573](https://github.com/icinga/icinga2/issues/6573) (Documentation, PR): Fix operator precedence table
* [#6528](https://github.com/icinga/icinga2/issues/6528) (Documentation, PR): Document default of User\#enable\_notifications
* [#6502](https://github.com/icinga/icinga2/issues/6502) (Documentation, PR): Update 17-language-reference.md
* [#6501](https://github.com/icinga/icinga2/issues/6501) (Documentation, PR): Update 03-monitoring-basics.md
* [#6488](https://github.com/icinga/icinga2/issues/6488) (Documentation, ITL, PR): Fix typo with the CheckCommand cert

### Support

* [#6669](https://github.com/icinga/icinga2/issues/6669) (PR): Don't throw an error when namespace indexers don't find a valid key
* [#6668](https://github.com/icinga/icinga2/issues/6668) (Installation, PR): Enhance vim syntax highlighting for 2.10
* [#6661](https://github.com/icinga/icinga2/issues/6661) (API, Log, code-quality, PR): Cache the peer address in the HTTP server
* [#6642](https://github.com/icinga/icinga2/issues/6642) (PR): Allow to override MaxConcurrentChecks constant
* [#6621](https://github.com/icinga/icinga2/issues/6621) (code-quality, PR): Remove unused timestamp function in DB IDO
* [#6618](https://github.com/icinga/icinga2/issues/6618) (PR): Silence compiler warning for nice\(\)
* [#6591](https://github.com/icinga/icinga2/issues/6591) (PR): Fix static initializer priority for namespaces in LTO builds
* [#6588](https://github.com/icinga/icinga2/issues/6588) (PR): Fix using full path in prepare-dirs/safe-reload scripts
* [#6586](https://github.com/icinga/icinga2/issues/6586) (PR): Fix non-unity builds on CentOS 7 with std::shared\_ptr
* [#6583](https://github.com/icinga/icinga2/issues/6583) (Documentation, Installation, PR): Update PostgreSQL library path variable in INSTALL.md
* [#6574](https://github.com/icinga/icinga2/issues/6574) (PR): Move new downtime constants into the Icinga namespace
* [#6570](https://github.com/icinga/icinga2/issues/6570) (Cluster, PR): Increase limit for simultaneously connected anonymous TLS clients
* [#6567](https://github.com/icinga/icinga2/issues/6567) (PR): ApiListener: Dump the state file port detail as number
* [#6556](https://github.com/icinga/icinga2/issues/6556) (Installation, Windows, PR): windows: Allow suppression of extra actions in the MSI package
* [#6544](https://github.com/icinga/icinga2/issues/6544) (code-quality, PR): Remove \#include for deprecated header file
* [#6539](https://github.com/icinga/icinga2/issues/6539) (PR): Build fix for CentOS 7 and non-unity builds
* [#6526](https://github.com/icinga/icinga2/issues/6526) (code-quality, PR): icinga::PackObject\(\): shorten conversion to string
* [#6510](https://github.com/icinga/icinga2/issues/6510) (Tests, Windows, PR): Update windows build scripts
* [#6494](https://github.com/icinga/icinga2/issues/6494) (Tests, PR): Test PackObject
* [#6489](https://github.com/icinga/icinga2/issues/6489) (code-quality, PR): Implement object packer for consistent hashing
* [#6484](https://github.com/icinga/icinga2/issues/6484) (Packages): Packages from https://packages.icinga.com are not Systemd Type=notify enabled?
* [#6469](https://github.com/icinga/icinga2/issues/6469) (Installation, Windows, PR): Fix Windows Agent resize behavior
* [#6458](https://github.com/icinga/icinga2/issues/6458) (code-quality, PR): Fix debug build log entry for ConfigItem activation priority
* [#6456](https://github.com/icinga/icinga2/issues/6456) (code-quality, PR): Keep notes for immediately log flushing
* [#6440](https://github.com/icinga/icinga2/issues/6440) (code-quality, PR): Fix typo
* [#6410](https://github.com/icinga/icinga2/issues/6410) (code-quality, PR): Remove unused code
* [#4959](https://github.com/icinga/icinga2/issues/4959) (Installation, Windows): Windows Agent Wizard Window resizes with screen, hiding buttons

## 2.9.3 (2019-07-30)

[Issue and PRs](https://github.com/Icinga/icinga2/issues?utf8=%E2%9C%93&q=milestone%3A2.9.3)

### Bugfixes

* Fix el7 not loading ECDHE cipher suites #7247
* Fix checkresults from the future breaking checks #6797 ref/NC/595861
* DB IDO: Don't enqueue queries when the feature is paused (HA) #5876

## 2.9.2 (2018-09-26)

### Enhancement

* [#6602](https://github.com/icinga/icinga2/issues/6602) (API, Cluster, PR): Improve TLS handshake exception logging
* [#6568](https://github.com/icinga/icinga2/issues/6568) (Configuration, PR): Ensure that config object types are committed in dependent load order
* [#6497](https://github.com/icinga/icinga2/issues/6497) (Configuration, PR): Improve error logging for match/regex/cidr\_match functions and unsupported dictionary usage

### Bug

* [#6596](https://github.com/icinga/icinga2/issues/6596) (Crash, PR): Fix crash on API queries with Fedora 28 hardening and GCC 8
* [#6581](https://github.com/icinga/icinga2/issues/6581) (Configuration, PR): Shuffle items before config validation
* [#6569](https://github.com/icinga/icinga2/issues/6569) (DB IDO): Custom Vars not updated after upgrade
* [#6533](https://github.com/icinga/icinga2/issues/6533) (Crash): Icinga2 crashes after using some api-commands on Fedora 28
* [#6505](https://github.com/icinga/icinga2/issues/6505) (Cluster, PR): Fix clusterzonecheck if not connected
* [#6498](https://github.com/icinga/icinga2/issues/6498) (Configuration, PR): Fix regression with MatchAny false conditions on match/regex/cidr\_match
* [#6496](https://github.com/icinga/icinga2/issues/6496) (Configuration): error with match and type matchany

### Documentation

* [#6590](https://github.com/icinga/icinga2/issues/6590) (DB IDO, Documentation, PR): Update workaround for custom vars
* [#6572](https://github.com/icinga/icinga2/issues/6572) (Documentation, PR): Add note about workaround for broken custom vars

### Support

* [#6540](https://github.com/icinga/icinga2/issues/6540) (Configuration): Evaluate a fixed config compiler commit order
* [#6486](https://github.com/icinga/icinga2/issues/6486) (Configuration): Configuration validation w/ ScheduledDowntimes performance decreased in 2.9
* [#6442](https://github.com/icinga/icinga2/issues/6442) (Configuration): Error while evaluating "assign where match" expression: std::bad\_cast

## 2.9.1 (2018-07-24)

### Bug

* [#6457](https://github.com/icinga/icinga2/issues/6457) (PR): Ensure that timer thread is initialized after Daemonize\(\)
* [#6449](https://github.com/icinga/icinga2/issues/6449): icinga r2.9.0-1 init.d script overrides PATH variable
* [#6445](https://github.com/icinga/icinga2/issues/6445): Problem with daemonize \(init scripts, -d\) on Debian 8 / CentOS 6 / Ubuntu 14 / SLES 11 in 2.9
* [#6444](https://github.com/icinga/icinga2/issues/6444) (PR): SELinux: allow systemd notify
* [#6443](https://github.com/icinga/icinga2/issues/6443): selinux and 2.9

### Support

* [#6470](https://github.com/icinga/icinga2/issues/6470) (code-quality, PR): Fix spelling errors.
* [#6467](https://github.com/icinga/icinga2/issues/6467) (Tests, PR): Start and stop the timer thread lazily
* [#6461](https://github.com/icinga/icinga2/issues/6461) (Tests): Broken tests with fix from \#6457
* [#6451](https://github.com/icinga/icinga2/issues/6451) (Packages, PR): Fix initscripts
* [#6450](https://github.com/icinga/icinga2/issues/6450) (Packages): init script helpers - source: not found

## 2.9.0 (2018-07-17)

### Notes

- Elasticsearch 6 Support
- icinga health check supports minimum version parameter, ido thresholds for query rate, dummy check is executed in-memory, avoids plugin call
- `ApplicationVersion` constant in the configuration
- Setup wizards: global zone, disable conf.d inclusion, unified parameter handling
- TTL support for check results, pretty formatting for REST API queries
- TLS support for IDO PostgreSQL
- Improvements for check scheduling, concurrent checks with command endpoints, downtime notification handling, scheduled downtimes and memory handling with many API requests

### Enhancement

* [#6400](https://github.com/icinga/icinga2/issues/6400) (Plugins, Windows, PR): Enhance debug logging for check\_nscp\_api
* [#6321](https://github.com/icinga/icinga2/issues/6321) (Log, PR): Update log message for skipped certificate renewal
* [#6305](https://github.com/icinga/icinga2/issues/6305) (PR): Introduce the 'Environment' variable
* [#6299](https://github.com/icinga/icinga2/issues/6299) (Check Execution, Log, PR): Change log level for failed event command execution
* [#6285](https://github.com/icinga/icinga2/issues/6285) (CLI, Log, PR): Add support for config validation log timestamps
* [#6270](https://github.com/icinga/icinga2/issues/6270) (Configuration, PR): Add activation priority for config object types
* [#6236](https://github.com/icinga/icinga2/issues/6236) (DB IDO, PR): Add TLS support for DB IDO PostgreSQL feature
* [#6219](https://github.com/icinga/icinga2/issues/6219) (Elasticsearch, PR): Add support for Elasticsearch 6
* [#6211](https://github.com/icinga/icinga2/issues/6211) (DB IDO): IDO pgsql with TLS support
* [#6209](https://github.com/icinga/icinga2/issues/6209) (CLI, PR): Unify zone name settings in node setup/wizard; add connection-less mode for node setup
* [#6208](https://github.com/icinga/icinga2/issues/6208) (CLI): Add connection-less support for node setup CLI command
* [#6206](https://github.com/icinga/icinga2/issues/6206) (Configuration, PR): Add ApplicationVersion built-in constant
* [#6205](https://github.com/icinga/icinga2/issues/6205) (API, PR): API: Unify verbose error messages
* [#6194](https://github.com/icinga/icinga2/issues/6194) (Elasticsearch, Graylog, PR): Elasticsearch/GELF: Add metric unit to performance data fields
* [#6170](https://github.com/icinga/icinga2/issues/6170) (Configuration, Windows, PR): Add option to windows installer to add global zones
* [#6158](https://github.com/icinga/icinga2/issues/6158) (API, Log): Review API debugging: verboseErrors and diagnostic information
* [#6136](https://github.com/icinga/icinga2/issues/6136) (Check Execution, PR): Add counter for current concurrent checks to Icinga check
* [#6131](https://github.com/icinga/icinga2/issues/6131) (Log, PR): Log which ticket was invalid on the master
* [#6109](https://github.com/icinga/icinga2/issues/6109) (Plugins, PR): Add 'used' feature to check\_memory
* [#6090](https://github.com/icinga/icinga2/issues/6090) (Notifications, PR): Fixed URL encoding for HOSTNAME and SERVICENAME in mail notification
* [#6078](https://github.com/icinga/icinga2/issues/6078) (Check Execution, PR): Add more metrics and details to built-in 'random' check
* [#6039](https://github.com/icinga/icinga2/issues/6039) (Configuration, PR): Improve location info for some error messages
* [#6033](https://github.com/icinga/icinga2/issues/6033) (Compat): Deprecate StatusDataWriter
* [#6032](https://github.com/icinga/icinga2/issues/6032) (Compat): Deprecate CompatLogger
* [#6010](https://github.com/icinga/icinga2/issues/6010) (Cluster, PR): Move the endpoint list into a new line for the 'cluster' check
* [#5996](https://github.com/icinga/icinga2/issues/5996) (PR): Add systemd watchdog and adjust reload behaviour
* [#5985](https://github.com/icinga/icinga2/issues/5985) (DB IDO, PR): Add query thresholds for the 'ido' check: Rate and pending queries
* [#5979](https://github.com/icinga/icinga2/issues/5979) (CLI, PR): Add quit, exit and help
* [#5973](https://github.com/icinga/icinga2/issues/5973) (API, Check Execution, PR): Add 'ttl' support for check result freshness via REST API
* [#5959](https://github.com/icinga/icinga2/issues/5959) (API, PR): API: Add 'pretty' parameter for beautified JSON response bodies
* [#5905](https://github.com/icinga/icinga2/issues/5905) (Elasticsearch): Add support for Elasticsearch 6
* [#5888](https://github.com/icinga/icinga2/issues/5888) (DB IDO, PR): FindMySQL: Support mariadbclient implementation
* [#5877](https://github.com/icinga/icinga2/issues/5877) (API): Add pretty format to REST API parameters \(for debugging\)
* [#5811](https://github.com/icinga/icinga2/issues/5811) (CLI, PR): Update NodeName/ZoneName constants with 'api setup'
* [#5767](https://github.com/icinga/icinga2/issues/5767) (CLI, PR): Implement ability to make global zones configurable during node wizard/setup
* [#5733](https://github.com/icinga/icinga2/issues/5733) (Plugins, Windows, PR): Make --perf-syntax also change short message
* [#5729](https://github.com/icinga/icinga2/issues/5729) (CLI, Cluster, PR): Correct node wizard output formatting
* [#5675](https://github.com/icinga/icinga2/issues/5675) (InfluxDB, PR): Add pdv unit to influxdbwriter if not empty + doc
* [#5627](https://github.com/icinga/icinga2/issues/5627) (InfluxDB, Metrics): InfluxDBWriter: Send metric unit \(perfdata\)
* [#5605](https://github.com/icinga/icinga2/issues/5605) (CLI, Cluster, Configuration): Disable conf.d inclusion in node setup wizards
* [#5509](https://github.com/icinga/icinga2/issues/5509) (Cluster, wishlist): Add metrics about communication between endpoints
* [#5444](https://github.com/icinga/icinga2/issues/5444) (Cluster): Display endpoints in the second line of the ClusterCheckTask output
* [#5426](https://github.com/icinga/icinga2/issues/5426) (CLI, Configuration, PR): Add the ability to disable the conf.d inclusion through the node wizard
* [#5418](https://github.com/icinga/icinga2/issues/5418) (Plugins, Windows): Feature request: check\_perfmon.exe - Change name of counter in output
* [#4966](https://github.com/icinga/icinga2/issues/4966) (CLI, Cluster): Unify setting of master zones name
* [#4508](https://github.com/icinga/icinga2/issues/4508) (CLI): node wizard/setup: allow to disable conf.d inclusion
* [#3455](https://github.com/icinga/icinga2/issues/3455) (API, Log): startup.log in stage dir has no timestamps
* [#3245](https://github.com/icinga/icinga2/issues/3245) (CLI, help wanted, wishlist): Add option to Windows installer to add global zone during setup
* [#2287](https://github.com/icinga/icinga2/issues/2287) (help wanted, wishlist): Please support systemd startup notification

### Bug

* [#6429](https://github.com/icinga/icinga2/issues/6429) (PR): Make HttpServerConnection\#m\_DataHandlerMutex a boost::recursive\_mutex
* [#6428](https://github.com/icinga/icinga2/issues/6428) (API): Director kickstart wizard querying the API results in TLS stream disconnected infinite loop
* [#6411](https://github.com/icinga/icinga2/issues/6411) (Plugins, Windows, PR): Windows: Conform to the Plugin API spec for performance label quoting
* [#6407](https://github.com/icinga/icinga2/issues/6407) (Windows, PR): Fix wrong UOM in check\_uptime windows plugin
* [#6405](https://github.com/icinga/icinga2/issues/6405) (Windows, PR): TcpSocket\#Bind\(\): reuse socket addresses on Windows, too
* [#6403](https://github.com/icinga/icinga2/issues/6403) (API, PR): Conform to RFC for CRLF in HTTP requests
* [#6401](https://github.com/icinga/icinga2/issues/6401) (Elasticsearch, InfluxDB, PR): Fix connection error handling in Elasticsearch and InfluxDB features
* [#6397](https://github.com/icinga/icinga2/issues/6397) (Plugins, Windows, PR): TlsStream\#IsEof\(\): fix false positive EOF indicator
* [#6394](https://github.com/icinga/icinga2/issues/6394) (Crash, Elasticsearch): Icinga will throw an exception, if ElasticSearch is not reachable
* [#6393](https://github.com/icinga/icinga2/issues/6393) (API, Elasticsearch, PR): Stream\#ReadLine\(\): fix false positive buffer underflow indicator
* [#6387](https://github.com/icinga/icinga2/issues/6387) (Configuration, Crash, Windows, PR): Remove ApiUser password\_hash functionality
* [#6383](https://github.com/icinga/icinga2/issues/6383) (API, CLI, PR): HttpRequest\#ParseBody\(\): indicate success on complete body
* [#6378](https://github.com/icinga/icinga2/issues/6378) (Windows): Analyze Windows reload behaviour
* [#6371](https://github.com/icinga/icinga2/issues/6371) (API, Cluster, PR): ApiListener\#NewClientHandlerInternal\(\): Explicitly close the TLS stream on any failure
* [#6368](https://github.com/icinga/icinga2/issues/6368) (CLI, PR): Fix program option parsing
* [#6365](https://github.com/icinga/icinga2/issues/6365) (CLI): Different behavior between `icinga2 -V` and `icinga2 --version`
* [#6355](https://github.com/icinga/icinga2/issues/6355) (API): HTTP header size too low: Long URLs and session cookies cause bad requests
* [#6354](https://github.com/icinga/icinga2/issues/6354) (Elasticsearch): ElasticsearchWriter not writing to ES
* [#6336](https://github.com/icinga/icinga2/issues/6336) (Log, PR): Fix unnecessary blank in log message
* [#6324](https://github.com/icinga/icinga2/issues/6324) (Crash, PR): Ensure that password hash generation from OpenSSL is atomic
* [#6319](https://github.com/icinga/icinga2/issues/6319) (Windows): Windows service restart fails and config validate runs forever
* [#6297](https://github.com/icinga/icinga2/issues/6297) (Cluster, PR): Execute event commands only on actively checked host/service objects in an HA zone
* [#6294](https://github.com/icinga/icinga2/issues/6294) (API, Configuration, PR): Ensure that group memberships on API object creation are unique
* [#6292](https://github.com/icinga/icinga2/issues/6292) (Notifications, PR): Fix problem with reminder notifications if the checkable is flapping
* [#6290](https://github.com/icinga/icinga2/issues/6290) (OpenTSDB, PR): Fixed opentsdb metric name with colon chars
* [#6282](https://github.com/icinga/icinga2/issues/6282) (Configuration): Issue when using excludes in TimePeriod Objects
* [#6279](https://github.com/icinga/icinga2/issues/6279) (Crash): segfault with sha1\_block\_data\_order\_avx of libcrypto
* [#6255](https://github.com/icinga/icinga2/issues/6255) (Configuration): On debian based systems /etc/default/icinga2 is not read/used
* [#6242](https://github.com/icinga/icinga2/issues/6242) (Plugins, Windows): Sporadic check\_nscp\_api timeouts
* [#6239](https://github.com/icinga/icinga2/issues/6239) (Plugins, Windows, PR): Fix Windows check\_memory rounding
* [#6231](https://github.com/icinga/icinga2/issues/6231) (Notifications): icinga2.8 - Notifications are sent even in downtime
* [#6218](https://github.com/icinga/icinga2/issues/6218) (PR): attempt to fix issue \#5277
* [#6217](https://github.com/icinga/icinga2/issues/6217) (Check Execution, PR): Fix check behavior on restart
* [#6204](https://github.com/icinga/icinga2/issues/6204) (API, PR): API: Check if objects exists and return proper error message
* [#6195](https://github.com/icinga/icinga2/issues/6195) (API, Crash, PR): Fix crash in remote api console
* [#6193](https://github.com/icinga/icinga2/issues/6193) (Crash, Graylog, PR): GelfWriter: Fix crash on invalid performance data metrics
* [#6184](https://github.com/icinga/icinga2/issues/6184) (API): debug console with API connection sometimes hangs since 2.8.2
* [#6125](https://github.com/icinga/icinga2/issues/6125) (Configuration, PR): Fix description of the NotificationComponent in notification.conf
* [#6077](https://github.com/icinga/icinga2/issues/6077) (API, PR): Allow to pass raw performance data in 'process-check-result' API action
* [#6057](https://github.com/icinga/icinga2/issues/6057) (Notifications): Icinga2 sends notifications without logging about it and despite having a downtime
* [#6020](https://github.com/icinga/icinga2/issues/6020) (CLI, PR): Fix crash when running 'icinga2 console' without HOME environment variable
* [#6019](https://github.com/icinga/icinga2/issues/6019): icinga2 console -r crashes when run without a HOME environment variable
* [#6016](https://github.com/icinga/icinga2/issues/6016) (Notifications, PR): Check notification state filters for problems only, not for Custom, etc.
* [#5988](https://github.com/icinga/icinga2/issues/5988) (Check Execution, Cluster, PR): Fix concurrent checks limit while using command\_endpoint
* [#5964](https://github.com/icinga/icinga2/issues/5964) (Metrics, OpenTSDB, PR): OpenTSDB writer - Fix function for escaping host tag chars.
* [#5963](https://github.com/icinga/icinga2/issues/5963) (Metrics, OpenTSDB): OpenTSDB writer is escaping wrong chars for host names.
* [#5952](https://github.com/icinga/icinga2/issues/5952) (Notifications): Custom notifications are filtered by object state
* [#5940](https://github.com/icinga/icinga2/issues/5940) (PR): Remove deprecated Chocolatey functions
* [#5928](https://github.com/icinga/icinga2/issues/5928) (PR): Fix build problem with MSVC
* [#5908](https://github.com/icinga/icinga2/issues/5908) (Windows): Icinga2 fails to build on Windows
* [#5901](https://github.com/icinga/icinga2/issues/5901) (PR): Do not replace colons in plugin output
* [#5885](https://github.com/icinga/icinga2/issues/5885) (PR): Workaround for GCC bug 61321
* [#5884](https://github.com/icinga/icinga2/issues/5884): Icinga2 fails to build
* [#5872](https://github.com/icinga/icinga2/issues/5872) (PR): Replace incorrect fclose\(\) call with pclose\(\)
* [#5863](https://github.com/icinga/icinga2/issues/5863) (PR): Fix glob error handling
* [#5861](https://github.com/icinga/icinga2/issues/5861) (PR): Fix incorrect memory access
* [#5860](https://github.com/icinga/icinga2/issues/5860) (PR): Fix memory leaks in the unit tests
* [#5853](https://github.com/icinga/icinga2/issues/5853) (Plugins, Windows, PR): Fix missing space in check\_service output
* [#5840](https://github.com/icinga/icinga2/issues/5840) (Elasticsearch, PR): Fix newline terminator for bulk requests in ElasticsearchWriter
* [#5796](https://github.com/icinga/icinga2/issues/5796) (CLI, PR): Fix error reporting for 'icinga2 console -r'
* [#5795](https://github.com/icinga/icinga2/issues/5795) (Elasticsearch): ElasticsearchWriter gives "Unexpected response code 400" with Elasticsearch 6.x
* [#5763](https://github.com/icinga/icinga2/issues/5763) (API): "icinga2 api setup" should explicitly set the NodeName constant in constants.conf
* [#5753](https://github.com/icinga/icinga2/issues/5753) (API, Cluster, Metrics, PR): Fix that RingBuffer does not get updated and add metrics about communication between endpoints
* [#5718](https://github.com/icinga/icinga2/issues/5718) (API, PR): API: Fix http status codes
* [#5550](https://github.com/icinga/icinga2/issues/5550) (API): Verify error codes and returned log messages in API actions
* [#5277](https://github.com/icinga/icinga2/issues/5277) (Notifications): Flexible downtime is expired at end\_time, not trigger\_time+duration
* [#5095](https://github.com/icinga/icinga2/issues/5095) (API): Wrong HTTP status code when API request fails
* [#5083](https://github.com/icinga/icinga2/issues/5083) (Check Execution): Initial checks are not executed immediately
* [#4786](https://github.com/icinga/icinga2/issues/4786) (API): API: Command process-check-result fails if it contains performance data
* [#4785](https://github.com/icinga/icinga2/issues/4785) (Compat): Semicolons in plugin output are converted to colon
* [#4732](https://github.com/icinga/icinga2/issues/4732) (API, Configuration): Duplicate groups allowed when creating host
* [#4436](https://github.com/icinga/icinga2/issues/4436) (Check Execution): New objects not scheduled to check immediately
* [#4272](https://github.com/icinga/icinga2/issues/4272) (Cluster, Configuration): Duplicating downtime from ScheduledDowntime object on each restart
* [#3431](https://github.com/icinga/icinga2/issues/3431) (Cluster): Eventhandler trigger on all endpoints in high available zone 

### ITL

* [#6389](https://github.com/icinga/icinga2/issues/6389) (ITL, PR): New ITL command nscp-local-tasksched
* [#6348](https://github.com/icinga/icinga2/issues/6348) (ITL, PR): Fix for catalogued locally databases. Fixes \#6338
* [#6338](https://github.com/icinga/icinga2/issues/6338) (ITL): db2\_health not working with catalogued databases, as --hostname is always used
* [#6308](https://github.com/icinga/icinga2/issues/6308) (ITL, PR): Update lsi-raid ITL command
* [#6263](https://github.com/icinga/icinga2/issues/6263) (ITL, PR): ITL: Add default thresholds to windows check commands
* [#6139](https://github.com/icinga/icinga2/issues/6139) (ITL, PR): itl/disk: Ignore overlay and netfs filesystems
* [#6045](https://github.com/icinga/icinga2/issues/6045) (ITL, PR): Move the "passive" check command to command-icinga.conf
* [#6043](https://github.com/icinga/icinga2/issues/6043) (ITL): ITL "plugins" has an implicit dependency on "itl"
* [#6034](https://github.com/icinga/icinga2/issues/6034) (ITL, PR): ITL by\_ssh add -E parameter
* [#5958](https://github.com/icinga/icinga2/issues/5958) (ITL, PR): Add minimum version check to the built-in icinga command
* [#5954](https://github.com/icinga/icinga2/issues/5954) (ITL, PR): ITL: Add mongodb --authdb parameter support
* [#5951](https://github.com/icinga/icinga2/issues/5951) (ITL, PR): itl: Add command parameters for snmp-memory
* [#5921](https://github.com/icinga/icinga2/issues/5921) (ITL, PR): Add icingacli-director check to ITL
* [#5920](https://github.com/icinga/icinga2/issues/5920) (ITL): Add Check for Director Jobs to ITL
* [#5914](https://github.com/icinga/icinga2/issues/5914) (ITL, PR): Fix for wrong attribute in ITL mongodb CheckCommand
* [#5906](https://github.com/icinga/icinga2/issues/5906) (ITL, PR): Add check\_openmanage command to ITL.
* [#5902](https://github.com/icinga/icinga2/issues/5902) (ITL, PR): Add parameter --octetlength to snmp-storage command.
* [#5817](https://github.com/icinga/icinga2/issues/5817) (ITL): mongodb\_address vs mongodb\_host
* [#5812](https://github.com/icinga/icinga2/issues/5812) (ITL): Better way to check required parameters in notification scripts
* [#5805](https://github.com/icinga/icinga2/issues/5805) (ITL, PR): Add support for LD\_LIBRARY\_PATH env variable in oracle\_health ITL CheckCommand
* [#5792](https://github.com/icinga/icinga2/issues/5792) (ITL, PR): ITL: Add check\_rpc
* [#5787](https://github.com/icinga/icinga2/issues/5787) (Check Execution, ITL): random check should provide performance data metrics
* [#5744](https://github.com/icinga/icinga2/issues/5744) (Check Execution, ITL, PR): Implement DummyCheckTask and move dummy into embedded in-memory checks
* [#5717](https://github.com/icinga/icinga2/issues/5717) (ITL, PR): add order tags to disk check
* [#5714](https://github.com/icinga/icinga2/issues/5714) (ITL): disk check in icinga2/itl/command-plugins.conf lacks order tags
* [#5260](https://github.com/icinga/icinga2/issues/5260) (ITL): CheckCommand mongodb does not expose authdb option

### Documentation

* [#6436](https://github.com/icinga/icinga2/issues/6436) (Documentation, PR): Update tested Elasticsearch version
* [#6435](https://github.com/icinga/icinga2/issues/6435) (Documentation, PR): Add note on sysconfig shell variables for Systemd to the Upgrading docs
* [#6433](https://github.com/icinga/icinga2/issues/6433) (Documentation, PR): Docs: Fix typos in 03-monitoring-basics.md
* [#6426](https://github.com/icinga/icinga2/issues/6426) (Documentation, PR): Update 'Upgrading to 2.9' docs
* [#6413](https://github.com/icinga/icinga2/issues/6413) (Documentation, PR): Fix table in Livestatus Filters
* [#6391](https://github.com/icinga/icinga2/issues/6391) (Documentation, PR): Docs: Fix icinga.com link
* [#6390](https://github.com/icinga/icinga2/issues/6390) (Documentation, Windows, PR): Docs: Update Windows wizard images
* [#6375](https://github.com/icinga/icinga2/issues/6375) (Documentation, PR): some minor fixes in the flapping documentation
* [#6374](https://github.com/icinga/icinga2/issues/6374) (Documentation, PR): Docs: Add an additional note for VMWare timeouts on Ubuntu 16.04 LTS
* [#6373](https://github.com/icinga/icinga2/issues/6373) (Documentation, PR): Drop command template imports for versions \< 2.6 in the docs
* [#6372](https://github.com/icinga/icinga2/issues/6372) (Documentation, PR): Remove the import of 'legacy-timeperiod' in the docs
* [#6350](https://github.com/icinga/icinga2/issues/6350) (Documentation, PR): clarify the permision system of the api in the docs
* [#6344](https://github.com/icinga/icinga2/issues/6344) (Documentation, PR): README: Fix broken community link
* [#6330](https://github.com/icinga/icinga2/issues/6330) (Documentation, PR): Fix $ipaddress6$ attribute name typo in the docs
* [#6317](https://github.com/icinga/icinga2/issues/6317) (Documentation, PR): Add a note on Windows NSClient++ CPU checks to the docs
* [#6289](https://github.com/icinga/icinga2/issues/6289) (Documentation, PR): Update release documentation with git tag signing key configuration
* [#6286](https://github.com/icinga/icinga2/issues/6286) (Documentation): Update Windows wizard screenshots in the docs
* [#6283](https://github.com/icinga/icinga2/issues/6283) (Documentation, PR): edit Icinga license info so that GitHub recognizes it
* [#6271](https://github.com/icinga/icinga2/issues/6271) (Documentation, PR): Enhance advanced topics with \(scheduled\) downtimes
* [#6267](https://github.com/icinga/icinga2/issues/6267) (Documentation, PR): Update docs to reflect required user\* attributes for notification objects
* [#6265](https://github.com/icinga/icinga2/issues/6265) (Documentation): Notifications user/user\_groups required
* [#6264](https://github.com/icinga/icinga2/issues/6264) (Documentation, PR): Enhance "Getting Started" chapter
* [#6262](https://github.com/icinga/icinga2/issues/6262) (Documentation, PR): Enhance the environment variables chapter
* [#6254](https://github.com/icinga/icinga2/issues/6254) (Documentation, PR): Enhance release documentation
* [#6253](https://github.com/icinga/icinga2/issues/6253) (Documentation, PR): Doc: Add note for not fully supported Plugin collections
* [#6243](https://github.com/icinga/icinga2/issues/6243) (Documentation, PR): Update PostgreSQL documentation
* [#6226](https://github.com/icinga/icinga2/issues/6226) (Documentation, PR): Fix broken SELinux anchor in the documentation
* [#6224](https://github.com/icinga/icinga2/issues/6224) (Documentation, PR): Update volatile docs
* [#6216](https://github.com/icinga/icinga2/issues/6216) (Documentation): Volatile service explanation 
* [#6180](https://github.com/icinga/icinga2/issues/6180) (Documentation, PR): Doc: fixed wrong information about defaulting
* [#6128](https://github.com/icinga/icinga2/issues/6128) (Documentation, PR): Adding documentation for configurable global zones during setup
* [#6067](https://github.com/icinga/icinga2/issues/6067) (Documentation, Windows, PR): Improve Windows builds and testing
* [#6022](https://github.com/icinga/icinga2/issues/6022) (Configuration, Documentation, PR): Update default config and documentation for the "library" keyword
* [#6018](https://github.com/icinga/icinga2/issues/6018) (Documentation): Move init configuration from getting-started
* [#6000](https://github.com/icinga/icinga2/issues/6000) (Documentation, PR): Add newline to COPYING to fix Github license detection
* [#5948](https://github.com/icinga/icinga2/issues/5948) (Documentation, PR): doc: Improve INSTALL documentation
* [#4958](https://github.com/icinga/icinga2/issues/4958) (Check Execution, Documentation): How to set the HOME environment variable

### Support

* [#6439](https://github.com/icinga/icinga2/issues/6439) (PR): Revert "Fix obsolete parameter in Systemd script"
* [#6423](https://github.com/icinga/icinga2/issues/6423) (PR): Fix missing next check update causing the scheduler to execute checks too often
* [#6421](https://github.com/icinga/icinga2/issues/6421) (Check Execution): High CPU load due to seemingly ignored check\_interval
* [#6412](https://github.com/icinga/icinga2/issues/6412) (Plugins, Windows, PR): Fix output formatting in windows plugins
* [#6402](https://github.com/icinga/icinga2/issues/6402) (Cluster, code-quality, PR): Use SSL\_pending\(\) for remaining TLS stream data
* [#6384](https://github.com/icinga/icinga2/issues/6384) (PR): Remove leftover for sysconfig file parsing
* [#6381](https://github.com/icinga/icinga2/issues/6381) (Packages, PR): Fix sysconfig not being handled correctly by sysvinit
* [#6377](https://github.com/icinga/icinga2/issues/6377) (code-quality, PR): Fix missing name for workqueue while creating runtime objects via API
* [#6364](https://github.com/icinga/icinga2/issues/6364) (code-quality): lib/base/workqueue.cpp:212: assertion failed: !m\_Name.IsEmpty\(\)
* [#6361](https://github.com/icinga/icinga2/issues/6361) (API, Cluster): Analyse socket IO handling with HTTP/JSON-RPC
* [#6359](https://github.com/icinga/icinga2/issues/6359) (Configuration, PR): Fix ScheduledDowntimes replicating on restart
* [#6357](https://github.com/icinga/icinga2/issues/6357) (API, PR): Increase header size to 8KB for HTTP requests
* [#6347](https://github.com/icinga/icinga2/issues/6347) (Packages, PR): SELinux: Allow notification plugins to read local users 
* [#6343](https://github.com/icinga/icinga2/issues/6343) (Check Execution, Cluster, PR): Fix that checks with command\_endpoint don't return any check results
* [#6337](https://github.com/icinga/icinga2/issues/6337): Checks via command\_endpoint are not executed \(snapshot packages only\)
* [#6328](https://github.com/icinga/icinga2/issues/6328) (Installation, Packages, PR): Rework sysconfig file/startup environment
* [#6320](https://github.com/icinga/icinga2/issues/6320) (PR): Ensure that icinga\_min\_version parameter is optional
* [#6309](https://github.com/icinga/icinga2/issues/6309) (PR): Fix compiler warning in checkercomponent.ti
* [#6306](https://github.com/icinga/icinga2/issues/6306) (code-quality, PR): Adjust message for CheckResultReader deprecation
* [#6301](https://github.com/icinga/icinga2/issues/6301) (Documentation, code-quality, PR): Adjust deprecation removal for compat features
* [#6295](https://github.com/icinga/icinga2/issues/6295) (Compat, PR): Deprecate compatlog feature
* [#6238](https://github.com/icinga/icinga2/issues/6238) (Notifications, PR): Implement better way to check parameters in notification scripts
* [#6233](https://github.com/icinga/icinga2/issues/6233) (Check Execution): Verify next check execution on daemon reload
* [#6229](https://github.com/icinga/icinga2/issues/6229) (Packages, PR): Don't use shell variables in sysconfig
* [#6214](https://github.com/icinga/icinga2/issues/6214) (Packages): Reload-internal with unresolved shell variable
* [#6201](https://github.com/icinga/icinga2/issues/6201) (Windows, PR): Handle exceptions from X509Certificate2
* [#6199](https://github.com/icinga/icinga2/issues/6199) (API, PR): Return 500 when no api action is successful
* [#6198](https://github.com/icinga/icinga2/issues/6198) (Compat, PR): Deprecate Statusdatawriter
* [#6187](https://github.com/icinga/icinga2/issues/6187) (code-quality, PR): Remove Icinga Studio Screenshots
* [#6181](https://github.com/icinga/icinga2/issues/6181) (Tests, PR): tests: Ensure IcingaApplication is initialized before adding config
* [#6174](https://github.com/icinga/icinga2/issues/6174) (API, PR): Fix crash without CORS setting
* [#6173](https://github.com/icinga/icinga2/issues/6173) (API, Crash): Using the API crashes Icinga2 in v2.8.1-537-g064fc80
* [#6171](https://github.com/icinga/icinga2/issues/6171) (code-quality, PR): Update copyright of the Windows Agent to 2018
* [#6163](https://github.com/icinga/icinga2/issues/6163) (PR): Fix reload handling by updating the PID file before process overtake
* [#6160](https://github.com/icinga/icinga2/issues/6160) (code-quality, PR): Replace std::vector:push\_back calls with initializer list
* [#6126](https://github.com/icinga/icinga2/issues/6126) (PR): Require systemd headers
* [#6113](https://github.com/icinga/icinga2/issues/6113) (Tests, PR): appveyor: Disable artifacts until we use them
* [#6107](https://github.com/icinga/icinga2/issues/6107) (code-quality, PR): Allow MYSQL\_LIB to be specified by ENV variable
* [#6105](https://github.com/icinga/icinga2/issues/6105) (Tests): Snapshot builds fail on livestatus tests
* [#6098](https://github.com/icinga/icinga2/issues/6098) (API, code-quality, PR): Clean up CORS implementation
* [#6085](https://github.com/icinga/icinga2/issues/6085) (Cluster, Crash, PR): Fix crash with anonymous clients on certificate signing request and storing sent bytes
* [#6083](https://github.com/icinga/icinga2/issues/6083) (Log, code-quality, PR): Fix wrong type logging in ConfigItem::Commit
* [#6082](https://github.com/icinga/icinga2/issues/6082) (Installation, Packages): PID file removed after reload
* [#6063](https://github.com/icinga/icinga2/issues/6063) (Compat, PR): Deprecate CheckResultReader
* [#6062](https://github.com/icinga/icinga2/issues/6062) (code-quality, PR): Remove the obsolete 'make-agent-config.py' script
* [#6061](https://github.com/icinga/icinga2/issues/6061) (code-quality, PR): Remove jenkins test scripts
* [#6060](https://github.com/icinga/icinga2/issues/6060) (code-quality, PR): Remove Icinga development docker scripts
* [#6059](https://github.com/icinga/icinga2/issues/6059) (code-quality, PR): Remove Icinga Studio
* [#6058](https://github.com/icinga/icinga2/issues/6058) (code-quality, PR): Clean up the Icinga plugins a bit
* [#6055](https://github.com/icinga/icinga2/issues/6055) (Check Execution, Windows, code-quality, PR): methods: Remove unused clrchecktask feature
* [#6054](https://github.com/icinga/icinga2/issues/6054) (Check Execution, Windows, code-quality): Remove unused clrchecktask
* [#6051](https://github.com/icinga/icinga2/issues/6051) (code-quality, PR): Set FOLDER cmake property for the icingaloader target
* [#6050](https://github.com/icinga/icinga2/issues/6050) (code-quality, PR): Replace boost::algorithm::split calls with String::Split
* [#6044](https://github.com/icinga/icinga2/issues/6044) (code-quality, PR): Implement support for frozen arrays and dictionaries
* [#6038](https://github.com/icinga/icinga2/issues/6038) (PR): Fix missing include for boost::split
* [#6037](https://github.com/icinga/icinga2/issues/6037) (PR): Fix build error on Windows
* [#6029](https://github.com/icinga/icinga2/issues/6029) (code-quality, PR): Remove duplicate semicolons
* [#6028](https://github.com/icinga/icinga2/issues/6028) (Packages): python notification not running when icinga ran as a service
* [#6026](https://github.com/icinga/icinga2/issues/6026) (Check Execution, Windows, PR): Fix flapping support for Windows
* [#6025](https://github.com/icinga/icinga2/issues/6025) (Windows): Implement Flapping on Windows
* [#6023](https://github.com/icinga/icinga2/issues/6023): Icinga should check whether the libsystemd library is available
* [#6017](https://github.com/icinga/icinga2/issues/6017) (PR): Remove build breaking include
* [#6015](https://github.com/icinga/icinga2/issues/6015) (code-quality, PR): Fix whitespaces in CMakeLists files
* [#6009](https://github.com/icinga/icinga2/issues/6009) (PR): Build fix for ancient versions of GCC
* [#6008](https://github.com/icinga/icinga2/issues/6008) (PR): Fix compatibility with CMake \< 3.1
* [#6007](https://github.com/icinga/icinga2/issues/6007) (PR): Fix missing include
* [#6005](https://github.com/icinga/icinga2/issues/6005) (PR): Fix incorrect dependencies for mkunity targets
* [#5999](https://github.com/icinga/icinga2/issues/5999) (PR): Build fix
* [#5998](https://github.com/icinga/icinga2/issues/5998) (code-quality, PR): Build all remaining libraries as object libraries
* [#5997](https://github.com/icinga/icinga2/issues/5997) (PR): Use gcc-ar and gcc-ranlib when building with -flto
* [#5994](https://github.com/icinga/icinga2/issues/5994) (InfluxDB, PR): InfluxDBWriter: Fix macro in template
* [#5993](https://github.com/icinga/icinga2/issues/5993) (code-quality, PR): Use CMake object libraries for our libs
* [#5992](https://github.com/icinga/icinga2/issues/5992) (code-quality, PR): Remove unused includes
* [#5984](https://github.com/icinga/icinga2/issues/5984) (DB IDO, PR): Fix missing static libraries for DB IDO
* [#5983](https://github.com/icinga/icinga2/issues/5983) (code-quality, PR): Use initializer lists for arrays and dictionaries
* [#5980](https://github.com/icinga/icinga2/issues/5980) (code-quality, PR): Explicitly pass 1 or 0 for notification filters in DB IDO
* [#5974](https://github.com/icinga/icinga2/issues/5974) (PR): Fix non-unity builds with the icinga check
* [#5971](https://github.com/icinga/icinga2/issues/5971) (code-quality, PR): Remove libdemo and libhello
* [#5970](https://github.com/icinga/icinga2/issues/5970) (code-quality, PR): Allocate ConfigItemBuilder objects on the stack
* [#5969](https://github.com/icinga/icinga2/issues/5969) (code-quality, PR): Remove the WorkQueue::m\_StatsMutex instance variable
* [#5968](https://github.com/icinga/icinga2/issues/5968) (code-quality, PR): Update the RingBuffer class to use a regular mutex instead of ObjectLock
* [#5967](https://github.com/icinga/icinga2/issues/5967) (code-quality, PR): Avoid accessing attributes for validators where not necessary
* [#5965](https://github.com/icinga/icinga2/issues/5965) (code-quality, PR): Avoid unnecessary casts in the JSON encoder
* [#5961](https://github.com/icinga/icinga2/issues/5961) (PR): Fix macro warning from the icinga check
* [#5960](https://github.com/icinga/icinga2/issues/5960): Macro warning from the icinga check
* [#5957](https://github.com/icinga/icinga2/issues/5957) (code-quality, PR): Change a bunch more copyright headers for 2018
* [#5955](https://github.com/icinga/icinga2/issues/5955) (Configuration, code-quality, PR): Avoid mutex contention in the config parser
* [#5946](https://github.com/icinga/icinga2/issues/5946) (code-quality, PR): Use clang-tidy to add some more C++11 features
* [#5945](https://github.com/icinga/icinga2/issues/5945) (code-quality, PR): Fix incorrect indentation for code generated by mkclass
* [#5944](https://github.com/icinga/icinga2/issues/5944) (code-quality, PR): Add the final keyword to classes
* [#5939](https://github.com/icinga/icinga2/issues/5939) (PR): Build fix for Debian wheezy
* [#5937](https://github.com/icinga/icinga2/issues/5937) (code-quality, PR): Remove inline methods and use explicit template instantiation to minimize the number of weak symbols
* [#5936](https://github.com/icinga/icinga2/issues/5936) (code-quality, PR): Clean up source lists in the CMakeLists.txt files
* [#5935](https://github.com/icinga/icinga2/issues/5935) (code-quality, PR): Implement support for precompiled headers
* [#5934](https://github.com/icinga/icinga2/issues/5934) (code-quality, PR): Add more include/library paths for MySQL and PostgreSQL
* [#5933](https://github.com/icinga/icinga2/issues/5933) (code-quality, PR): Change copyright headers for 2018
* [#5932](https://github.com/icinga/icinga2/issues/5932) (code-quality, PR): Fix copyright header in cli/troubleshootcommand.hpp
* [#5931](https://github.com/icinga/icinga2/issues/5931) (code-quality, PR): Improve detection for linker flags
* [#5930](https://github.com/icinga/icinga2/issues/5930) (code-quality, PR): Replace boost::function with std::function
* [#5929](https://github.com/icinga/icinga2/issues/5929) (code-quality, PR): Get rid of boost::assign::list\_of in mkclass
* [#5927](https://github.com/icinga/icinga2/issues/5927) (code-quality, PR): Build libraries as static libraries
* [#5909](https://github.com/icinga/icinga2/issues/5909) (code-quality, PR): WIP: Improve build times
* [#5903](https://github.com/icinga/icinga2/issues/5903) (code-quality, PR): Cleanup CompatUtility class and features
* [#5897](https://github.com/icinga/icinga2/issues/5897) (code-quality, PR): Remove unnecessary inline statements
* [#5894](https://github.com/icinga/icinga2/issues/5894) (code-quality, PR): Remove string\_iless
* [#5891](https://github.com/icinga/icinga2/issues/5891) (code-quality, PR): Update .gitignore
* [#5889](https://github.com/icinga/icinga2/issues/5889) (code-quality, PR): execvpe: Fixup indention for readability
* [#5887](https://github.com/icinga/icinga2/issues/5887) (PR): Windows build fix
* [#5886](https://github.com/icinga/icinga2/issues/5886) (code-quality): Remove unnecessary 'inline' keyword
* [#5882](https://github.com/icinga/icinga2/issues/5882) (code-quality, PR): Avoid unnecessary allocations
* [#5871](https://github.com/icinga/icinga2/issues/5871) (code-quality, PR): Unit tests for the LegacyTimePeriod class
* [#5868](https://github.com/icinga/icinga2/issues/5868) (Configuration, code-quality, PR): Use std::unique\_ptr for Expression objects
* [#5865](https://github.com/icinga/icinga2/issues/5865) (code-quality, PR): Add missing initializer in Utility::NewUniqueID\(\)
* [#5862](https://github.com/icinga/icinga2/issues/5862) (code-quality, PR): Replace a few more NULLs with nullptr
* [#5858](https://github.com/icinga/icinga2/issues/5858) (Tests, code-quality, PR): Travis: Add support for Coverity
* [#5857](https://github.com/icinga/icinga2/issues/5857) (code-quality, PR): Fix compiler warnings
* [#5855](https://github.com/icinga/icinga2/issues/5855) (PR): Fix build problems with Visual Studio 2017
* [#5848](https://github.com/icinga/icinga2/issues/5848) (code-quality, PR): Fix COPYING format
* [#5846](https://github.com/icinga/icinga2/issues/5846) (code-quality, PR): Fix compiler warnings
* [#5831](https://github.com/icinga/icinga2/issues/5831) (Check Execution, Configuration): No checks were launched on snapshot version 2.8.0.71 \(RHEL6\)
* [#5827](https://github.com/icinga/icinga2/issues/5827) (code-quality, PR): Replace StatsFunction with Function
* [#5825](https://github.com/icinga/icinga2/issues/5825) (code-quality, PR): Replace boost::assign::list\_of with initializer lists
* [#5824](https://github.com/icinga/icinga2/issues/5824) (code-quality, PR): Replace a few Boost features with equivalent C++11 features
* [#5821](https://github.com/icinga/icinga2/issues/5821) (Packages, Windows): check\_disk build error
* [#5819](https://github.com/icinga/icinga2/issues/5819) (code-quality, PR): Avoid unnecessary allocations in the FunctionCallExpression class
* [#5816](https://github.com/icinga/icinga2/issues/5816) (code-quality, PR): Re-implement WrapFunction\(\) using C++11 features
* [#5809](https://github.com/icinga/icinga2/issues/5809) (Documentation, Installation, PR): Raise required OpenSSL version to 1.0.1
* [#5758](https://github.com/icinga/icinga2/issues/5758) (Documentation, Packages): Completely remove the spec file from the icinga2 repository
* [#5743](https://github.com/icinga/icinga2/issues/5743) (CLI, Configuration, Installation): node setup: Deprecate --master\_host and use --parent\_host instead
* [#5725](https://github.com/icinga/icinga2/issues/5725) (code-quality, PR): Use real UUIDs for Utility::NewUniqueID
* [#5388](https://github.com/icinga/icinga2/issues/5388) (Packages, PR): Handle mis-detection with clang on RHEL/CentOS 7
* [#3246](https://github.com/icinga/icinga2/issues/3246) (Installation): Add option to windows installer to disable inclusion of conf.d directory

## 2.8.4 (2018-04-25)

### Bug

* [#6257](https://github.com/icinga/icinga2/issues/6257) (Check Execution): Plugins crash when run from icinga2-2.8.3 

### Support

* [#6260](https://github.com/icinga/icinga2/issues/6260) (Check Execution, PR): Revert "fixup set rlimit stack failed condition"

## 2.8.3 (2018-04-24)

### Notes

- Fix InfluxDB backslash escaping
- Fix Elasticsearch crash on invalid performance data
- Sysconfig file settings are taken into account
- Support multiple parameters for check_nscp_api
- Documentation enhancements and fixes

### Bug

* [#6207](https://github.com/icinga/icinga2/issues/6207) (Plugins, Windows, PR): Fix multiple parameter problems for check\_nscp\_api
* [#6196](https://github.com/icinga/icinga2/issues/6196) (InfluxDB, Metrics, PR): Fix InfluxDB backslash escaping
* [#6192](https://github.com/icinga/icinga2/issues/6192) (Crash, Elasticsearch, PR): Elasticsearch: Fix crash with invalid performance data metrics
* [#6191](https://github.com/icinga/icinga2/issues/6191) (Crash, Elasticsearch): Invalid Perfdata causing Segmentation fault with ElasticsearchWriter
* [#6182](https://github.com/icinga/icinga2/issues/6182) (InfluxDB): Windows Disk performance data broken in InfluxDB
* [#6179](https://github.com/icinga/icinga2/issues/6179) (CLI, Crash, PR): Fix crash in api user command
* [#6178](https://github.com/icinga/icinga2/issues/6178) (API, Crash): Error: boost::bad\_any\_cast: failed conversion using boost::any\_cast
* [#6140](https://github.com/icinga/icinga2/issues/6140): Force check has no effect
* [#6119](https://github.com/icinga/icinga2/issues/6119) (PR): fixup set rlimit stack failed condition
* [#5925](https://github.com/icinga/icinga2/issues/5925) (Crash, PR): Fix missing variable name in ApiListener::Start
* [#5924](https://github.com/icinga/icinga2/issues/5924) (Crash): The lock variable in ApiListener::Start is missing its name
* [#5881](https://github.com/icinga/icinga2/issues/5881) (API, PR): Fix package error message
* [#5706](https://github.com/icinga/icinga2/issues/5706) (Plugins, Windows): nscp\_api - cannot use check\_cpu with "time" argument used multiple times

### Documentation

* [#6227](https://github.com/icinga/icinga2/issues/6227) (Documentation, PR): Fix missing anchors in CLI commands chapter
* [#6203](https://github.com/icinga/icinga2/issues/6203) (Documentation, PR): Add docs for script debugger and API filters
* [#6177](https://github.com/icinga/icinga2/issues/6177) (Documentation, PR): Doc: Fix typo in API user creation example
* [#6176](https://github.com/icinga/icinga2/issues/6176) (Documentation, PR): hashed\_password -\> password\_hash. Fixes \#6175
* [#6175](https://github.com/icinga/icinga2/issues/6175) (Documentation): ApiUser does not know hashed\_password Attribute
* [#6166](https://github.com/icinga/icinga2/issues/6166) (Documentation, PR): Fix broken link in README
* [#6145](https://github.com/icinga/icinga2/issues/6145) (Documentation, PR): Fix incorrect parameter name in the API documentation
* [#6102](https://github.com/icinga/icinga2/issues/6102) (Documentation, PR): Fix typo in Apply for Rules documentation
* [#6080](https://github.com/icinga/icinga2/issues/6080) (Documentation, PR): Document the 'ignore\_on\_error' attribute for object creation
* [#6068](https://github.com/icinga/icinga2/issues/6068) (Documentation, PR): Fix the explanation of `types` and `states` for user objects
* [#5913](https://github.com/icinga/icinga2/issues/5913) (Documentation, ITL, PR): Enhance http\_certificate parameter documentation
* [#5838](https://github.com/icinga/icinga2/issues/5838) (Documentation, PR): services.conf has also be moved to zones.d/global-templates/
* [#5797](https://github.com/icinga/icinga2/issues/5797) (Documentation): Document the ignore\_on\_error parameter for CreateObjectHandler::HandleRequest
* [#5610](https://github.com/icinga/icinga2/issues/5610) (Documentation, ITL): http check doesn't map the critical ssl certificate age option

### Support

* [#6250](https://github.com/icinga/icinga2/issues/6250) (PR): Fix typo
* [#6241](https://github.com/icinga/icinga2/issues/6241) (Packages, PR): Fix Sysconfig file detection for Icinga 2 settings
* [#6230](https://github.com/icinga/icinga2/issues/6230) (PR): Unbreak build against Boost 1.67
* [#6215](https://github.com/icinga/icinga2/issues/6215) (Configuration, Packages): Sysconfig limits and settings are not respected
* [#6202](https://github.com/icinga/icinga2/issues/6202) (Packages, code-quality, PR): Use VERSION instead of icinga2.spec

## 2.8.2 (2018-03-22)

### Notes

A bugfix release with a focus on security.

Most of these have been brought to our attention by the community and we are very thankful for that. Special thanks to Michael H., Julian and Michael O., who helped by reporting and assisting us in fixing security bugs. CVEs have also been requested for these issues, they are as follows: CVE-2017-16933, CVE-2018-6532, CVE-2018-6533, CVE-2018-6534, CVE-2018-6535, CVE-2018-6536.

### Enhancement

* [#5715](https://github.com/icinga/icinga2/issues/5715) (API, PR): Hash API password and comparison

### Bug

* [#6153](https://github.com/icinga/icinga2/issues/6153) (API, PR): Improve error handling for empty packages in /v1/config/packages
* [#6147](https://github.com/icinga/icinga2/issues/6147) (PR): Fix incorrect argument type for JsonRpc::SendMessage
* [#6146](https://github.com/icinga/icinga2/issues/6146) (PR): Ensure that SetCorked\(\) works properly
* [#6134](https://github.com/icinga/icinga2/issues/6134) (PR): Fix incorrect HTTP content length limits
* [#6133](https://github.com/icinga/icinga2/issues/6133) (PR): Limit the number of HTTP/JSON-RPC requests we read in parallel
* [#6132](https://github.com/icinga/icinga2/issues/6132) (PR): Fix HTTP parser crash/hang
* [#6129](https://github.com/icinga/icinga2/issues/6129): api/packages not created by prepare-dir/daemon
* [#5995](https://github.com/icinga/icinga2/issues/5995) (InfluxDB, PR): Fix InfluxDB requests
* [#5991](https://github.com/icinga/icinga2/issues/5991): Partial privilege escalation via PID file manipulation
* [#5987](https://github.com/icinga/icinga2/issues/5987) (Elasticsearch, InfluxDB, Metrics): InfluxDBWriter and ElasticsearchWriter stop writing to HTTP API
* [#5943](https://github.com/icinga/icinga2/issues/5943) (PR): Fix incorrect ::Start call
* [#5793](https://github.com/icinga/icinga2/issues/5793): CVE-2017-16933: root privilege escalation via prepare-dirs \(init script and systemd service file\)
* [#5760](https://github.com/icinga/icinga2/issues/5760) (Crash, PR): Fix incorrect socket handling for the HTTP client

### Documentation

* [#6172](https://github.com/icinga/icinga2/issues/6172) (Documentation, PR): Docs: Add a note to only query the NSClient++ API from the local Icinga 2 client
* [#6111](https://github.com/icinga/icinga2/issues/6111) (Documentation, PR): Add Upgrading to Icinga 2.8.2 chapter
* [#6089](https://github.com/icinga/icinga2/issues/6089) (Documentation, PR): Docs: Fix bracket in notification example
* [#6086](https://github.com/icinga/icinga2/issues/6086) (Documentation, PR): Upgrading: Make it more clear that the Director script is just an example
* [#6075](https://github.com/icinga/icinga2/issues/6075) (Documentation, PR): Explain how to register functions in the global scope
* [#6014](https://github.com/icinga/icinga2/issues/6014) (Documentation, PR): Docs: Add IDO DB tuning tips
* [#6006](https://github.com/icinga/icinga2/issues/6006) (Documentation, PR): Fix wrong nscp-local include in the docs

### Support

* [#6148](https://github.com/icinga/icinga2/issues/6148) (PR): Fix ApiUser unit test
* [#6135](https://github.com/icinga/icinga2/issues/6135) (API, Cluster, PR): Limit JSON RPC message size
* [#6115](https://github.com/icinga/icinga2/issues/6115) (PR): Fix incorrect size of request limits
* [#6114](https://github.com/icinga/icinga2/issues/6114) (PR): Fix typo in prepare-dirs
* [#6104](https://github.com/icinga/icinga2/issues/6104) (PR): Fix nullptr dereferences
* [#6103](https://github.com/icinga/icinga2/issues/6103) (PR): HTTP Security fixes
* [#5982](https://github.com/icinga/icinga2/issues/5982) (Packages, PR): SELinux: Allows icinga2\_t to send sigkill to all domains it transitions to
* [#5916](https://github.com/icinga/icinga2/issues/5916) (Packages): Unable to kill process group after check timeout if SElinux is enabled
* [#5850](https://github.com/icinga/icinga2/issues/5850) (Installation, PR): init script security fixes
* [#5764](https://github.com/icinga/icinga2/issues/5764) (InfluxDB, code-quality, PR): Improve InfluxdbWriter performance
* [#5759](https://github.com/icinga/icinga2/issues/5759) (code-quality, PR): Make default getters and setters non-virtual

## 2.8.1 (2018-01-17)

### Enhancement

* [#5856](https://github.com/icinga/icinga2/issues/5856) (PR): Implement AppLocal deployment support for UCRT

### Bug

* [#5986](https://github.com/icinga/icinga2/issues/5986) (DB IDO, PR): Fix wrong schema constraint for fresh 2.8.0 installations
* [#5947](https://github.com/icinga/icinga2/issues/5947) (DB IDO): Duplicate entry constraint violations in 2.8
* [#5907](https://github.com/icinga/icinga2/issues/5907) (PR): Windows plugin check\_swap build fix
* [#5808](https://github.com/icinga/icinga2/issues/5808) (Crash, PR): Fix missing variable name which can lead to segfaults
* [#5807](https://github.com/icinga/icinga2/issues/5807) (Crash): icinga v2.8.0 crashes frequently with "segmentation fault" on Debian 8.9
* [#5804](https://github.com/icinga/icinga2/issues/5804) (Log, PR): Silence UpdateRepository message errors
* [#5776](https://github.com/icinga/icinga2/issues/5776) (Cluster, Log): 2.8.0: warning/JsonRpcConnection: Call to non-existent function 'event::UpdateRepository' 
* [#5746](https://github.com/icinga/icinga2/issues/5746) (Livestatus, PR): livestatus: custom variables return empty arrays instead of strings
* [#5716](https://github.com/icinga/icinga2/issues/5716) (Livestatus, PR): add bogus zero reply in livestatus when aggregate and non matching filter
* [#5626](https://github.com/icinga/icinga2/issues/5626) (Livestatus, help wanted): Empty result set with non-matching filters in Livestatus stats query

### ITL

* [#5785](https://github.com/icinga/icinga2/issues/5785) (ITL, PR): ITL: Drop ssl\_sni default setting
* [#5775](https://github.com/icinga/icinga2/issues/5775) (ITL): Default usage of ssl\_sni in check\_tcp

### Documentation

* [#5972](https://github.com/icinga/icinga2/issues/5972) (Documentation, PR): Update 08-advanced-topics.md
* [#5942](https://github.com/icinga/icinga2/issues/5942) (Documentation, PR): Add some technical insights into the cluster-zone health check and log lag
* [#5922](https://github.com/icinga/icinga2/issues/5922) (Documentation, PR): Fix link format in documentation
* [#5918](https://github.com/icinga/icinga2/issues/5918) (Documentation, PR): Fix typo in SELinux documentation
* [#5911](https://github.com/icinga/icinga2/issues/5911) (Documentation, PR): Update ElasticsearchWriter docs for 5.x support only
* [#5866](https://github.com/icinga/icinga2/issues/5866) (Documentation, PR): Remove redundant FreeBSD from restart instructions and add openSUSE
* [#5864](https://github.com/icinga/icinga2/issues/5864) (Documentation, PR): Add missing initdb to PostgreSQL documentation
* [#5835](https://github.com/icinga/icinga2/issues/5835) (Documentation, PR): Fixes postgres schema upgrade path
* [#5833](https://github.com/icinga/icinga2/issues/5833) (Documentation, PR): fix formatting error
* [#5790](https://github.com/icinga/icinga2/issues/5790) (Documentation, PR): Documentation fixes
* [#5783](https://github.com/icinga/icinga2/issues/5783) (Documentation, PR): Fix formatting in value types docs
* [#5773](https://github.com/icinga/icinga2/issues/5773) (Documentation, Windows, PR): Update Windows Client requirements for 2.8
* [#5757](https://github.com/icinga/icinga2/issues/5757) (Documentation, PR): Add documentation about automatic service restarts with systemd

### Support

* [#5989](https://github.com/icinga/icinga2/issues/5989) (PR): changelog.py: Adjust categories and labels: Enhancement, Bug, ITL, Documentation, Support
* [#5938](https://github.com/icinga/icinga2/issues/5938) (Packages, Windows): chocolatey outdated version
* [#5893](https://github.com/icinga/icinga2/issues/5893) (code-quality, PR): Whitespace fix
* [#5892](https://github.com/icinga/icinga2/issues/5892) (Installation, PR): Enable installing the init scripts on Solaris
* [#5851](https://github.com/icinga/icinga2/issues/5851) (Plugins, Windows, PR): Fix check\_service returning Warning instead of Critical
* [#5780](https://github.com/icinga/icinga2/issues/5780) (Packages, Windows): Icinga Agent Windows 2.8.0 msvcr120.dll is missing

## 2.8.0 (2017-11-16)

### Notes

* Certificate path changed to /var/lib/icinga2/certs - check the upgrading docs!
* DB IDO 2.8.0 schema upgrade
* Cluster/Clients: Forward certificate signing requests over multiple levels
* Cluster/Clients: Support on-demand signing next to ticket based certificate request signing
* New flapping detection algorithm
* Add ElasticsearchWriter feature with HTTP proxy support
* Add CORS support for the REST API
* Deprecate `flapping_threshold` config option
* Remove client configuration mode "bottom up"
* Remove classicui meta configuration package
* Remove deprecated `enable_legacy_mode` in Graphite feature
* Spec file was moved to https://github.com/icinga/icinga-packaging
* ITL CheckCommand definition updates
* Documentation updates

### Enhancement

* [#5682](https://github.com/icinga/icinga2/issues/5682) (Cluster, Configuration, PR): Implement support for migrating certificates to /var/lib/icinga2/certs
* [#5681](https://github.com/icinga/icinga2/issues/5681) (CLI, Cluster, Windows): Update Windows wizard from enhanced CSR signing \(optional ticket\)
* [#5679](https://github.com/icinga/icinga2/issues/5679) (CLI, Cluster): Migration path for improved certificate signing in the cluster
* [#5606](https://github.com/icinga/icinga2/issues/5606) (Cluster, PR): Remove bottom-up client mode
* [#5602](https://github.com/icinga/icinga2/issues/5602) (Windows, PR): Add windows process elevation and log message if user does not have privileges to read/write files
* [#5587](https://github.com/icinga/icinga2/issues/5587) (Log, PR): SyslogLogger: Implement option to set syslog facility
* [#5580](https://github.com/icinga/icinga2/issues/5580) (Configuration, PR): Implement new script functions: path\_exists, glob and glob\_recursive
* [#5571](https://github.com/icinga/icinga2/issues/5571) (CLI, Cluster, PR): Implement support for forwarding certificate signing requests in the cluster
* [#5569](https://github.com/icinga/icinga2/issues/5569) (Metrics, PR): ElasticWriter: Add basic auth and TLS support for Elasticsearch behind an HTTP proxy
* [#5554](https://github.com/icinga/icinga2/issues/5554) (API, Cluster, PR): Add subjectAltName extension for all non-CA certificates
* [#5547](https://github.com/icinga/icinga2/issues/5547) (API, PR): Add optional reload parameter to config stage upload
* [#5538](https://github.com/icinga/icinga2/issues/5538) (Metrics): Add ElasticsearchWriter feature
* [#5534](https://github.com/icinga/icinga2/issues/5534) (Configuration, PR): Implement get\_services\(host {name,object}\) and add host object support for get\_service\(\)
* [#5527](https://github.com/icinga/icinga2/issues/5527) (API, PR): API: Add execution\_{start,end} attribute to 'process-check-result' action
* [#5450](https://github.com/icinga/icinga2/issues/5450) (CLI, Cluster): Enhance CSR Autosigning \(CA proxy, etc.\)
* [#5443](https://github.com/icinga/icinga2/issues/5443) (API, PR): Add CORS support and set response header 'Access-Control-Allow-Origin'
* [#5435](https://github.com/icinga/icinga2/issues/5435) (Plugins, Windows, PR): Add -d option to check\_service
* [#5002](https://github.com/icinga/icinga2/issues/5002) (API, wishlist): API process-check-result allow setting timestamp
* [#4912](https://github.com/icinga/icinga2/issues/4912) (Configuration): new function get\_services\(host\_name\)
* [#4799](https://github.com/icinga/icinga2/issues/4799) (Cluster): Remove cluster/client mode "bottom up" w/ repository.d and node update-config
* [#4769](https://github.com/icinga/icinga2/issues/4769) (API): Validate and activate config package stages without triggering a reload
* [#4326](https://github.com/icinga/icinga2/issues/4326) (API): API should provide CORS Header
* [#3891](https://github.com/icinga/icinga2/issues/3891) (Plugins): Add option to specify ServiceDescription instead of ServiceName with check\_service.exe

### Bug

* [#5728](https://github.com/icinga/icinga2/issues/5728) (Plugins, Windows, PR): Fix check\_service not working with names
* [#5720](https://github.com/icinga/icinga2/issues/5720) (Check Execution): Flapping tests and bugs
* [#5710](https://github.com/icinga/icinga2/issues/5710) (CLI, Configuration, PR): Include default global zones during node wizard/setup
* [#5707](https://github.com/icinga/icinga2/issues/5707) (CLI): node wizard/setup override zones.conf but do not include default global zones \(director-global, global-templates\)
* [#5696](https://github.com/icinga/icinga2/issues/5696) (PR): Fix fork error handling
* [#5641](https://github.com/icinga/icinga2/issues/5641) (PR): Fix compiler warnings on macOS 10.13
* [#5635](https://github.com/icinga/icinga2/issues/5635) (Configuration, PR): Fix match\(\), regex\(\), cidr\_match\(\) behaviour with MatchAll and empty arrays
* [#5634](https://github.com/icinga/icinga2/issues/5634) (Configuration): match\(\) for arrays returns boolean true if array is empty
* [#5620](https://github.com/icinga/icinga2/issues/5620) (API, PR): Ensure that the REST API config package/stage creation is atomic
* [#5617](https://github.com/icinga/icinga2/issues/5617): Crash with premature EOF on resource limited OS
* [#5614](https://github.com/icinga/icinga2/issues/5614) (PR): Fixed missing include statement in unit tests
* [#5584](https://github.com/icinga/icinga2/issues/5584) (Windows): Build error on Windows
* [#5581](https://github.com/icinga/icinga2/issues/5581) (API, Cluster, Crash, PR): Fix possible race condition in ApiListener locking
* [#5558](https://github.com/icinga/icinga2/issues/5558) (API, PR): Don't sent scheme and hostname in request
* [#5515](https://github.com/icinga/icinga2/issues/5515) (Windows): Config validation fails on Windows with unprivileged account
* [#5500](https://github.com/icinga/icinga2/issues/5500) (Crash, PR): Process: Fix JSON parsing error on process helper crash
* [#5497](https://github.com/icinga/icinga2/issues/5497) (API, PR): API: Fix requested attrs/joins/meta type errors in object query response
* [#5485](https://github.com/icinga/icinga2/issues/5485) (DB IDO, PR): Ensure that expired/removed downtimes/comments are correctly updated in DB IDO
* [#5377](https://github.com/icinga/icinga2/issues/5377) (API, Log): Sending wrong value for key causes ugly stacktrace
* [#5231](https://github.com/icinga/icinga2/issues/5231) (Check Execution, PR): Report failure to kill check command after exceeding timeout
* [#4981](https://github.com/icinga/icinga2/issues/4981) (Check Execution): Failure to kill check command after exceeding timeout is not reported

### ITL

* [#5678](https://github.com/icinga/icinga2/issues/5678) (ITL, PR): Added missing "-q" parameter to check\_ntp\_peer
* [#5672](https://github.com/icinga/icinga2/issues/5672) (ITL, PR): add itl snmp-service for manubulon plugin check\_snmp\_win.pl
* [#5647](https://github.com/icinga/icinga2/issues/5647) (ITL, PR): Allow to disable thresholds for ipmi CheckCommand
* [#5640](https://github.com/icinga/icinga2/issues/5640) (ITL, PR): ITL: Support weathermap data in snmp\_interface CheckCommand
* [#5638](https://github.com/icinga/icinga2/issues/5638) (ITL, PR): Add support for check\_address as default in database CheckCommand objects
* [#5578](https://github.com/icinga/icinga2/issues/5578) (ITL, PR): ITL: Re-Add ssl\_sni attribute for check\_tcp
* [#5577](https://github.com/icinga/icinga2/issues/5577) (ITL): ssl CheckCommand does not support SNI
* [#5570](https://github.com/icinga/icinga2/issues/5570) (ITL, PR): check\_esxi\_hardware.py with new --no-lcd parameter
* [#5559](https://github.com/icinga/icinga2/issues/5559) (ITL, PR): Exclude configfs from disk checks
* [#5427](https://github.com/icinga/icinga2/issues/5427) (ITL): Update negate CheckCommand definition
* [#5401](https://github.com/icinga/icinga2/issues/5401) (ITL, PR): itl: Add manubulon/check\_snmp\_env.pl as CheckCommand snmp-env
* [#5394](https://github.com/icinga/icinga2/issues/5394) (ITL, PR): itl: add additional mssql\_health arguments
* [#5387](https://github.com/icinga/icinga2/issues/5387) (ITL, PR): Add missing options to snmp CheckCommand definition

### Documentation

* [#5768](https://github.com/icinga/icinga2/issues/5768) (Documentation, PR): Update .mailmap and AUTHORS
* [#5761](https://github.com/icinga/icinga2/issues/5761) (Documentation, PR): Fix wrong anchors in the documentation
* [#5755](https://github.com/icinga/icinga2/issues/5755) (Documentation, PR): Fix missing Accept header in troubleshooting docs
* [#5754](https://github.com/icinga/icinga2/issues/5754) (Documentation, PR): Improve documentation of cipher\_list
* [#5752](https://github.com/icinga/icinga2/issues/5752) (Documentation, PR): Add Noah Hilverling to .mailmap
* [#5748](https://github.com/icinga/icinga2/issues/5748) (Documentation, PR): Fix missing word in pin checks in a zone doc chapter
* [#5741](https://github.com/icinga/icinga2/issues/5741) (Documentation, PR): Fix manual certificate creation chapter in the docs
* [#5738](https://github.com/icinga/icinga2/issues/5738) (Documentation, PR): Update release docs
* [#5734](https://github.com/icinga/icinga2/issues/5734) (Documentation, PR): Fix broken links inside the documentation
* [#5727](https://github.com/icinga/icinga2/issues/5727) (Documentation, PR): Update upgrading documentation for 2.8
* [#5708](https://github.com/icinga/icinga2/issues/5708) (Documentation, PR): Fixed grammar and spelling mistakes
* [#5703](https://github.com/icinga/icinga2/issues/5703) (Documentation): Minor documentation typos in flapping detection description
* [#5695](https://github.com/icinga/icinga2/issues/5695) (Documentation, PR): Enhance Security chapter for Distributed Monitoring documentation
* [#5691](https://github.com/icinga/icinga2/issues/5691) (Documentation, PR): Fixed doc formatting
* [#5690](https://github.com/icinga/icinga2/issues/5690) (Documentation): Improve documentation of cipher\_list
* [#5688](https://github.com/icinga/icinga2/issues/5688) (Documentation, PR): Fixed typos and punctuation
* [#5680](https://github.com/icinga/icinga2/issues/5680) (Documentation): Review documentation for enhanced CSR signing and update migration chapter for 2.8
* [#5677](https://github.com/icinga/icinga2/issues/5677) (Documentation, PR): Fix typo in threshold syntax documentation
* [#5668](https://github.com/icinga/icinga2/issues/5668) (Documentation, PR): Enhance Monitoring Basics in the documentation
* [#5667](https://github.com/icinga/icinga2/issues/5667) (Documentation): Explain which values can be used for set\_if in command arguments
* [#5666](https://github.com/icinga/icinga2/issues/5666) (Documentation): Explain the notification with users defined on host/service in a dedicated docs chapter
* [#5665](https://github.com/icinga/icinga2/issues/5665) (Documentation): Better explanations and iteration details for "apply for" documentation
* [#5664](https://github.com/icinga/icinga2/issues/5664) (Documentation): Add usage examples to the "apply" chapter based on custom attribute values
* [#5663](https://github.com/icinga/icinga2/issues/5663) (Documentation): Explain custom attribute value types and nested dictionaries
* [#5662](https://github.com/icinga/icinga2/issues/5662) (Documentation): Explain how to use a different host check command
* [#5655](https://github.com/icinga/icinga2/issues/5655) (Documentation, PR): Enhance documentation with more details on value types for object attributes
* [#5576](https://github.com/icinga/icinga2/issues/5576) (Documentation, PR): Fixed downtime example in documentation
* [#5568](https://github.com/icinga/icinga2/issues/5568) (Documentation, PR): Add documentation for multi-line plugin output for API actions
* [#5511](https://github.com/icinga/icinga2/issues/5511) (Cluster, Documentation, Windows): SSL errors with leading zeros in certificate serials \(created \< v2.4\) with OpenSSL 1.1.0
* [#5379](https://github.com/icinga/icinga2/issues/5379) (Documentation, PR): Set shell prompt for commands to be \#
* [#5186](https://github.com/icinga/icinga2/issues/5186) (Documentation): Document boolean values understood by set\_if
* [#5060](https://github.com/icinga/icinga2/issues/5060) (Documentation): Missing documentation for macro\(\)
* [#4015](https://github.com/icinga/icinga2/issues/4015) (Documentation): Add documentation for host state calculation from plugin exit codes

### Support

* [#5765](https://github.com/icinga/icinga2/issues/5765) (Configuration, PR): Fix default configuration example for ElasticsearchWriter
* [#5739](https://github.com/icinga/icinga2/issues/5739) (Metrics, PR): Rename ElasticWriter to ElasticsearchWriter
* [#5732](https://github.com/icinga/icinga2/issues/5732) (Check Execution, DB IDO, PR): Fix flapping calculation and events
* [#5730](https://github.com/icinga/icinga2/issues/5730) (PR): Add missing trims to GetMasterHostPort and remove Convert.ToString from variables that are strings already
* [#5719](https://github.com/icinga/icinga2/issues/5719) (Cluster, Installation, Windows, PR): Update Windows Wizard for 2.8 and new signing methods
* [#5687](https://github.com/icinga/icinga2/issues/5687) (Cluster, Log, PR): Improve error message for unknown cluster message functions
* [#5686](https://github.com/icinga/icinga2/issues/5686) (Log): Ugly stacktrace with mismatching versions in cluster
* [#5643](https://github.com/icinga/icinga2/issues/5643) (PR): Fix debug builds on Apple Clang 9.0.0 \(macOS High Sierra\)
* [#5637](https://github.com/icinga/icinga2/issues/5637) (InfluxDB, PR): Fix unnecessary String\(\) casts in InfluxdbWriter
* [#5629](https://github.com/icinga/icinga2/issues/5629) (InfluxDB, Metrics, code-quality): Remove the unnecessary String\(\) casts in influxdbwriter.cpp
* [#5624](https://github.com/icinga/icinga2/issues/5624) (PR): Fixed missing include statement in unit test
* [#5619](https://github.com/icinga/icinga2/issues/5619) (Packages, PR): Exit early in changelog.py if GitHub API fetch fails
* [#5616](https://github.com/icinga/icinga2/issues/5616) (PR): Fix a build warning
* [#5608](https://github.com/icinga/icinga2/issues/5608) (CLI, Cluster, PR): Fix certificate paths for installers
* [#5604](https://github.com/icinga/icinga2/issues/5604) (Packages, PR): Remove the icinga2-classicui-package and update documentation
* [#5601](https://github.com/icinga/icinga2/issues/5601) (Installation, Packages, PR): Ensure that the cache directory always is set and add a note to upgrading docs
* [#5563](https://github.com/icinga/icinga2/issues/5563) (Cluster, PR): Implement additional logging for the JsonRpc class
* [#5545](https://github.com/icinga/icinga2/issues/5545) (Installation, Windows, PR): Add Edit button to Windows Setup Wizard
* [#5488](https://github.com/icinga/icinga2/issues/5488) (code-quality, PR): Implement additional functions for printing values with LLDB/GDB
* [#5486](https://github.com/icinga/icinga2/issues/5486) (Graphite, PR): Graphite: Remove deprecated legacy schema mode
* [#5301](https://github.com/icinga/icinga2/issues/5301) (Installation, Packages): Remove the icinga2-classicui-config package
* [#5258](https://github.com/icinga/icinga2/issues/5258) (Installation, PR): Fix clang compiler detection on Fedora and macOS
* [#4992](https://github.com/icinga/icinga2/issues/4992) (Graphite): Remove deprecated GraphiteWriter feature enable\_legacy\_mode
* [#4982](https://github.com/icinga/icinga2/issues/4982) (Notifications, Tests): Verify and fix flapping detection

## 2.7.2 (2017-11-09)

### Notes

* Fixed invalid attribute names in the systemd unit file
* Fixed incorrect unique constraint for IDO DB
* Moved spec file to the icinga-packaging Git repository
* Documentation updates

### Bug

* [#5636](https://github.com/icinga/icinga2/issues/5636) (DB IDO, PR): Fix unique constraint matching for UPDATE downtime/comment runtime tables in DB IDO
* [#5623](https://github.com/icinga/icinga2/issues/5623) (DB IDO): Duplicate Key on MySQL after upgrading to v2.7.1
* [#5603](https://github.com/icinga/icinga2/issues/5603) (DB IDO): Icinga 2.7.1 IDO Unique Key Constraint Violation with PostgreSQL

### Documentation

* [#5653](https://github.com/icinga/icinga2/issues/5653) (Documentation, PR): Docs: Fix default value for `snmp\_nocrypt` for Manubulon CheckCommand definitions
* [#5652](https://github.com/icinga/icinga2/issues/5652) (Documentation, PR): Docs: Fix missing default value for cluster-zone checks
* [#5632](https://github.com/icinga/icinga2/issues/5632) (Documentation, PR): Docs: Mention SELinux in Getting Started chapter

### Support

* [#5736](https://github.com/icinga/icinga2/issues/5736) (Packages, PR): Remove spec file
* [#5612](https://github.com/icinga/icinga2/issues/5612) (Documentation, Packages, PR): Improve documentation and systemd config on TaskMax

## 2.7.1 (2017-09-21)

### Notes

* Fixes and upgrade documentation for notificatication scripts introduced in 2.7.0
* InfluxdbWriter attribute `socket_timeout` introduced in 2.7.0 was deprecated (will be removed in 2.8.0). Details in #5469 and #5460
* Livestatus bygroup table stats fixes for NagVis
* DB IDO: Fixes for downtime/comment history queries not correctly updating the end time
* check_nscp_api allows white spaces in arguments
* Bugfixes
* Documentation updates

### Enhancement

* [#5523](https://github.com/icinga/icinga2/issues/5523) (Cluster, Log, PR): Enhance client connect/sync logging and include bytes/zone in logs
* [#5474](https://github.com/icinga/icinga2/issues/5474) (Notifications, PR): Notification scripts - make HOSTADDRESS optional
* [#5468](https://github.com/icinga/icinga2/issues/5468) (Notifications, PR): Make notification mails more readable. Remove redundancy and cruft.

### Bug

* [#5585](https://github.com/icinga/icinga2/issues/5585) (DB IDO, PR): Fix where clause for non-matching {downtime,comment}history IDO database updates
* [#5566](https://github.com/icinga/icinga2/issues/5566) (Cluster, Log, PR): Logs: Change config sync update to highlight an information, not an error
* [#5539](https://github.com/icinga/icinga2/issues/5539) (Plugins, Windows, PR): check\_nscp\_api: Allow arguments containing spaces
* [#5537](https://github.com/icinga/icinga2/issues/5537) (Plugins): check\_nscp\_api: support spaces in query arguments
* [#5524](https://github.com/icinga/icinga2/issues/5524) (Cluster, PR): Change FIFO::Optimize\(\) frequency for large messages
* [#5513](https://github.com/icinga/icinga2/issues/5513) (Cluster): Node in Cluster loses connection
* [#5504](https://github.com/icinga/icinga2/issues/5504) (InfluxDB, PR): Fix TLS Race Connecting to InfluxDB
* [#5503](https://github.com/icinga/icinga2/issues/5503) (Livestatus, PR): Fix grouping for Livestatus queries with 'Stats'
* [#5502](https://github.com/icinga/icinga2/issues/5502) (Notifications, PR): Fix duplicate variable in notification scripts
* [#5495](https://github.com/icinga/icinga2/issues/5495) (Notifications, PR): Fix parameter order for AcknowledgeSvcProblem / AcknowledgeHostProblem / apiactions:AcknowledgeProblem
* [#5492](https://github.com/icinga/icinga2/issues/5492) (DB IDO): Comments may not be removed correctly
* [#5484](https://github.com/icinga/icinga2/issues/5484) (Log): Timestamp comparison of config files logs a wrong message
* [#5483](https://github.com/icinga/icinga2/issues/5483) (DB IDO, PR): Fix config validation for DB IDO categories 'DbCatEverything'
* [#5469](https://github.com/icinga/icinga2/issues/5469) (InfluxDB): Failure to connect to InfluxDB increases CPU utilisation by 100%  for every failure
* [#5466](https://github.com/icinga/icinga2/issues/5466) (DB IDO, PR): DB IDO: Fix host's unreachable state in history tables
* [#5460](https://github.com/icinga/icinga2/issues/5460) (InfluxDB): Icinga 2.7 InfluxdbWriter fails to write metrics to InfluxDB over HTTPS
* [#5458](https://github.com/icinga/icinga2/issues/5458) (DB IDO): IDO donwtimehistory records orphaned from scheduleddowntime records following restart
* [#5405](https://github.com/icinga/icinga2/issues/5405) (DB IDO): IDO statehistory table does not show hosts going to "UNREACHABLE" state.
* [#5078](https://github.com/icinga/icinga2/issues/5078) (Compat, Livestatus): Livestatus hostsbygroup and servicesbyhostgroup do not work

### ITL

* [#5543](https://github.com/icinga/icinga2/issues/5543) (ITL, PR): ITL: Correct arguments for ipmi-sensor CheckCommand

### Documentation

* [#5594](https://github.com/icinga/icinga2/issues/5594) (Documentation, PR): Docs: Enhance certificate and configuration troubleshooting chapter
* [#5593](https://github.com/icinga/icinga2/issues/5593) (Documentation, PR): Docs: Add a note for upgrading to 2.7
* [#5583](https://github.com/icinga/icinga2/issues/5583) (Documentation, PR): Docs: Add example for Windows service monitoring with check\_nscp\_api
* [#5582](https://github.com/icinga/icinga2/issues/5582) (Documentation, PR): Docs: Add firewall details for check\_nscp\_api
* [#5549](https://github.com/icinga/icinga2/issues/5549) (Documentation, PR): Fix cli command used to enable debuglog feature on windows
* [#5536](https://github.com/icinga/icinga2/issues/5536) (Documentation, PR): Fixed nscp-disk service example
* [#5522](https://github.com/icinga/icinga2/issues/5522) (Documentation, PR): Docs: Update freshness checks; add chapter for external check results
* [#5516](https://github.com/icinga/icinga2/issues/5516) (Documentation, PR): Updates the install dependencies for Debian 9 'stretch'
* [#5506](https://github.com/icinga/icinga2/issues/5506) (Documentation, PR): Docs: Fix wrong parameter for ITL CheckCommand nscp\_api
* [#5496](https://github.com/icinga/icinga2/issues/5496) (Documentation, PR): Docs: Update examples for match/regex/cidr\_match and mode for arrays \(Match{All,Any}\)
* [#5494](https://github.com/icinga/icinga2/issues/5494) (Documentation, PR): Docs: Add section for multiple template imports
* [#5491](https://github.com/icinga/icinga2/issues/5491) (Documentation, PR): Update "Getting Started" documentation with Alpine Linux
* [#5487](https://github.com/icinga/icinga2/issues/5487) (Documentation, PR): Docs: Enhance Troubleshooting with nscp-local, check\_source, wrong thresholds
* [#5476](https://github.com/icinga/icinga2/issues/5476) (Documentation, PR): Docs: Fix ITL chapter TOC; add introduction with mini TOC
* [#5475](https://github.com/icinga/icinga2/issues/5475) (Documentation, PR): Docs: Add a note on required configuration updates for new notification scripts in v2.7.0
* [#5461](https://github.com/icinga/icinga2/issues/5461) (Documentation, PR): Update Icinga repository release rpm location
* [#5457](https://github.com/icinga/icinga2/issues/5457) (Documentation, PR): Add Changelog generation script for GitHub API
* [#5428](https://github.com/icinga/icinga2/issues/5428) (Documentation): "Plugin Check Commands" section inside ITL docs needs adjustments

### Support

* [#5599](https://github.com/icinga/icinga2/issues/5599) (PR): changelog.py: Add "backported" to the list of ignored labels
* [#5590](https://github.com/icinga/icinga2/issues/5590) (Cluster, Log, PR): Silence log level for configuration file updates
* [#5529](https://github.com/icinga/icinga2/issues/5529) (Log, PR): Change two more loglines for checkables so checkable is quoted
* [#5528](https://github.com/icinga/icinga2/issues/5528) (Log, PR): Change loglines for checkables so checkable is quoted
* [#5501](https://github.com/icinga/icinga2/issues/5501) (Installation, Packages, PR): SELinux: fixes for 2.7.0
* [#5479](https://github.com/icinga/icinga2/issues/5479) (Packages): Icinga2 2.7.0 requires SELinux boolean icinga2\_can\_connect\_all on CentOS 7 even for default port
* [#5477](https://github.com/icinga/icinga2/issues/5477) (Installation, Packages, PR): Systemd: Add DefaultTasksMax=infinity to service file
* [#5392](https://github.com/icinga/icinga2/issues/5392) (Packages, PR): Ensure the cache directory exists
* [#4918](https://github.com/icinga/icinga2/issues/4918) (Packages): cgroup: fork rejected by pids controller in /system.slice/icinga2.service
* [#4414](https://github.com/icinga/icinga2/issues/4414) (Packages): /usr/lib/icinga2/prepare-dirs does not create /var/cache/icinga2

## 2.7.0 (2017-08-02)

### Notes

* New mail notification scripts. Please note that this requires a configuration update to NotificationCommand objects, Notification apply rules for specific settings and of course the notification scripts. More can be found [here](https://github.com/Icinga/icinga2/pull/5475).
* check_nscp_api plugin for NSClient++ REST API checks
* Work queues for features including logs & metrics
* More metrics for the "icinga" check
* Many bugfixes

### Enhancement

* [#5421](https://github.com/icinga/icinga2/issues/5421) (Plugins, Windows, PR): Windows Plugins: Add new parameter to check\_disk to show used space
* [#5348](https://github.com/icinga/icinga2/issues/5348) (Configuration, PR): Implement support for handling exceptions in user scripts
* [#5331](https://github.com/icinga/icinga2/issues/5331) (Graylog, PR): GelfWriter: Add 'check\_command' to CHECK RESULT/\* NOTIFICATION/STATE CHANGE messages
* [#5330](https://github.com/icinga/icinga2/issues/5330) (Graphite, PR): GraphiteWriter: Add 'connected' to stats; fix reconnect exceptions
* [#5329](https://github.com/icinga/icinga2/issues/5329) (Graylog, PR): GelfWriter: Use async work queue and add feature metric stats
* [#5320](https://github.com/icinga/icinga2/issues/5320) (Configuration, PR): zones.conf: Add global-templates & director-global by default
* [#5287](https://github.com/icinga/icinga2/issues/5287) (Graphite, InfluxDB, Metrics, PR): Use workqueues in Graphite and InfluxDB features
* [#5284](https://github.com/icinga/icinga2/issues/5284) (Check Execution, PR): Add feature stats to 'icinga' check as performance data metrics
* [#5280](https://github.com/icinga/icinga2/issues/5280) (API, Cluster, Log, PR): Implement WorkQueue metric stats and periodic logging
* [#5266](https://github.com/icinga/icinga2/issues/5266) (API, Cluster, PR): Add API & Cluster metric stats to /v1/status & icinga check incl. performance data
* [#5264](https://github.com/icinga/icinga2/issues/5264) (Configuration, PR): Implement new array match functionality
* [#5247](https://github.com/icinga/icinga2/issues/5247) (Log, PR): Add target object in cluster error messages to debug log
* [#5246](https://github.com/icinga/icinga2/issues/5246) (API, Cluster, PR): Add subjectAltName X509 ext for certificate requests
* [#5242](https://github.com/icinga/icinga2/issues/5242) (Configuration, PR): Allow expressions for the type in object/template declarations
* [#5241](https://github.com/icinga/icinga2/issues/5241) (InfluxDB, PR): Verbose InfluxDB Error Logging
* [#5239](https://github.com/icinga/icinga2/issues/5239) (Plugins, Windows, PR): Add NSCP API check plugin for NSClient++ HTTP API
* [#5212](https://github.com/icinga/icinga2/issues/5212) (Cluster, Log): Add additional logging for config sync
* [#5145](https://github.com/icinga/icinga2/issues/5145): Add a GitHub issue template
* [#5133](https://github.com/icinga/icinga2/issues/5133) (API, wishlist): ApiListener: Metrics for cluster data
* [#5106](https://github.com/icinga/icinga2/issues/5106) (Configuration): Add director-global as global zone to the default zones.conf configuration
* [#4945](https://github.com/icinga/icinga2/issues/4945) (API, Log): No hint for missing permissions in Icinga2 log for API user
* [#4925](https://github.com/icinga/icinga2/issues/4925): Update changelog generation scripts for GitHub
* [#4411](https://github.com/icinga/icinga2/issues/4411) (InfluxDB, Log, Metrics): Better Debugging for InfluxdbWriter
* [#4288](https://github.com/icinga/icinga2/issues/4288) (Cluster, Log): Add check information to the debuglog when check result is discarded
* [#4242](https://github.com/icinga/icinga2/issues/4242) (Configuration): Default mail notification from header
* [#3557](https://github.com/icinga/icinga2/issues/3557) (Log): Log started and stopped features 

### Bug

* [#5433](https://github.com/icinga/icinga2/issues/5433) (CLI, PR): Fix: update feature list help text
* [#5367](https://github.com/icinga/icinga2/issues/5367) (CLI, Crash): Unable to start icinga2 with kernel-3.10.0-514.21.2 RHEL7
* [#5350](https://github.com/icinga/icinga2/issues/5350) (Plugins): check\_nscp\_api not building on Debian wheezy
* [#5316](https://github.com/icinga/icinga2/issues/5316) (Livestatus, PR): Fix for stats min operator
* [#5308](https://github.com/icinga/icinga2/issues/5308) (Configuration, PR): Improve validation for attributes which must not be 'null'
* [#5297](https://github.com/icinga/icinga2/issues/5297) (PR): Fix compiler warnings
* [#5295](https://github.com/icinga/icinga2/issues/5295) (Notifications, PR): Fix missing apostrophe in notification log
* [#5292](https://github.com/icinga/icinga2/issues/5292) (PR): Build fix for OpenSSL 0.9.8 and stack\_st\_X509\_EXTENSION
* [#5288](https://github.com/icinga/icinga2/issues/5288) (Configuration): Hostgroup using assign for Host with groups = null segfault
* [#5278](https://github.com/icinga/icinga2/issues/5278) (PR): Build fix for I2\_LEAK\_DEBUG
* [#5262](https://github.com/icinga/icinga2/issues/5262) (Graylog, PR): Fix performance data processing in GelfWriter feature
* [#5259](https://github.com/icinga/icinga2/issues/5259) (API, PR): Don't allow acknowledgement expire timestamps in the past
* [#5256](https://github.com/icinga/icinga2/issues/5256) (Configuration): Config type changes break object serialization \(JsonEncode\)
* [#5250](https://github.com/icinga/icinga2/issues/5250) (API, Compat): Acknowledgement expire time in the past
* [#5245](https://github.com/icinga/icinga2/issues/5245) (Notifications, PR): Fix that host downtimes might be triggered even if their state is Up
* [#5224](https://github.com/icinga/icinga2/issues/5224) (Configuration, Notifications): Icinga sends notifications even though a Downtime object exists
* [#5223](https://github.com/icinga/icinga2/issues/5223) (Plugins, Windows): Wrong return Code for Windows ICMP
* [#5219](https://github.com/icinga/icinga2/issues/5219) (InfluxDB): InfluxDBWriter feature might block and leak memory
* [#5211](https://github.com/icinga/icinga2/issues/5211) (API, Cluster): Config received is always accepted by client even if own config is newer
* [#5194](https://github.com/icinga/icinga2/issues/5194) (API, CLI): No subjectAltName in Icinga CA created CSRs
* [#5168](https://github.com/icinga/icinga2/issues/5168) (Windows): include files from other volume/partition
* [#5146](https://github.com/icinga/icinga2/issues/5146) (Configuration): parsing of scheduled downtime object allow typing range instead of ranges
* [#5132](https://github.com/icinga/icinga2/issues/5132) (Graphite): GraphiteWriter can slow down Icinga's check result processing
* [#5062](https://github.com/icinga/icinga2/issues/5062) (Compat): icinga2 checkresults error
* [#5043](https://github.com/icinga/icinga2/issues/5043) (API): API POST request with 'attrs' as array returns bad\_cast error
* [#5040](https://github.com/icinga/icinga2/issues/5040) (Cluster): CRL loading fails due to incorrect return code check
* [#5033](https://github.com/icinga/icinga2/issues/5033) (DB IDO): Flexible downtimes which are not triggered must not update DB IDO's actual\_end\_time in downtimehistory table
* [#4984](https://github.com/icinga/icinga2/issues/4984) (API): Wrong response type when unauthorized
* [#4983](https://github.com/icinga/icinga2/issues/4983) (Livestatus): Typo in livestatus key worst\_services\_state for hostgroups table
* [#4956](https://github.com/icinga/icinga2/issues/4956) (DB IDO, PR): Fix persistent comments for Acknowledgements
* [#4941](https://github.com/icinga/icinga2/issues/4941) (Metrics, PR): PerfData: Server Timeouts for InfluxDB Writer
* [#4927](https://github.com/icinga/icinga2/issues/4927) (InfluxDB, Metrics): InfluxDbWriter error 500 hanging Icinga daemon
* [#4913](https://github.com/icinga/icinga2/issues/4913) (API): acknowledge-problem api sending notifications when notify is false
* [#4909](https://github.com/icinga/icinga2/issues/4909) (CLI): icinga2 feature disable fails on already disabled feature
* [#4896](https://github.com/icinga/icinga2/issues/4896) (Plugins): Windows Agent: performance data of check\_perfmon
* [#4832](https://github.com/icinga/icinga2/issues/4832) (API, Configuration): API max\_check\_attempts validation
* [#4818](https://github.com/icinga/icinga2/issues/4818): Acknowledgements marked with Persistent Comment are not honored
* [#4779](https://github.com/icinga/icinga2/issues/4779): Superflous error messages for non-exisiting lsb\_release/sw\_vers commands \(on NetBSD\)
* [#4778](https://github.com/icinga/icinga2/issues/4778): Fix for traditional glob\(3\) behaviour
* [#4777](https://github.com/icinga/icinga2/issues/4777): NetBSD execvpe.c fix
* [#4709](https://github.com/icinga/icinga2/issues/4709) (API): Posting config stage fails on FreeBSD
* [#4696](https://github.com/icinga/icinga2/issues/4696) (Notifications): Notifications are sent when reloading Icinga 2 even though they're deactivated via modified attributes
* [#4666](https://github.com/icinga/icinga2/issues/4666) (Graylog, Metrics): GelfWriter with enable\_send\_perfdata breaks checks
* [#4532](https://github.com/icinga/icinga2/issues/4532) (Graylog, Metrics): Icinga 2 "hangs" if the GelfWriter cannot send messages
* [#4440](https://github.com/icinga/icinga2/issues/4440) (DB IDO, Log): Exceptions might be better than exit in IDO
* [#3664](https://github.com/icinga/icinga2/issues/3664) (DB IDO): mysql\_error cannot be used for mysql\_init
* [#3483](https://github.com/icinga/icinga2/issues/3483) (Compat): Stacktrace on Command Pipe Error
* [#3410](https://github.com/icinga/icinga2/issues/3410) (Livestatus): Livestatus: Problem with stats min operator
* [#121](https://github.com/icinga/icinga2/issues/121) (CLI, PR): give only warnings if feature is already disabled

### ITL

* [#5384](https://github.com/icinga/icinga2/issues/5384) (ITL, PR): Remove default value for 'dns\_query\_type'
* [#5383](https://github.com/icinga/icinga2/issues/5383) (ITL): Monitoring-Plugins check\_dns command does not support the `-q` flag
* [#5372](https://github.com/icinga/icinga2/issues/5372) (ITL, PR): Update ITL CheckCommand description attribute, part 2
* [#5363](https://github.com/icinga/icinga2/issues/5363) (ITL, PR): Update missing description attributes for ITL CheckCommand definitions
* [#5347](https://github.com/icinga/icinga2/issues/5347) (ITL, PR): Improve ITL CheckCommand description attribute
* [#5344](https://github.com/icinga/icinga2/issues/5344) (ITL, PR): Add ip4-or-ipv6 import to logstash ITL command
* [#5343](https://github.com/icinga/icinga2/issues/5343) (ITL): logstash ITL command misses import
* [#5236](https://github.com/icinga/icinga2/issues/5236) (ITL, PR): ITL: Add some missing arguments to ssl\_cert
* [#5210](https://github.com/icinga/icinga2/issues/5210) (ITL, PR): Add report mode to db2\_health
* [#5170](https://github.com/icinga/icinga2/issues/5170) (ITL, PR): Enhance mail notifications scripts and add support for command line parameters
* [#5139](https://github.com/icinga/icinga2/issues/5139) (ITL, PR): Add more options to ldap CheckCommand
* [#5129](https://github.com/icinga/icinga2/issues/5129) (ITL): Additional parameters for perfout manubulon scripts
* [#5126](https://github.com/icinga/icinga2/issues/5126) (ITL, PR): Added support to NRPE v2 in NRPE CheckCommand
* [#5075](https://github.com/icinga/icinga2/issues/5075) (ITL, PR): fix mitigation for nwc\_health
* [#5063](https://github.com/icinga/icinga2/issues/5063) (ITL, PR): Add additional arguments to mssql\_health
* [#5046](https://github.com/icinga/icinga2/issues/5046) (ITL): Add querytype to dns check
* [#5019](https://github.com/icinga/icinga2/issues/5019) (ITL, PR): Added CheckCommand definitions for SMART, RAID controller and IPMI ping check
* [#5015](https://github.com/icinga/icinga2/issues/5015) (ITL, PR): nwc\_health\_report attribute requires a value
* [#4987](https://github.com/icinga/icinga2/issues/4987) (ITL): Review `dummy` entry in ITL
* [#4985](https://github.com/icinga/icinga2/issues/4985) (ITL): Allow hpasm command from ITL to run in local mode
* [#4964](https://github.com/icinga/icinga2/issues/4964) (ITL, PR): ITL: check\_icmp: add missing TTL attribute
* [#4839](https://github.com/icinga/icinga2/issues/4839) (ITL): Remove deprecated dns\_expected\_answer attribute
* [#4826](https://github.com/icinga/icinga2/issues/4826) (ITL): Prepare icingacli-businessprocess for next release
* [#4661](https://github.com/icinga/icinga2/issues/4661) (ITL): ITL - check\_oracle\_health - report option to shorten output
* [#124](https://github.com/icinga/icinga2/issues/124) (ITL, PR): FreeBSD's /dev/fd can either be inside devfs, or be of type fdescfs.
* [#123](https://github.com/icinga/icinga2/issues/123) (ITL, PR): ITL: Update ipmi CheckCommand attributes 
* [#120](https://github.com/icinga/icinga2/issues/120) (ITL, PR): Add new parameter for check\_http: -L: Wrap output in HTML link
* [#117](https://github.com/icinga/icinga2/issues/117) (ITL, PR): Support --only-critical for check\_apt
* [#115](https://github.com/icinga/icinga2/issues/115) (ITL, PR): Inverse Interface Switch for snmp-interface
* [#114](https://github.com/icinga/icinga2/issues/114) (ITL, PR): Adding -A to snmp interfaces check

### Documentation

* [#5448](https://github.com/icinga/icinga2/issues/5448) (Documentation, PR): Update documentation for 2.7.0
* [#5440](https://github.com/icinga/icinga2/issues/5440) (Documentation, PR): Add missing notification state filter to documentation 
* [#5425](https://github.com/icinga/icinga2/issues/5425) (Documentation, PR): Fix formatting in API docs
* [#5410](https://github.com/icinga/icinga2/issues/5410) (Documentation): Update docs for better compatibility with mkdocs
* [#5393](https://github.com/icinga/icinga2/issues/5393) (Documentation, PR): Fix typo in the documentation
* [#5378](https://github.com/icinga/icinga2/issues/5378) (Documentation, PR): Fixed warnings when using mkdocs
* [#5370](https://github.com/icinga/icinga2/issues/5370) (Documentation, PR): Rename ChangeLog to CHANGELOG.md
* [#5366](https://github.com/icinga/icinga2/issues/5366) (Documentation, PR): Fixed wrong node in documentation chapter Client/Satellite Linux Setup
* [#5365](https://github.com/icinga/icinga2/issues/5365) (Documentation, PR): Update package documentation for Debian Stretch
* [#5358](https://github.com/icinga/icinga2/issues/5358) (Documentation, PR): Add documentation for securing mysql on Debian/Ubuntu.
* [#5357](https://github.com/icinga/icinga2/issues/5357) (Documentation, Notifications, PR): Notification Scripts: Ensure that mail from address works on Debian/RHEL/SUSE \(mailutils vs mailx\)
* [#5354](https://github.com/icinga/icinga2/issues/5354) (Documentation, PR): Docs: Fix built-in template description and URLs
* [#5349](https://github.com/icinga/icinga2/issues/5349) (Documentation, PR): Docs: Fix broken format for notes/tips in CLI command chapter
* [#5339](https://github.com/icinga/icinga2/issues/5339) (Documentation, ITL, PR): Add accept\_cname to dns CheckCommand
* [#5336](https://github.com/icinga/icinga2/issues/5336) (Documentation, PR): Docs: Fix formatting issues and broken URLs
* [#5333](https://github.com/icinga/icinga2/issues/5333) (Documentation, PR): Update documentation for enhanced notification scripts
* [#5324](https://github.com/icinga/icinga2/issues/5324) (Documentation, PR): Fix phrasing in Getting Started chapter
* [#5317](https://github.com/icinga/icinga2/issues/5317) (Documentation, PR): Fix typo in INSTALL.md
* [#5315](https://github.com/icinga/icinga2/issues/5315) (Documentation, PR): Docs: Replace nagios-plugins by monitoring-plugins for Debian/Ubuntu
* [#5314](https://github.com/icinga/icinga2/issues/5314) (Documentation, PR): Document Common name \(CN\) in client setup
* [#5309](https://github.com/icinga/icinga2/issues/5309) (Documentation, PR): Docs: Replace the command pipe w/ the REST API as Icinga Web 2 requirement in 'Getting Started' chapter
* [#5291](https://github.com/icinga/icinga2/issues/5291) (Documentation): Update docs for RHEL/CentOS 5 EOL
* [#5285](https://github.com/icinga/icinga2/issues/5285) (Documentation, PR): Fix sysstat installation in troubleshooting docs
* [#5279](https://github.com/icinga/icinga2/issues/5279) (Documentation, PR): Docs: Add API query example for acknowledgements w/o expire time
* [#5275](https://github.com/icinga/icinga2/issues/5275) (Documentation, PR): Add troubleshooting hints for cgroup fork errors
* [#5244](https://github.com/icinga/icinga2/issues/5244) (Documentation, PR): Add a PR review section to CONTRIBUTING.md
* [#5237](https://github.com/icinga/icinga2/issues/5237) (Documentation, PR): Docs: Add a note for Windows debuglog to the troubleshooting chapter
* [#5227](https://github.com/icinga/icinga2/issues/5227) (Documentation, ITL, PR): feature/itl-vmware-esx-storage-path-standbyok
* [#5216](https://github.com/icinga/icinga2/issues/5216) (Documentation, PR): Remove "... is is ..." in CONTRIBUTING.md
* [#5206](https://github.com/icinga/icinga2/issues/5206) (Documentation): Typo in Getting Started Guide
* [#5203](https://github.com/icinga/icinga2/issues/5203) (Documentation, PR): Fix typo in Getting Started chapter
* [#5184](https://github.com/icinga/icinga2/issues/5184) (Documentation, PR): Doc/appendix: fix malformed markdown links
* [#5181](https://github.com/icinga/icinga2/issues/5181) (Documentation, PR): List SELinux packages required for building RPMs
* [#5178](https://github.com/icinga/icinga2/issues/5178) (Documentation, Windows): Documentation vague on "update-windows" check plugin
* [#5175](https://github.com/icinga/icinga2/issues/5175) (Documentation): Add a note about flapping problems to the docs
* [#5174](https://github.com/icinga/icinga2/issues/5174) (Documentation, PR): Add missing object type to Apply Rules doc example
* [#5173](https://github.com/icinga/icinga2/issues/5173) (Documentation): Object type missing from ping Service example in docs
* [#5167](https://github.com/icinga/icinga2/issues/5167) (Documentation): Add more assign where expression examples
* [#5166](https://github.com/icinga/icinga2/issues/5166) (API, Documentation): Set zone attribute to no\_user\_modify for API POST requests
* [#5165](https://github.com/icinga/icinga2/issues/5165) (Documentation, PR): Syntax error In Dependencies chapter
* [#5164](https://github.com/icinga/icinga2/issues/5164) (Documentation, ITL, PR): ITL: Add CheckCommand ssl\_cert, fix ssl attributes
* [#5161](https://github.com/icinga/icinga2/issues/5161) (Documentation, PR): ITL documentation - disk-windows usage note with % thresholds
* [#5157](https://github.com/icinga/icinga2/issues/5157) (Documentation): "Three Levels with master, Satellites, and Clients" chapter is not clear about client config
* [#5156](https://github.com/icinga/icinga2/issues/5156) (Documentation): Add CONTRIBUTING.md
* [#5155](https://github.com/icinga/icinga2/issues/5155) (Documentation): 3.5. Apply Rules topic in the docs needs work.
* [#5151](https://github.com/icinga/icinga2/issues/5151) (Documentation, PR): Replace http:// links with https:// links where a secure website exists
* [#5150](https://github.com/icinga/icinga2/issues/5150) (Documentation): Invalid links in documentation
* [#5149](https://github.com/icinga/icinga2/issues/5149) (Documentation, PR): Update documentation, change http:// links to https:// links where a website exists
* [#5144](https://github.com/icinga/icinga2/issues/5144) (Documentation): Extend troubleshooting docs w/ environment analysis and common tools
* [#5143](https://github.com/icinga/icinga2/issues/5143) (Documentation): Docs: Explain how to include your own config tree instead of conf.d
* [#5142](https://github.com/icinga/icinga2/issues/5142) (Documentation): Add an Elastic Stack Integrations chapter to feature documentation
* [#5140](https://github.com/icinga/icinga2/issues/5140) (Documentation): Documentation should explain that runtime modifications are not immediately updated for "object list"
* [#5137](https://github.com/icinga/icinga2/issues/5137) (Documentation): Doc updates: Getting Started w/ own config, Troubleshooting w/ debug console
* [#5111](https://github.com/icinga/icinga2/issues/5111) (Documentation): Fix duration attribute requirement for schedule-downtime API action
* [#5104](https://github.com/icinga/icinga2/issues/5104) (Documentation, PR): Correct link to nscp documentation
* [#5097](https://github.com/icinga/icinga2/issues/5097) (Documentation): The last example for typeof\(\) is missing the result
* [#5090](https://github.com/icinga/icinga2/issues/5090) (Cluster, Documentation): EventHandler to be executed at the endpoint
* [#5077](https://github.com/icinga/icinga2/issues/5077) (Documentation): Replace the 'command' feature w/ the REST API for Icinga Web 2
* [#5016](https://github.com/icinga/icinga2/issues/5016) (Documentation, ITL, PR): Add fuse.gvfs-fuse-daemon to disk\_exclude\_type
* [#5010](https://github.com/icinga/icinga2/issues/5010) (Documentation): \[Documentation\] Missing parameter for SNMPv3 auth
* [#3560](https://github.com/icinga/icinga2/issues/3560) (Documentation): Explain check\_memorys and check\_disks thresholds
* [#1880](https://github.com/icinga/icinga2/issues/1880) (Documentation): add a section for 'monitoring the icinga2 node'

### Support

* [#5359](https://github.com/icinga/icinga2/issues/5359) (CLI, PR): Fixed missing closing bracket in CLI command pki new-cert.
* [#5332](https://github.com/icinga/icinga2/issues/5332) (Configuration, Notifications, PR): Notification Scripts: notification\_type is always required
* [#5326](https://github.com/icinga/icinga2/issues/5326) (Documentation, Installation, PR): Install the images directory containing the needed PNGs for the markd
* [#5310](https://github.com/icinga/icinga2/issues/5310) (Packages, PR): RPM: Disable SELinux policy hardlink
* [#5306](https://github.com/icinga/icinga2/issues/5306) (Documentation, Packages, PR): Remove CentOS 5 from 'Getting started' docs
* [#5304](https://github.com/icinga/icinga2/issues/5304) (Documentation, Packages, PR): Update INSTALL.md for RPM builds
* [#5303](https://github.com/icinga/icinga2/issues/5303) (Packages, PR): RPM: Fix builds on Amazon Linux
* [#5299](https://github.com/icinga/icinga2/issues/5299) (Notifications): Ensure that "mail from" works on RHEL/CentOS
* [#5286](https://github.com/icinga/icinga2/issues/5286) (Configuration, PR): Fix verbose mode in notifications scripts
* [#5265](https://github.com/icinga/icinga2/issues/5265) (PR): Move PerfdataValue\(\) class into base library
* [#5252](https://github.com/icinga/icinga2/issues/5252) (Tests, PR): travis: Update to trusty as CI environment
* [#5251](https://github.com/icinga/icinga2/issues/5251) (Tests): Update Travis CI environment to trusty
* [#5248](https://github.com/icinga/icinga2/issues/5248) (Tests, PR): Travis: Run config validation at the end
* [#5238](https://github.com/icinga/icinga2/issues/5238) (DB IDO, PR): Remove deprecated "DbCat1 | DbCat2" notation for DB IDO categories
* [#5229](https://github.com/icinga/icinga2/issues/5229) (Installation, PR): CMake: require a GCC version according to INSTALL.md
* [#5226](https://github.com/icinga/icinga2/issues/5226) (Packages, PR): RPM spec: don't enable features after an upgrade
* [#5225](https://github.com/icinga/icinga2/issues/5225) (DB IDO, PR): Don't call mysql\_error\(\) after a failure of mysql\_init\(\)
* [#5218](https://github.com/icinga/icinga2/issues/5218) (Packages): icinga2.spec: Allow selecting g++ compiler on older SUSE release builds
* [#5189](https://github.com/icinga/icinga2/issues/5189) (Documentation, Packages, PR): RPM packaging updates
* [#5188](https://github.com/icinga/icinga2/issues/5188) (Documentation, Packages): Boost \>= 1.48 required
* [#5177](https://github.com/icinga/icinga2/issues/5177) (Packages): Issues Packing icinga 2.6.3 tar.gz to RPM
* [#5153](https://github.com/icinga/icinga2/issues/5153) (Packages, PR): Changed dependency of selinux subpackage
* [#5127](https://github.com/icinga/icinga2/issues/5127) (Installation, PR): Improve systemd service file
* [#5102](https://github.com/icinga/icinga2/issues/5102) (Compat, Configuration, Packages): Deprecate the icinga2-classicui-config package
* [#5101](https://github.com/icinga/icinga2/issues/5101) (Packages, Windows): Fix incorrect metadata for the Chocolatey package
* [#5100](https://github.com/icinga/icinga2/issues/5100) (Packages, Windows): Update Chocolatey package to match current guidelines
* [#5094](https://github.com/icinga/icinga2/issues/5094) (Cluster, Configuration): Log message "Object cannot be deleted because it was not created using the API"
* [#5087](https://github.com/icinga/icinga2/issues/5087) (Configuration): Function metadata should show available arguments
* [#5042](https://github.com/icinga/icinga2/issues/5042) (DB IDO, PR): Add link to upgrade documentation to log message
* [#4977](https://github.com/icinga/icinga2/issues/4977) (Cluster, Installation): icinga2/api/log directory is not created
* [#4921](https://github.com/icinga/icinga2/issues/4921) (Installation, Packages): No network dependency for /etc/init.d/icinga2
* [#4781](https://github.com/icinga/icinga2/issues/4781) (Packages): Improve SELinux Policy
* [#4776](https://github.com/icinga/icinga2/issues/4776) (Installation): NetBSD install path fixes
* [#4621](https://github.com/icinga/icinga2/issues/4621) (Configuration, Notifications, Packages): notifications always enabled after update

## 2.6.3 (2017-03-29)

### Bug

* [#5080](https://github.com/icinga/icinga2/issues/5080) (DB IDO): Missing index use can cause icinga\_downtimehistory queries to hang indefinitely
* [#4989](https://github.com/icinga/icinga2/issues/4989) (Check Execution): Icinga daemon runs with nice 5 after reload
* [#4930](https://github.com/icinga/icinga2/issues/4930) (Cluster): Change "Discarding 'config update object'" log messages to notice log level
* [#4603](https://github.com/icinga/icinga2/issues/4603) (DB IDO): With too many comments, Icinga reload process won't finish reconnecting to Database

### Documentation

* [#5057](https://github.com/icinga/icinga2/issues/5057) (Documentation): Update Security section in the Distributed Monitoring chapter
* [#5055](https://github.com/icinga/icinga2/issues/5055) (Documentation, ITL): mysql\_socket attribute missing in the documentation for the mysql CheckCommand
* [#5035](https://github.com/icinga/icinga2/issues/5035) (Documentation): Docs: Typo in Distributed Monitoring chapter
* [#5030](https://github.com/icinga/icinga2/issues/5030) (Documentation): Advanced topics: Mention the API and explain stick acks, fixed/flexible downtimes
* [#5029](https://github.com/icinga/icinga2/issues/5029) (Documentation): Advanced topics: Wrong acknowledgement notification filter
* [#4996](https://github.com/icinga/icinga2/issues/4996) (Documentation): documentation: mixed up host names in 6-distributed-monitoring.md
* [#4980](https://github.com/icinga/icinga2/issues/4980) (Documentation): Add OpenBSD and AlpineLinux package repositories to the documentation
* [#4955](https://github.com/icinga/icinga2/issues/4955) (Documentation, ITL): Review CheckCommand documentation including external URLs
* [#4954](https://github.com/icinga/icinga2/issues/4954) (Documentation): Add an example for /v1/actions/process-check-result which uses filter/type
* [#3133](https://github.com/icinga/icinga2/issues/3133) (Documentation): Add practical examples for apply expressions

## 2.6.2 (2017-02-13)

### Bug

* [#4952](https://github.com/icinga/icinga2/issues/4952) (API, CLI): Icinga crashes while trying to remove configuration files for objects which no longer exist

## 2.6.1 (2017-01-31)

### Notes

This release addresses a number of bugs we have identified in version 2.6.0.

The documentation changes reflect our recent move to GitHub.

### Enhancement

* [#4923](https://github.com/icinga/icinga2/issues/4923): Migration to Github
* [#4813](https://github.com/icinga/icinga2/issues/4813): Include argument name for log message about incorrect set\_if values

### Bug

* [#4950](https://github.com/icinga/icinga2/issues/4950): IDO schema update is not compatible to MySQL 5.7
* [#4882](https://github.com/icinga/icinga2/issues/4882): Crash - Error: parse error: premature EOF
* [#4877](https://github.com/icinga/icinga2/issues/4877) (DB IDO): IDO MySQL schema not working on MySQL 5.7
* [#4874](https://github.com/icinga/icinga2/issues/4874) (DB IDO): IDO: Timestamps in PostgreSQL may still have a time zone offset
* [#4867](https://github.com/icinga/icinga2/issues/4867): SIGPIPE shutdown on config reload

### Documentation

* [#4944](https://github.com/icinga/icinga2/issues/4944) (Documentation, PR): doc/6-distributed-monitoring.md: Fix typo
* [#4934](https://github.com/icinga/icinga2/issues/4934) (Documentation): Update contribution section for GitHub
* [#4917](https://github.com/icinga/icinga2/issues/4917) (Documentation): Incorrect license file mentioned in README.md
* [#4916](https://github.com/icinga/icinga2/issues/4916) (Documentation): Add travis-ci build status logo to README.md
* [#4908](https://github.com/icinga/icinga2/issues/4908) (Documentation): Move domain to icinga.com
* [#4885](https://github.com/icinga/icinga2/issues/4885) (Documentation): SLES 12 SP2 libboost\_thread package requires libboost\_chrono
* [#4869](https://github.com/icinga/icinga2/issues/4869) (Documentation): Update RELEASE.md
* [#4868](https://github.com/icinga/icinga2/issues/4868) (Documentation): Add more build details to INSTALL.md
* [#4803](https://github.com/icinga/icinga2/issues/4803) (Documentation): Update Repositories in Docs

### Support

* [#4870](https://github.com/icinga/icinga2/issues/4870) (Packages): SLES11 SP4 dependency on Postgresql \>= 8.4

## 2.6.0 (2016-12-13)

### Notes

* Client/Satellite setup
 * The "bottom up" client configuration mode has been deprecated. Check [#13255](https://dev.icinga.com/issues/13255) for additional details and migration.
* Linux/Unix daemon
 * Ensure that Icinga 2 does not leak file descriptors to executed commands.
 * There are 2 processes started instead of previously just one process.
* Windows client
 * Package bundles NSClient++ 0.5.0. ITL CheckCommands have been updated too.
 * Allow to configure the user account for the Icinga 2 service. This is useful if several checks require administrator permissions (e.g. check_update.exe)
 * Bugfixes for check plugins
* Cluster and API
 * Provide location information for objects and templates in the API
 * Improve log message for ignored config updates
 * Fix cluster resync problem with API created objects (hosts, downtimes, etc.)
 * Fix that API-created objects in a global zone are not synced to child endpoints
* Notifications
 * Several bugfixes for downtime, custom and flapping notifications
* New ITL CheckCommands: logstash, glusterfs, iostats
* Package builds require a compiler which supports C++11 features (gcc-c++ >= 4.7, clang++)
* DB IDO
 * Schema upgrade required (2.6.0.sql)
 * This update fixes timestamp columns required by Icinga Web 2 and might take a while. Please ensure to schedule a maintenance task for your database upgrade.

### Enhancement

* [#4798](https://github.com/icinga/icinga2/issues/4798) (Cluster): Deprecate cluster/client mode "bottom up" w/ repository.d and node update-config
* [#4770](https://github.com/icinga/icinga2/issues/4770) (API): Allow to evaluate macros through the API
* [#4713](https://github.com/icinga/icinga2/issues/4713) (Cluster): Check whether nodes are synchronizing the API log before putting them into UNKNOWN
* [#4651](https://github.com/icinga/icinga2/issues/4651) (Plugins): Review windows plugins performance output
* [#4631](https://github.com/icinga/icinga2/issues/4631) (Configuration): Suppress compiler warnings for auto-generated code
* [#4622](https://github.com/icinga/icinga2/issues/4622) (Cluster): Improve log message for ignored config updates
* [#4590](https://github.com/icinga/icinga2/issues/4590): Make sure that libmethods is automatically loaded even when not using the ITL
* [#4587](https://github.com/icinga/icinga2/issues/4587) (Configuration): Implement support for default templates
* [#4580](https://github.com/icinga/icinga2/issues/4580) (API): Provide location information for objects and templates in the API
* [#4576](https://github.com/icinga/icinga2/issues/4576): Use lambda functions for INITIALIZE\_ONCE
* [#4575](https://github.com/icinga/icinga2/issues/4575): Use 'auto' for iterator declarations
* [#4571](https://github.com/icinga/icinga2/issues/4571): Implement an rvalue constructor for the String and Value classes
* [#4570](https://github.com/icinga/icinga2/issues/4570) (Configuration): Implement a command-line argument for "icinga2 console" to allow specifying a script file
* [#4563](https://github.com/icinga/icinga2/issues/4563) (Configuration): Remove unused method: ApplyRule::DiscardRules
* [#4559](https://github.com/icinga/icinga2/issues/4559): Replace BOOST\_FOREACH with range-based for loops
* [#4557](https://github.com/icinga/icinga2/issues/4557): Add -fvisibility=hidden to the default compiler flags
* [#4537](https://github.com/icinga/icinga2/issues/4537): Implement an environment variable to keep Icinga from closing FDs on startup
* [#4536](https://github.com/icinga/icinga2/issues/4536): Avoid unnecessary string copies
* [#4535](https://github.com/icinga/icinga2/issues/4535): Remove deprecated functions
* [#3684](https://github.com/icinga/icinga2/issues/3684) (Configuration): Command line option for config syntax validation
* [#2968](https://github.com/icinga/icinga2/issues/2968): Better message for apply errors

### Bug

* [#4831](https://github.com/icinga/icinga2/issues/4831) (CLI): Wrong help string for node setup cli command argument --master\_host
* [#4828](https://github.com/icinga/icinga2/issues/4828) (API): Crash in CreateObjectHandler \(regression from \#11684
* [#4802](https://github.com/icinga/icinga2/issues/4802): Icinga tries to delete Downtime objects that were statically configured
* [#4801](https://github.com/icinga/icinga2/issues/4801): Sending a HUP signal to the child process for execution actually kills it
* [#4791](https://github.com/icinga/icinga2/issues/4791) (DB IDO): PostgreSQL: Don't use timestamp with timezone for UNIX timestamp columns
* [#4789](https://github.com/icinga/icinga2/issues/4789) (Notifications): Recovery notifications sent for Not-Problem notification type if notified before
* [#4775](https://github.com/icinga/icinga2/issues/4775) (Cluster): Crash w/ SendNotifications cluster handler and check result with empty perfdata
* [#4771](https://github.com/icinga/icinga2/issues/4771): Config validation crashes when using command\_endpoint without also having an ApiListener object
* [#4752](https://github.com/icinga/icinga2/issues/4752) (Graphite): Performance data writer for Graphite : Values without fraction limited to 2147483647 \(7FFFFFFF\)
* [#4740](https://github.com/icinga/icinga2/issues/4740): SIGALRM handling may be affected by recent commit
* [#4726](https://github.com/icinga/icinga2/issues/4726) (Notifications): Flapping notifications sent for soft state changes
* [#4717](https://github.com/icinga/icinga2/issues/4717) (API): Icinga crashes while deleting a config file which doesn't exist anymore
* [#4678](https://github.com/icinga/icinga2/issues/4678) (Configuration): Configuration validation fails when setting tls\_protocolmin to TLSv1.2
* [#4674](https://github.com/icinga/icinga2/issues/4674) (CLI): Parse error: "premature EOF" when running "icinga2 node update-config"
* [#4665](https://github.com/icinga/icinga2/issues/4665): Crash in ClusterEvents::SendNotificationsAPIHandler
* [#4646](https://github.com/icinga/icinga2/issues/4646) (Notifications): Forced custom notification is setting "force\_next\_notification": true permanently
* [#4644](https://github.com/icinga/icinga2/issues/4644) (API): Crash in HttpRequest::Parse while processing HTTP request
* [#4630](https://github.com/icinga/icinga2/issues/4630) (Configuration): Validation does not highlight the correct attribute
* [#4629](https://github.com/icinga/icinga2/issues/4629) (CLI): broken: icinga2 --version
* [#4620](https://github.com/icinga/icinga2/issues/4620) (API): Invalid API filter error messages
* [#4619](https://github.com/icinga/icinga2/issues/4619) (CLI): Cli: boost::bad\_get on icinga::String::String\(icinga::Value&&\) 
* [#4616](https://github.com/icinga/icinga2/issues/4616): Build fails with Visual Studio 2015
* [#4606](https://github.com/icinga/icinga2/issues/4606): Remove unused last\_in\_downtime field
* [#4602](https://github.com/icinga/icinga2/issues/4602) (CLI): Last option highlighted as the wrong one, even when it is not the culprit
* [#4599](https://github.com/icinga/icinga2/issues/4599): Unexpected state changes with max\_check\_attempts = 2
* [#4583](https://github.com/icinga/icinga2/issues/4583) (Configuration): Debug hints for dictionary expressions are nested incorrectly
* [#4574](https://github.com/icinga/icinga2/issues/4574) (Notifications): Don't send Flapping\* notifications when downtime is active
* [#4573](https://github.com/icinga/icinga2/issues/4573) (DB IDO): Getting error during schema update 
* [#4572](https://github.com/icinga/icinga2/issues/4572) (Configuration): Config validation shouldnt allow 'endpoints = \[ "" \]'
* [#4566](https://github.com/icinga/icinga2/issues/4566) (Notifications): Fixed downtimes scheduled for a future date trigger DOWNTIMESTART notifications
* [#4564](https://github.com/icinga/icinga2/issues/4564): Add missing initializer for WorkQueue::m\_NextTaskID
* [#4555](https://github.com/icinga/icinga2/issues/4555): Fix compiler warnings
* [#4541](https://github.com/icinga/icinga2/issues/4541) (DB IDO): Don't link against libmysqlclient\_r
* [#4538](https://github.com/icinga/icinga2/issues/4538): Don't update TimePeriod ranges for inactive objects
* [#4423](https://github.com/icinga/icinga2/issues/4423) (Metrics): InfluxdbWriter does not write state other than 0
* [#4369](https://github.com/icinga/icinga2/issues/4369) (Plugins): check\_network performance data in invalid format - ingraph
* [#4169](https://github.com/icinga/icinga2/issues/4169) (Cluster): Cluster resync problem with API created objects
* [#4098](https://github.com/icinga/icinga2/issues/4098) (API): Objects created in a global zone are not synced to child endpoints
* [#4010](https://github.com/icinga/icinga2/issues/4010) (API): API requests from execute-script action are too verbose
* [#3802](https://github.com/icinga/icinga2/issues/3802) (Compat): SCHEDULE\_AND\_PROPAGATE\_HOST\_DOWNTIME command missing
* [#3801](https://github.com/icinga/icinga2/issues/3801) (Compat): SCHEDULE\_AND\_PROPAGATE\_TRIGGERED\_HOST\_DOWNTIME command missing
* [#3575](https://github.com/icinga/icinga2/issues/3575) (DB IDO): MySQL 5.7.9, Incorrect datetime value Error
* [#3565](https://github.com/icinga/icinga2/issues/3565) (Plugins): Windows Agent: performance data of check\_perfmon
* [#3564](https://github.com/icinga/icinga2/issues/3564) (Plugins): Windows Agent: Performance data values for check\_perfmon.exe are invalid sometimes
* [#3220](https://github.com/icinga/icinga2/issues/3220) (Plugins): Implement support for resolving DNS hostnames in check\_ping.exe
* [#2847](https://github.com/icinga/icinga2/issues/2847): File descriptors are leaked to child processes which makes SELinux unhappy

### ITL

* [#4842](https://github.com/icinga/icinga2/issues/4842) (ITL): Add tempdir attribute to postgres CheckCommand
* [#4837](https://github.com/icinga/icinga2/issues/4837) (ITL): Add sudo option to mailq CheckCommand
* [#4836](https://github.com/icinga/icinga2/issues/4836) (ITL): Add verbose parameter to http CheckCommand
* [#4835](https://github.com/icinga/icinga2/issues/4835) (ITL): Add timeout option to mysql\_health CheckCommand
* [#4714](https://github.com/icinga/icinga2/issues/4714) (ITL): Default values for check\_swap are incorrect
* [#4710](https://github.com/icinga/icinga2/issues/4710) (ITL): snmp\_miblist variable to feed the -m option of check\_snmp is missing in the snmpv3 CheckCommand object
* [#4684](https://github.com/icinga/icinga2/issues/4684) (ITL): Add a radius CheckCommand for the radius check provide by nagios-plugins
* [#4681](https://github.com/icinga/icinga2/issues/4681) (ITL): Add CheckCommand definition for check\_logstash
* [#4677](https://github.com/icinga/icinga2/issues/4677) (ITL): Problem passing arguments to nscp-local CheckCommand objects
* [#4672](https://github.com/icinga/icinga2/issues/4672) (ITL): Add timeout option to oracle\_health CheckCommand
* [#4618](https://github.com/icinga/icinga2/issues/4618) (ITL): Hangman easter egg is broken
* [#4608](https://github.com/icinga/icinga2/issues/4608) (ITL): Add CheckCommand definition for check\_iostats
* [#4597](https://github.com/icinga/icinga2/issues/4597) (ITL): Default disk plugin check should not check inodes
* [#4595](https://github.com/icinga/icinga2/issues/4595) (ITL): Manubulon: Add missing procurve memory flag
* [#4585](https://github.com/icinga/icinga2/issues/4585) (ITL): Fix code style violations in the ITL
* [#4582](https://github.com/icinga/icinga2/issues/4582) (ITL): Incorrect help text for check\_swap
* [#4543](https://github.com/icinga/icinga2/issues/4543) (ITL): ITL - check\_vmware\_esx - specify a datacenter/vsphere server for esx/host checks
* [#4324](https://github.com/icinga/icinga2/issues/4324) (ITL): Add CheckCommand definition for check\_glusterfs

### Documentation

* [#4862](https://github.com/icinga/icinga2/issues/4862) (Documentation): "2.1.4. Installation Paths" should contain systemd paths
* [#4861](https://github.com/icinga/icinga2/issues/4861) (Documentation): Update "2.1.3. Enabled Features during Installation" - outdated "feature list"
* [#4859](https://github.com/icinga/icinga2/issues/4859) (Documentation): Update package instructions for Fedora
* [#4851](https://github.com/icinga/icinga2/issues/4851) (Documentation): Update README.md and correct project URLs
* [#4846](https://github.com/icinga/icinga2/issues/4846) (Documentation): Add a note for boolean values in the disk CheckCommand section
* [#4845](https://github.com/icinga/icinga2/issues/4845) (Documentation): Troubleshooting: Add examples for fetching the executed command line
* [#4840](https://github.com/icinga/icinga2/issues/4840) (Documentation): Update Windows screenshots in the client documentation
* [#4838](https://github.com/icinga/icinga2/issues/4838) (Documentation): Add example for concurrent\_checks in CheckerComponent object type
* [#4829](https://github.com/icinga/icinga2/issues/4829) (Documentation): Missing API headers for X-HTTP-Method-Override
* [#4827](https://github.com/icinga/icinga2/issues/4827) (Documentation): Fix example in PNP template docs
* [#4821](https://github.com/icinga/icinga2/issues/4821) (Documentation): Add a note about removing "conf.d" on the client for "top down command endpoint" setups
* [#4809](https://github.com/icinga/icinga2/issues/4809) (Documentation): Update API and Library Reference chapters
* [#4804](https://github.com/icinga/icinga2/issues/4804) (Documentation): Add a note about default template import to the CheckCommand object
* [#4800](https://github.com/icinga/icinga2/issues/4800) (Documentation): Docs: Typo in "CLI commands" chapter
* [#4793](https://github.com/icinga/icinga2/issues/4793) (Documentation): Docs: ITL plugins contrib order
* [#4787](https://github.com/icinga/icinga2/issues/4787) (Documentation): Doc: Swap packages.icinga.org w/ DebMon
* [#4780](https://github.com/icinga/icinga2/issues/4780) (Documentation): Add a note about pinning checks w/ command\_endpoint
* [#4736](https://github.com/icinga/icinga2/issues/4736) (Documentation): Docs: wrong heading level for commands.conf and groups.conf
* [#4708](https://github.com/icinga/icinga2/issues/4708) (Documentation): Add more Timeperiod examples in the documentation
* [#4706](https://github.com/icinga/icinga2/issues/4706) (Documentation): Add an example of multi-parents configuration for the Migration chapter
* [#4705](https://github.com/icinga/icinga2/issues/4705) (Documentation): Typo in the documentation
* [#4699](https://github.com/icinga/icinga2/issues/4699) (Documentation): Fix some spelling mistakes
* [#4667](https://github.com/icinga/icinga2/issues/4667) (Documentation): Add documentation for logrotation for the mainlog feature
* [#4653](https://github.com/icinga/icinga2/issues/4653) (Documentation): Corrections for distributed monitoring chapter
* [#4641](https://github.com/icinga/icinga2/issues/4641) (Documentation): Docs: Migrating Notification example tells about filters instead of types
* [#4639](https://github.com/icinga/icinga2/issues/4639) (Documentation): GDB example in the documentation isn't working
* [#4636](https://github.com/icinga/icinga2/issues/4636) (Documentation): Add development docs for writing a core dump file
* [#4601](https://github.com/icinga/icinga2/issues/4601) (Documentation): Typo in distributed monitoring docs
* [#4596](https://github.com/icinga/icinga2/issues/4596) (Documentation): Update service monitoring and distributed docs
* [#4589](https://github.com/icinga/icinga2/issues/4589) (Documentation): Fix help output for update-links.py
* [#4584](https://github.com/icinga/icinga2/issues/4584) (Documentation): Add missing reference to libmethods for the default ITL command templates
* [#4492](https://github.com/icinga/icinga2/issues/4492) (Documentation): Add information about function 'range'

### Support

* [#4796](https://github.com/icinga/icinga2/issues/4796) (Installation): Sort Changelog by category
* [#4792](https://github.com/icinga/icinga2/issues/4792) (Tests): Add unit test for notification state/type filter checks
* [#4724](https://github.com/icinga/icinga2/issues/4724) (Packages): Update .mailmap for icinga.com
* [#4671](https://github.com/icinga/icinga2/issues/4671) (Packages): Windows Installer should include NSClient++ 0.5.0
* [#4612](https://github.com/icinga/icinga2/issues/4612) (Tests): Unit tests randomly crash after the tests have completed
* [#4607](https://github.com/icinga/icinga2/issues/4607) (Packages): Improve support for building the chocolatey package
* [#4588](https://github.com/icinga/icinga2/issues/4588) (Installation): Use raw string literals in mkembedconfig
* [#4578](https://github.com/icinga/icinga2/issues/4578) (Installation): Improve detection for the -flto compiler flag
* [#4569](https://github.com/icinga/icinga2/issues/4569) (Installation): Set versions for all internal libraries
* [#4558](https://github.com/icinga/icinga2/issues/4558) (Installation): Update cmake config to require a compiler that supports C++11
* [#4556](https://github.com/icinga/icinga2/issues/4556) (Installation): logrotate file is not properly generated when the logrotate binary resides in /usr/bin
* [#4551](https://github.com/icinga/icinga2/issues/4551) (Tests): Implement unit tests for state changes
* [#2943](https://github.com/icinga/icinga2/issues/2943) (Installation): Make the user account configurable for the Windows service
* [#2792](https://github.com/icinga/icinga2/issues/2792) (Tests): Livestatus tests don't work on OS X

## 2.5.4 (2016-08-30)

### Notes

* Bugfixes

### Bug

* [#4277](https://github.com/icinga/icinga2/issues/4277): many check commands executed at same time when master reload

## 2.5.3 (2016-08-25)

### Notes

This release addresses an issue with PostgreSQL support for the IDO database module.

### Bug

* [#4554](https://github.com/icinga/icinga2/issues/4554) (DB IDO): ido pgsql migration from 2.4.0 to 2.5.0 : wrong size for config\_hash

## 2.5.2 (2016-08-24)

### Notes

* Bugfixes

### Bug

* [#4550](https://github.com/icinga/icinga2/issues/4550): Icinga 2 sends SOFT recovery notifications
* [#4549](https://github.com/icinga/icinga2/issues/4549) (DB IDO): Newly added group member tables in the IDO database are not updated

### Documentation

* [#4548](https://github.com/icinga/icinga2/issues/4548) (Documentation): Wrong formatting in client docs

## 2.5.1 (2016-08-23)

### Notes

* Bugfixes

### Bug

* [#4544](https://github.com/icinga/icinga2/issues/4544) (Notifications): Icinga 2 sends recovery notifications for SOFT NOT-OK states

## 2.5.0 (2016-08-23)

### Notes

* InfluxdbWriter feature
* API
    * New endpoints: /v1/variables and /v1/templates (GET requests), /v1/action/generate-ticket (POST request)
    * State/type filters for notifications/users are now string values (PUT, POST, GET requests)
* Configuration
    * TimePeriod excludes/includes attributes
    * DateTime object for formatting time strings
    * New prototype methods: Array#filter, Array#unique, Array#map, Array#reduce
    * icinga2.conf now includes plugins-contrib, manubulon, windows-plugins, nscp by default (ITL CheckCommand definitions)
    * Performance improvements (config compiler and validation)
* CLI
    * 'icinga2 object list' formats state/type filters as string values
    * Compiled config files are now visible with "notice" debug level (hidden by default)
    * CA serial file now uses a hash value (HA cluster w/ 2 CA directories)
* Cluster
    * There is a known issue with >2 endpoints inside a zone. Icinga 2 will now log a warning.
    * Support for accepted ciphers and minimum TLS version
    * Connection and error logging has been improved.
* DB IDO
    * Schema upgrade required (2.5.0.sql)
    * Incremental config dump (performance boost)
    * `categories` attribute is now an array. Previous method is deprecated and to be removed.
    * DbCatLog is not enabled by default anymore.
    * SSL support for MySQL
* New packages
    * vim-icinga2 for syntax highlighting
    * libicinga2 (Debian), icinga2-libs (RPM) for Icinga Studio packages

### Enhancement

* [#4516](https://github.com/icinga/icinga2/issues/4516): Remove some unused \#includes
* [#4498](https://github.com/icinga/icinga2/issues/4498): Remove unnecessary Dictionary::Contains calls
* [#4493](https://github.com/icinga/icinga2/issues/4493) (Cluster): Improve performance for Endpoint config validation
* [#4491](https://github.com/icinga/icinga2/issues/4491): Improve performance for type lookups
* [#4487](https://github.com/icinga/icinga2/issues/4487) (DB IDO): Incremental updates for the IDO database
* [#4486](https://github.com/icinga/icinga2/issues/4486) (DB IDO): Remove unused code from the IDO classes
* [#4485](https://github.com/icinga/icinga2/issues/4485) (API): Add API action for generating a PKI ticket
* [#4479](https://github.com/icinga/icinga2/issues/4479) (Configuration): Implement comparison operators for the Array class
* [#4467](https://github.com/icinga/icinga2/issues/4467): Implement the System\#sleep function
* [#4465](https://github.com/icinga/icinga2/issues/4465) (Configuration): Implement support for namespaces
* [#4464](https://github.com/icinga/icinga2/issues/4464) (CLI): Implement support for inspecting variables with LLDB/GDB
* [#4457](https://github.com/icinga/icinga2/issues/4457): Implement support for marking functions as deprecated
* [#4454](https://github.com/icinga/icinga2/issues/4454): Include compiler name/version and build host name in --version
* [#4451](https://github.com/icinga/icinga2/issues/4451) (Configuration): Move internal script functions into the 'Internal' namespace
* [#4449](https://github.com/icinga/icinga2/issues/4449): Improve logging for the WorkQueue class
* [#4445](https://github.com/icinga/icinga2/issues/4445): Rename/Remove experimental script functions
* [#4443](https://github.com/icinga/icinga2/issues/4443): Implement process\_check\_result script method for the Checkable class
* [#4442](https://github.com/icinga/icinga2/issues/4442) (API): Support for determining the Icinga 2 version via the API
* [#4431](https://github.com/icinga/icinga2/issues/4431) (Notifications): Add the notification type into the log message
* [#4424](https://github.com/icinga/icinga2/issues/4424) (Cluster): Enhance TLS handshake error messages with connection information
* [#4415](https://github.com/icinga/icinga2/issues/4415) (API): Remove obsolete debug log message
* [#4410](https://github.com/icinga/icinga2/issues/4410) (Configuration): Add map/reduce and filter functionality for the Array class
* [#4403](https://github.com/icinga/icinga2/issues/4403) (CLI): Add history for icinga2 console
* [#4398](https://github.com/icinga/icinga2/issues/4398) (Cluster): Log a warning if there are more than 2 zone endpoint members
* [#4393](https://github.com/icinga/icinga2/issues/4393) (Cluster): Include IP address and port in the "New connection" log message
* [#4388](https://github.com/icinga/icinga2/issues/4388) (Configuration): Implement the \_\_ptr script function
* [#4386](https://github.com/icinga/icinga2/issues/4386) (Cluster): Improve error messages for failed certificate validation
* [#4381](https://github.com/icinga/icinga2/issues/4381) (Cluster): Improve log message for connecting nodes without configured Endpoint object
* [#4352](https://github.com/icinga/icinga2/issues/4352) (Cluster): Enhance client disconnect message for "No data received on new API connection."
* [#4348](https://github.com/icinga/icinga2/issues/4348) (DB IDO): Do not populate logentries table by default
* [#4325](https://github.com/icinga/icinga2/issues/4325) (API): API: Add missing downtime\_depth attribute
* [#4314](https://github.com/icinga/icinga2/issues/4314) (DB IDO): Change Ido\*Connection 'categories' attribute to an array
* [#4295](https://github.com/icinga/icinga2/issues/4295) (DB IDO): Enhance IDO check with schema version info
* [#4294](https://github.com/icinga/icinga2/issues/4294) (DB IDO): Update DB IDO schema version to 1.14.1
* [#4290](https://github.com/icinga/icinga2/issues/4290) (API): Implement support for getting a list of global variables from the API
* [#4281](https://github.com/icinga/icinga2/issues/4281) (API): Support for enumerating available templates via the API
* [#4268](https://github.com/icinga/icinga2/issues/4268) (Metrics): InfluxDB Metadata
* [#4206](https://github.com/icinga/icinga2/issues/4206) (Cluster): Add lag threshold for cluster-zone check
* [#4178](https://github.com/icinga/icinga2/issues/4178) (API): Improve logging for HTTP API requests
* [#4154](https://github.com/icinga/icinga2/issues/4154) (Configuration): Remove the \(unused\) 'inherits' keyword
* [#4129](https://github.com/icinga/icinga2/issues/4129) (Configuration): Improve performance for field accesses
* [#4061](https://github.com/icinga/icinga2/issues/4061) (Configuration): Allow strings in state/type filters
* [#4048](https://github.com/icinga/icinga2/issues/4048): Cleanup downtimes created by ScheduleDowntime
* [#4046](https://github.com/icinga/icinga2/issues/4046) (Configuration): Config parser should not log names of included files by default
* [#3999](https://github.com/icinga/icinga2/issues/3999) (API): ApiListener: Make minimum TLS version configurable
* [#3997](https://github.com/icinga/icinga2/issues/3997) (API): ApiListener: Force server's preferred cipher
* [#3911](https://github.com/icinga/icinga2/issues/3911) (Graphite): Add acknowledgement type to Graphite, InfluxDB, OpenTSDB metadata
* [#3888](https://github.com/icinga/icinga2/issues/3888) (API): Implement SSL cipher configuration support for the API feature
* [#3763](https://github.com/icinga/icinga2/issues/3763): Add name attribute for WorkQueue class
* [#3562](https://github.com/icinga/icinga2/issues/3562) (Metrics): Add InfluxDbWriter feature
* [#3400](https://github.com/icinga/icinga2/issues/3400): Remove the deprecated IcingaStatusWriter feature
* [#3237](https://github.com/icinga/icinga2/issues/3237) (Metrics): Gelf module: expose 'perfdata' fields for 'CHECK\_RESULT' events
* [#3224](https://github.com/icinga/icinga2/issues/3224) (Configuration): Implement support for formatting date/time
* [#3178](https://github.com/icinga/icinga2/issues/3178) (DB IDO): Add SSL support for the IdoMysqlConnection feature
* [#2970](https://github.com/icinga/icinga2/issues/2970) (Metrics): Add timestamp support for GelfWriter
* [#2040](https://github.com/icinga/icinga2/issues/2040): Exclude option for TimePeriod definitions

### Bug

* [#4534](https://github.com/icinga/icinga2/issues/4534) (CLI): Icinga2 segault on startup
* [#4524](https://github.com/icinga/icinga2/issues/4524) (API): API Remote crash via Google Chrome
* [#4520](https://github.com/icinga/icinga2/issues/4520) (Configuration): Memory leak when using closures
* [#4512](https://github.com/icinga/icinga2/issues/4512) (Cluster): Incorrect certificate validation error message
* [#4511](https://github.com/icinga/icinga2/issues/4511): ClrCheck is null on \*nix
* [#4505](https://github.com/icinga/icinga2/issues/4505) (CLI): Cannot set ownership for user 'icinga' group 'icinga' on file '/var/lib/icinga2/ca/serial.txt'.
* [#4504](https://github.com/icinga/icinga2/issues/4504) (API): API: events for DowntimeTriggered does not provide needed information
* [#4502](https://github.com/icinga/icinga2/issues/4502) (DB IDO): IDO query fails due to key contraint violation for the icinga\_customvariablestatus table
* [#4501](https://github.com/icinga/icinga2/issues/4501) (Cluster): DB IDO started before daemonizing \(no systemd\)
* [#4500](https://github.com/icinga/icinga2/issues/4500) (DB IDO): Query for customvariablestatus incorrectly updates the host's/service's insert ID
* [#4499](https://github.com/icinga/icinga2/issues/4499) (DB IDO): Insert fails for the icinga\_scheduleddowntime table due to duplicate key
* [#4497](https://github.com/icinga/icinga2/issues/4497): Fix incorrect detection of the 'Concurrency' variable
* [#4496](https://github.com/icinga/icinga2/issues/4496) (API): API: action schedule-downtime requires a duration also when fixed is true
* [#4495](https://github.com/icinga/icinga2/issues/4495): Use hash-based serial numbers for new certificates
* [#4490](https://github.com/icinga/icinga2/issues/4490) (Cluster): ClusterEvents::NotificationSentAllUsersAPIHandler\(\) does not set notified\_users
* [#4488](https://github.com/icinga/icinga2/issues/4488): Replace GetType\(\)-\>GetName\(\) calls with GetReflectionType\(\)-\>GetName\(\)
* [#4484](https://github.com/icinga/icinga2/issues/4484) (Cluster): Only allow sending command\_endpoint checks to directly connected child zones
* [#4483](https://github.com/icinga/icinga2/issues/4483) (DB IDO): ido CheckCommand returns returns "Could not connect to database server" when HA enabled
* [#4481](https://github.com/icinga/icinga2/issues/4481) (DB IDO): Fix the "ido" check command for use with command\_endpoint
* [#4478](https://github.com/icinga/icinga2/issues/4478): CompatUtility::GetCheckableNotificationStateFilter is returning an incorrect value
* [#4476](https://github.com/icinga/icinga2/issues/4476) (DB IDO): Importing mysql schema fails
* [#4475](https://github.com/icinga/icinga2/issues/4475) (CLI): pki sign-csr does not log where it is writing the certificate file
* [#4472](https://github.com/icinga/icinga2/issues/4472) (DB IDO): IDO marks objects as inactive on shutdown
* [#4471](https://github.com/icinga/icinga2/issues/4471) (DB IDO): IDO does duplicate config updates
* [#4466](https://github.com/icinga/icinga2/issues/4466) (Configuration): 'use' keyword cannot be used with templates
* [#4462](https://github.com/icinga/icinga2/issues/4462) (Notifications): Add log message if notifications are forced \(i.e. filters are not checked\)
* [#4461](https://github.com/icinga/icinga2/issues/4461) (Notifications): Notification resent, even if interval = 0
* [#4460](https://github.com/icinga/icinga2/issues/4460) (DB IDO): Fixed downtime start does not update actual\_start\_time
* [#4458](https://github.com/icinga/icinga2/issues/4458): Flexible downtimes should be removed after trigger\_time+duration
* [#4455](https://github.com/icinga/icinga2/issues/4455): Disallow casting "" to an Object
* [#4447](https://github.com/icinga/icinga2/issues/4447): Handle I/O errors while writing the Icinga state file more gracefully
* [#4446](https://github.com/icinga/icinga2/issues/4446) (Notifications): Incorrect downtime notification events
* [#4444](https://github.com/icinga/icinga2/issues/4444): Fix building Icinga with -fvisibility=hidden
* [#4439](https://github.com/icinga/icinga2/issues/4439) (Configuration): Icinga doesn't delete temporary icinga2.debug file when config validation fails
* [#4434](https://github.com/icinga/icinga2/issues/4434) (Notifications): Notification sent too fast when one master fails
* [#4430](https://github.com/icinga/icinga2/issues/4430) (Cluster): Remove obsolete README files in tools/syntax
* [#4427](https://github.com/icinga/icinga2/issues/4427) (Notifications): Missing notification for recovery during downtime
* [#4425](https://github.com/icinga/icinga2/issues/4425) (DB IDO): Change the way outdated comments/downtimes are deleted on restart
* [#4420](https://github.com/icinga/icinga2/issues/4420) (Notifications): Multiple notifications when master fails
* [#4418](https://github.com/icinga/icinga2/issues/4418) (DB IDO): icinga2 IDO reload performance significant slower with latest snapshot release
* [#4417](https://github.com/icinga/icinga2/issues/4417) (Notifications): Notification interval mistimed
* [#4413](https://github.com/icinga/icinga2/issues/4413) (DB IDO): icinga2 empties custom variables, host-, servcie- and contactgroup members at the end of IDO database reconnection
* [#4412](https://github.com/icinga/icinga2/issues/4412) (Notifications): Reminder notifications ignore HA mode
* [#4405](https://github.com/icinga/icinga2/issues/4405) (DB IDO): Deprecation warning should include object type and name
* [#4401](https://github.com/icinga/icinga2/issues/4401) (Metrics): Incorrect escaping / formatting of perfdata to InfluxDB
* [#4399](https://github.com/icinga/icinga2/issues/4399): Icinga stats min\_execution\_time and max\_execution\_time are invalid
* [#4394](https://github.com/icinga/icinga2/issues/4394): icinga check reports "-1" for minimum latency and execution time and only uptime has a number but 0
* [#4391](https://github.com/icinga/icinga2/issues/4391) (DB IDO): Do not clear {host,service,contact}group\_members tables on restart
* [#4384](https://github.com/icinga/icinga2/issues/4384) (API): Fix URL encoding for '&'
* [#4380](https://github.com/icinga/icinga2/issues/4380) (Cluster): Increase cluster reconnect interval
* [#4378](https://github.com/icinga/icinga2/issues/4378) (Notifications): Optimize two ObjectLocks into one in Notification::BeginExecuteNotification method
* [#4376](https://github.com/icinga/icinga2/issues/4376) (Cluster): CheckerComponent sometimes fails to schedule checks in time
* [#4375](https://github.com/icinga/icinga2/issues/4375) (Cluster): Duplicate messages for command\_endpoint w/ master and satellite
* [#4372](https://github.com/icinga/icinga2/issues/4372) (API): state\_filters\_real shouldn't be visible in the API
* [#4371](https://github.com/icinga/icinga2/issues/4371) (Notifications): notification.notification\_number runtime attribute returning 0 \(instead of 1\) in first notification e-mail
* [#4370](https://github.com/icinga/icinga2/issues/4370): Test the change with HARD OK transitions
* [#4363](https://github.com/icinga/icinga2/issues/4363) (DB IDO): IDO module starts threads before daemonize
* [#4356](https://github.com/icinga/icinga2/issues/4356) (DB IDO): DB IDO query queue does not clean up with v2.4.10-520-g124c80b
* [#4349](https://github.com/icinga/icinga2/issues/4349) (DB IDO): Add missing index on state history for DB IDO cleanup
* [#4345](https://github.com/icinga/icinga2/issues/4345): Ensure to clear the SSL error queue before calling SSL\_{read,write,do\_handshake}
* [#4343](https://github.com/icinga/icinga2/issues/4343) (Configuration): include\_recursive should gracefully handle inaccessible files
* [#4341](https://github.com/icinga/icinga2/issues/4341) (API): Icinga incorrectly disconnects all endpoints if one has a wrong certificate
* [#4340](https://github.com/icinga/icinga2/issues/4340) (DB IDO): deadlock in ido reconnect
* [#4329](https://github.com/icinga/icinga2/issues/4329) (Metrics): Key Escapes in InfluxDB Writer Don't Work
* [#4313](https://github.com/icinga/icinga2/issues/4313) (Configuration): Icinga crashes when using include\_recursive in an object definition
* [#4309](https://github.com/icinga/icinga2/issues/4309) (Configuration): ConfigWriter::EmitScope incorrectly quotes dictionary keys
* [#4300](https://github.com/icinga/icinga2/issues/4300) (DB IDO): Comment/Downtime delete queries are slow
* [#4293](https://github.com/icinga/icinga2/issues/4293) (DB IDO): Overflow in current\_notification\_number column in DB IDO MySQL
* [#4287](https://github.com/icinga/icinga2/issues/4287) (DB IDO): Program status table is not updated in IDO after starting icinga
* [#4283](https://github.com/icinga/icinga2/issues/4283) (Cluster): Icinga 2 satellite crashes
* [#4278](https://github.com/icinga/icinga2/issues/4278) (DB IDO): SOFT state changes with the same state are not logged
* [#4275](https://github.com/icinga/icinga2/issues/4275) (API): Trying to delete an object protected by a permissions filter, ends up deleting all objects that match the filter instead
* [#4274](https://github.com/icinga/icinga2/issues/4274) (Notifications): Duplicate notifications
* [#4264](https://github.com/icinga/icinga2/issues/4264) (Metrics): InfluxWriter doesnt sanitize the data before sending
* [#4259](https://github.com/icinga/icinga2/issues/4259): Flapping Notifications dependent on state change
* [#4258](https://github.com/icinga/icinga2/issues/4258): last SOFT state should be hard \(max\_check\_attempts\)
* [#4257](https://github.com/icinga/icinga2/issues/4257) (Configuration): Incorrect custom variable name in the hosts.conf example config
* [#4255](https://github.com/icinga/icinga2/issues/4255) (Configuration): Config validation should not delete comments/downtimes w/o reference
* [#4244](https://github.com/icinga/icinga2/issues/4244): SOFT OK-state after returning from a soft state
* [#4239](https://github.com/icinga/icinga2/issues/4239) (Notifications): Downtime notifications do not pass author and comment
* [#4232](https://github.com/icinga/icinga2/issues/4232): Problems with check scheduling for HARD state changes \(standalone/command\_endpoint\)
* [#4231](https://github.com/icinga/icinga2/issues/4231) (DB IDO): Volatile check results for OK-\>OK transitions are logged into DB IDO statehistory
* [#4187](https://github.com/icinga/icinga2/issues/4187): Icinga 2 client gets killed during network scans
* [#4171](https://github.com/icinga/icinga2/issues/4171) (DB IDO): Outdated downtime/comments not removed from IDO database \(restart\)
* [#4134](https://github.com/icinga/icinga2/issues/4134) (Configuration): Don't allow flow control keywords outside of other flow control constructs
* [#4121](https://github.com/icinga/icinga2/issues/4121) (Notifications): notification interval = 0 not honoured in HA clusters
* [#4106](https://github.com/icinga/icinga2/issues/4106) (Notifications): last\_problem\_notification should be synced in HA cluster
* [#4077](https://github.com/icinga/icinga2/issues/4077): Numbers are not properly formatted in runtime macro strings
* [#4002](https://github.com/icinga/icinga2/issues/4002): Don't violate POSIX by ensuring that the argument to usleep\(3\) is less than 1000000 
* [#3954](https://github.com/icinga/icinga2/issues/3954) (Cluster): High load when pinning command endpoint on HA cluster
* [#3949](https://github.com/icinga/icinga2/issues/3949) (DB IDO): IDO: entry\_time of all comments is set to the date and time when Icinga 2 was restarted
* [#3902](https://github.com/icinga/icinga2/issues/3902): Hang in TlsStream::Handshake
* [#3820](https://github.com/icinga/icinga2/issues/3820) (Configuration): High CPU usage with self-referenced parent zone config
* [#3805](https://github.com/icinga/icinga2/issues/3805) (Metrics): GELF multi-line output
* [#3627](https://github.com/icinga/icinga2/issues/3627) (API): /v1 returns HTML even if JSON is requested
* [#3486](https://github.com/icinga/icinga2/issues/3486) (Notifications): Notification times w/ empty begin/end specifications prevent sending notifications
* [#3370](https://github.com/icinga/icinga2/issues/3370): Race condition in CreatePipeOverlapped
* [#3365](https://github.com/icinga/icinga2/issues/3365) (DB IDO): IDO: there is no usable object index on icinga\_{scheduleddowntime,comments}
* [#3364](https://github.com/icinga/icinga2/issues/3364) (DB IDO): IDO: check\_source should not be a TEXT field
* [#3361](https://github.com/icinga/icinga2/issues/3361) (DB IDO): Missing indexes for icinga\_endpoints\* and icinga\_zones\* tables in DB IDO schema
* [#3355](https://github.com/icinga/icinga2/issues/3355) (DB IDO): IDO: icinga\_host/service\_groups alias columns are TEXT columns
* [#3229](https://github.com/icinga/icinga2/issues/3229): Function::Invoke should optionally register ScriptFrame
* [#2996](https://github.com/icinga/icinga2/issues/2996) (Cluster): Custom notification external commands do not work in a master-master setup
* [#2039](https://github.com/icinga/icinga2/issues/2039): Disable immediate hard state after first checkresult

### ITL

* [#4518](https://github.com/icinga/icinga2/issues/4518) (ITL): ITL uses unsupported arguments for check\_swap on Debian wheezy/Ubuntu trusty
* [#4506](https://github.com/icinga/icinga2/issues/4506) (ITL): Add interfacetable CheckCommand options --trafficwithpkt and --snmp-maxmsgsize
* [#4477](https://github.com/icinga/icinga2/issues/4477) (ITL): Add perfsyntax parameter to nscp-local-counter CheckCommand
* [#4456](https://github.com/icinga/icinga2/issues/4456) (ITL): Add custom variables for all check\_swap arguments
* [#4437](https://github.com/icinga/icinga2/issues/4437) (ITL): Add command definition for check\_mysql\_query
* [#4421](https://github.com/icinga/icinga2/issues/4421) (ITL): -q option for check\_ntp\_time is wrong
* [#4416](https://github.com/icinga/icinga2/issues/4416) (ITL): Add check command definition for check\_graphite
* [#4397](https://github.com/icinga/icinga2/issues/4397) (ITL): A lot of missing parameters for \(latest\) mysql\_health
* [#4379](https://github.com/icinga/icinga2/issues/4379) (ITL): Add support for "-A" command line switch to CheckCommand "snmp-process" 
* [#4359](https://github.com/icinga/icinga2/issues/4359) (ITL): ITL: check\_iftraffic64.pl default values, wrong postfix value in CheckCommand
* [#4332](https://github.com/icinga/icinga2/issues/4332) (ITL): Add check command definition for db2\_health
* [#4305](https://github.com/icinga/icinga2/issues/4305) (ITL): Add check command definitions for kdc and rbl
* [#4297](https://github.com/icinga/icinga2/issues/4297) (ITL): add check command for plugin check\_apache\_status
* [#4276](https://github.com/icinga/icinga2/issues/4276) (ITL): Adding option to access ifName for manubulon snmp-interface check command
* [#4254](https://github.com/icinga/icinga2/issues/4254) (ITL): Add "fuse.gvfsd-fuse" to the list of excluded file systems for check\_disk
* [#4250](https://github.com/icinga/icinga2/issues/4250) (ITL): Add CIM port parameter for esxi\_hardware CheckCommand
* [#4023](https://github.com/icinga/icinga2/issues/4023) (ITL): Add "retries" option to check\_snmp command
* [#3711](https://github.com/icinga/icinga2/issues/3711) (ITL): icinga2.conf: Include plugins-contrib, manubulon, windows-plugins, nscp by default
* [#3683](https://github.com/icinga/icinga2/issues/3683) (ITL): Add IPv4/IPv6 support to the rest of the monitoring-plugins
* [#3012](https://github.com/icinga/icinga2/issues/3012) (ITL): Extend CheckCommand definitions for nscp-local

### Documentation

* [#4521](https://github.com/icinga/icinga2/issues/4521) (Documentation): Typo in Notification object documentation
* [#4517](https://github.com/icinga/icinga2/issues/4517) (Documentation): Documentation is missing for the API permissions that are new in 2.5.0
* [#4513](https://github.com/icinga/icinga2/issues/4513) (Documentation): Development docs: Add own section for gdb backtrace from a running process
* [#4510](https://github.com/icinga/icinga2/issues/4510) (Documentation): Docs: API example uses wrong attribute name
* [#4489](https://github.com/icinga/icinga2/issues/4489) (Documentation): Missing documentation for "legacy-timeperiod" template
* [#4470](https://github.com/icinga/icinga2/issues/4470) (Documentation): The description for the http\_certificate attribute doesn't have the right default value
* [#4468](https://github.com/icinga/icinga2/issues/4468) (Documentation): Add URL and short description for Monitoring Plugins inside the ITL documentation
* [#4453](https://github.com/icinga/icinga2/issues/4453) (Documentation): Rewrite Client and Cluster chapter and; add service monitoring chapter
* [#4419](https://github.com/icinga/icinga2/issues/4419) (Documentation): Incorrect API permission name for /v1/status in the documentation
* [#4396](https://github.com/icinga/icinga2/issues/4396) (Documentation): Missing explanation for three level clusters with CSR auto-signing
* [#4395](https://github.com/icinga/icinga2/issues/4395) (Documentation): Incorrect documentation about apply rules in zones.d directories
* [#4387](https://github.com/icinga/icinga2/issues/4387) (Documentation): Improve author information about check\_yum
* [#4361](https://github.com/icinga/icinga2/issues/4361) (Documentation): pkg-config is not listed as a build requirement in INSTALL.md
* [#4337](https://github.com/icinga/icinga2/issues/4337) (Documentation): Add a note to the docs that API POST updates to custom attributes/groups won't trigger re-evaluation
* [#4333](https://github.com/icinga/icinga2/issues/4333) (Documentation): Documentation: Setting up Plugins section is broken
* [#4328](https://github.com/icinga/icinga2/issues/4328) (Documentation): Typo in Manubulon CheckCommand documentation
* [#4318](https://github.com/icinga/icinga2/issues/4318) (Documentation): Migration docs still show unsupported CHANGE\_\*MODATTR external commands
* [#4306](https://github.com/icinga/icinga2/issues/4306) (Documentation): Add a note about creating Zone/Endpoint objects with the API
* [#4299](https://github.com/icinga/icinga2/issues/4299) (Documentation): Incorrect URL for API examples in the documentation
* [#4265](https://github.com/icinga/icinga2/issues/4265) (Documentation): Improve "Endpoint" documentation
* [#4263](https://github.com/icinga/icinga2/issues/4263) (Documentation): Fix systemd client command formatting
* [#4238](https://github.com/icinga/icinga2/issues/4238) (Documentation): Missing quotes for API action URL
* [#4236](https://github.com/icinga/icinga2/issues/4236) (Documentation): Use HTTPS for debmon.org links in the documentation
* [#4217](https://github.com/icinga/icinga2/issues/4217) (Documentation): node setup: Add a note for --endpoint syntax for client-master connection
* [#4124](https://github.com/icinga/icinga2/issues/4124) (Documentation): Documentation review
* [#3612](https://github.com/icinga/icinga2/issues/3612) (Documentation): Update SELinux documentation

### Support

* [#4526](https://github.com/icinga/icinga2/issues/4526) (Packages): Revert dependency on firewalld on RHEL
* [#4494](https://github.com/icinga/icinga2/issues/4494) (Installation): Remove unused functions from icinga-installer
* [#4452](https://github.com/icinga/icinga2/issues/4452) (Packages): Error compiling on windows due to changes in apilistener around minimum tls version
* [#4432](https://github.com/icinga/icinga2/issues/4432) (Packages): Windows build broken since ref 11292
* [#4404](https://github.com/icinga/icinga2/issues/4404) (Installation): Increase default systemd timeout
* [#4344](https://github.com/icinga/icinga2/issues/4344) (Packages): Build fails with Visual Studio 2013
* [#4327](https://github.com/icinga/icinga2/issues/4327) (Packages): Icinga fails to build with OpenSSL 1.1.0
* [#4251](https://github.com/icinga/icinga2/issues/4251) (Tests): Add debugging mode for Utility::GetTime
* [#4234](https://github.com/icinga/icinga2/issues/4234) (Tests): Boost tests are missing a dependency on libmethods
* [#4230](https://github.com/icinga/icinga2/issues/4230) (Installation): Windows: Error with repository handler \(missing /var/lib/icinga2/api/repository path\)
* [#4211](https://github.com/icinga/icinga2/issues/4211) (Packages): Incorrect filter in pick.py
* [#4190](https://github.com/icinga/icinga2/issues/4190) (Packages): Windows Installer: Remove dependency on KB2999226 package
* [#4148](https://github.com/icinga/icinga2/issues/4148) (Packages): RPM update starts disabled icinga2 service
* [#4147](https://github.com/icinga/icinga2/issues/4147) (Packages): Reload permission error with SELinux
* [#4135](https://github.com/icinga/icinga2/issues/4135) (Installation): Add script for automatically cherry-picking commits for minor versions
* [#3829](https://github.com/icinga/icinga2/issues/3829) (Packages): Provide packages for icinga-studio on Fedora
* [#3708](https://github.com/icinga/icinga2/issues/3708) (Packages): Firewalld Service definition for Icinga
* [#2606](https://github.com/icinga/icinga2/issues/2606) (Packages): Package for syntax highlighting

## 2.4.9 (2016-05-19)

### Notes

This release fixes a number of issues introduced in 2.4.8.

### Bug

* [#4225](https://github.com/icinga/icinga2/issues/4225) (Compat): Command Pipe thread 100% CPU Usage
* [#4224](https://github.com/icinga/icinga2/issues/4224): Checks are not executed anymore on command
* [#4222](https://github.com/icinga/icinga2/issues/4222) (Configuration): Segfault when trying to start 2.4.8
* [#4221](https://github.com/icinga/icinga2/issues/4221) (Metrics): Error: Function call 'rename' for file '/var/spool/icinga2/tmp/service-perfdata' failed with error code 2, 'No such file or directory'

## 2.4.10 (2016-05-19)

### Notes

* Bugfixes

### Bug

* [#4227](https://github.com/icinga/icinga2/issues/4227): Checker component doesn't execute any checks for command\_endpoint

## 2.4.8 (2016-05-17)

### Notes

* Bugfixes
* Support for limiting the maximum number of concurrent checks (new configuration option)
* HA-aware features now wait for connected cluster nodes in the same zone (e.g. DB IDO)
* The 'icinga' check now alerts on failed reloads

### Enhancement

* [#4203](https://github.com/icinga/icinga2/issues/4203) (Cluster): Only activate HARunOnce objects once there's a cluster connection
* [#4198](https://github.com/icinga/icinga2/issues/4198): Move CalculateExecutionTime and CalculateLatency into the CheckResult class
* [#4196](https://github.com/icinga/icinga2/issues/4196) (Cluster): Remove unused cluster commands
* [#4149](https://github.com/icinga/icinga2/issues/4149) (CLI): Implement SNI support for the CLI commands
* [#4103](https://github.com/icinga/icinga2/issues/4103): Add support for subjectAltName in SSL certificates
* [#3919](https://github.com/icinga/icinga2/issues/3919) (Configuration): Internal check for config problems
* [#3321](https://github.com/icinga/icinga2/issues/3321): "icinga" check should have state WARNING when the last reload failed
* [#2993](https://github.com/icinga/icinga2/issues/2993) (Metrics): PerfdataWriter: Better failure handling for file renames across file systems
* [#2896](https://github.com/icinga/icinga2/issues/2896) (Cluster): Alert config reload failures with the icinga check 
* [#2468](https://github.com/icinga/icinga2/issues/2468): Maximum concurrent service checks

### Bug

* [#4219](https://github.com/icinga/icinga2/issues/4219) (DB IDO): Postgresql warnings on startup
* [#4212](https://github.com/icinga/icinga2/issues/4212): assertion failed: GetResumeCalled\(\)
* [#4210](https://github.com/icinga/icinga2/issues/4210) (API): Incorrect variable names for joined fields in filters
* [#4204](https://github.com/icinga/icinga2/issues/4204) (DB IDO): Ensure that program status updates are immediately updated in DB IDO
* [#4202](https://github.com/icinga/icinga2/issues/4202) (API): API: Missing error handling for invalid JSON request body
* [#4182](https://github.com/icinga/icinga2/issues/4182): Crash in UnameHelper
* [#4180](https://github.com/icinga/icinga2/issues/4180): Expired downtimes are not removed
* [#4170](https://github.com/icinga/icinga2/issues/4170) (API): Icinga Crash with the workflow Create\_Host-\> Downtime for the Host -\>  Delete Downtime -\> Remove Host
* [#4145](https://github.com/icinga/icinga2/issues/4145) (Configuration): Wrong log severity causes segfault
* [#4120](https://github.com/icinga/icinga2/issues/4120): notification sent out during flexible downtime
* [#4038](https://github.com/icinga/icinga2/issues/4038) (API): inconsistent API /v1/objects/\* response for PUT requests
* [#4037](https://github.com/icinga/icinga2/issues/4037) (Compat): Command pipe overloaded: Can't send external Icinga command to the local command file
* [#4029](https://github.com/icinga/icinga2/issues/4029) (API): Icinga2 API: deleting service with cascade=1 does not delete dependant notification
* [#3938](https://github.com/icinga/icinga2/issues/3938): Crash with empty ScheduledDowntime 'ranges' attribute
* [#3932](https://github.com/icinga/icinga2/issues/3932): "day -X" time specifications are parsed incorrectly
* [#3912](https://github.com/icinga/icinga2/issues/3912) (Compat): Empty author/text attribute for comment/downtimes external commands causing crash
* [#3881](https://github.com/icinga/icinga2/issues/3881) (Cluster): Icinga2 agent gets stuck after disconnect and won't relay messages
* [#3707](https://github.com/icinga/icinga2/issues/3707) (Configuration): Comments and downtimes of deleted checkable objects are not deleted
* [#3526](https://github.com/icinga/icinga2/issues/3526): Icinga crashes with a segfault on receiving a lot of check results for nonexisting hosts/services
* [#3316](https://github.com/icinga/icinga2/issues/3316) (Configuration): Service apply without name possible

### ITL

* [#4184](https://github.com/icinga/icinga2/issues/4184) (ITL): 'disk' CheckCommand: Exclude 'cgroup' and 'tracefs' by default
* [#3634](https://github.com/icinga/icinga2/issues/3634) (ITL): Provide icingacli in the ITL

### Documentation

* [#4205](https://github.com/icinga/icinga2/issues/4205) (Documentation): Add the category to the generated changelog
* [#4193](https://github.com/icinga/icinga2/issues/4193) (Documentation): Missing documentation for event commands w/ execution bridge
* [#4144](https://github.com/icinga/icinga2/issues/4144) (Documentation): Incorrect chapter headings for Object\#to\_string and Object\#type

### Support

* [#4146](https://github.com/icinga/icinga2/issues/4146) (Packages): Update chocolatey packages and RELEASE.md

## 2.4.7 (2016-04-21)

### Notes

* Bugfixes

### Bug

* [#4142](https://github.com/icinga/icinga2/issues/4142) (DB IDO): Crash in IdoMysqlConnection::ExecuteMultipleQueries

## 2.4.6 (2016-04-20)

### Notes

* Bugfixes

### Bug

* [#4140](https://github.com/icinga/icinga2/issues/4140) (DB IDO): Failed assertion in IdoPgsqlConnection::FieldToEscapedString

### Documentation

* [#4141](https://github.com/icinga/icinga2/issues/4141) (Documentation): Update RELEASE.md
* [#4136](https://github.com/icinga/icinga2/issues/4136) (Documentation): Docs: Zone attribute 'endpoints' is an array

### Support

* [#4139](https://github.com/icinga/icinga2/issues/4139) (Packages): Icinga 2 fails to build on Ubuntu Xenial

## 2.4.5 (2016-04-20)

### Notes

* Windows Installer changed from NSIS to MSI
* New configuration attribute for hosts and services: check_timeout (overrides the CheckCommand's timeout when set)
* ITL updates
* Lots of bugfixes

### Enhancement

* [#3023](https://github.com/icinga/icinga2/issues/3023) (Configuration): Implement support for overriding check command timeout

### Bug

* [#4131](https://github.com/icinga/icinga2/issues/4131) (Configuration): Vim Syntax Highlighting does not work with assign where
* [#4116](https://github.com/icinga/icinga2/issues/4116) (API): icinga2 crashes when a command\_endpoint is set, but the api feature is not active
* [#4114](https://github.com/icinga/icinga2/issues/4114): Compiler warning in NotifyActive
* [#4109](https://github.com/icinga/icinga2/issues/4109) (API): Navigation attributes are missing in /v1/objects/\<type\>
* [#4104](https://github.com/icinga/icinga2/issues/4104) (Configuration): Segfault during config validation if host exists, service does not exist any longer and downtime expires
* [#4095](https://github.com/icinga/icinga2/issues/4095): DowntimesExpireTimerHandler crashes Icinga2 with \<unknown function\>
* [#4089](https://github.com/icinga/icinga2/issues/4089): Make the socket event engine configurable
* [#4078](https://github.com/icinga/icinga2/issues/4078) (Configuration): Overwriting global type variables causes crash in ConfigItem::Commit\(\)
* [#4076](https://github.com/icinga/icinga2/issues/4076) (API): API User gets wrongly authenticated \(client\_cn and no password\)
* [#4066](https://github.com/icinga/icinga2/issues/4066): ConfigSync broken from 2.4.3. to 2.4.4 under Windows
* [#4056](https://github.com/icinga/icinga2/issues/4056) (CLI): Remove semi-colons in the auto-generated configs
* [#4052](https://github.com/icinga/icinga2/issues/4052) (API): Config validation for Notification objects should check whether the state filters are valid
* [#4035](https://github.com/icinga/icinga2/issues/4035) (DB IDO): IDO: historical contact notifications table column notification\_id is off-by-one
* [#4031](https://github.com/icinga/icinga2/issues/4031): Downtimes are not always activated/expired on restart
* [#4016](https://github.com/icinga/icinga2/issues/4016): Symlink subfolders not followed/considered for config files
* [#4014](https://github.com/icinga/icinga2/issues/4014): Use retry\_interval instead of check\_interval for first OK -\> NOT-OK state change
* [#3973](https://github.com/icinga/icinga2/issues/3973) (Cluster): Downtimes and Comments are not synced to child zones
* [#3970](https://github.com/icinga/icinga2/issues/3970) (API): Socket Exceptions \(Operation not permitted\) while reading from API
* [#3907](https://github.com/icinga/icinga2/issues/3907) (Configuration): Too many assign where filters cause stack overflow
* [#3780](https://github.com/icinga/icinga2/issues/3780) (DB IDO): DB IDO: downtime is not in effect after restart

### ITL

* [#3953](https://github.com/icinga/icinga2/issues/3953) (ITL): Add --units, --rate and --rate-multiplier support for the snmpv3 check command
* [#3903](https://github.com/icinga/icinga2/issues/3903) (ITL): Add --method parameter for check\_{oracle,mysql,mssql}\_health CheckCommands

### Documentation

* [#4122](https://github.com/icinga/icinga2/issues/4122) (Documentation): Remove instance\_name from Ido\*Connection example
* [#4108](https://github.com/icinga/icinga2/issues/4108) (Documentation): Incorrect link in the documentation
* [#4080](https://github.com/icinga/icinga2/issues/4080) (Documentation): Update documentation URL for Icinga Web 2
* [#4058](https://github.com/icinga/icinga2/issues/4058) (Documentation): Docs: Cluster manual SSL generation formatting is broken
* [#4057](https://github.com/icinga/icinga2/issues/4057) (Documentation): Update the CentOS installation documentation
* [#4055](https://github.com/icinga/icinga2/issues/4055) (Documentation): Add silent install / reference to NSClient++ to documentation
* [#4043](https://github.com/icinga/icinga2/issues/4043) (Documentation): Docs: Remove the migration script chapter
* [#4041](https://github.com/icinga/icinga2/issues/4041) (Documentation): Explain how to use functions for wildcard matches for arrays and/or dictionaries in assign where expressions
* [#4039](https://github.com/icinga/icinga2/issues/4039) (Documentation): Update .mailmap for Markus Frosch
* [#3145](https://github.com/icinga/icinga2/issues/3145) (Documentation): Add Windows setup wizard screenshots

### Support

* [#4127](https://github.com/icinga/icinga2/issues/4127) (Installation): Windows installer does not copy "features-enabled" on upgrade
* [#4119](https://github.com/icinga/icinga2/issues/4119) (Installation): Update chocolatey uninstall script for the MSI package
* [#4118](https://github.com/icinga/icinga2/issues/4118) (Installation): icinga2-installer.exe doesn't wait until NSIS uninstall.exe exits
* [#4117](https://github.com/icinga/icinga2/issues/4117) (Installation): Make sure to update the agent wizard banner
* [#4113](https://github.com/icinga/icinga2/issues/4113) (Installation): Package fails to build on \*NIX
* [#4099](https://github.com/icinga/icinga2/issues/4099) (Installation): make install overwrites configuration files
* [#4074](https://github.com/icinga/icinga2/issues/4074) (Installation): FatalError\(\) returns when called before Application.Run
* [#4073](https://github.com/icinga/icinga2/issues/4073) (Installation): Install 64-bit version of NSClient++ on 64-bit versions of Windows
* [#4072](https://github.com/icinga/icinga2/issues/4072) (Installation): Update NSClient++ to version 0.4.4.19
* [#4069](https://github.com/icinga/icinga2/issues/4069) (Installation): Error compiling icinga2 targeted for x64 on Windows
* [#4064](https://github.com/icinga/icinga2/issues/4064) (Packages): Build 64-bit packages for Windows
* [#4053](https://github.com/icinga/icinga2/issues/4053) (Installation): Icinga 2 Windows Agent does not honor install path during upgrade
* [#4032](https://github.com/icinga/icinga2/issues/4032) (Packages): Remove dependency for .NET 3.5 from the chocolatey package
* [#3988](https://github.com/icinga/icinga2/issues/3988) (Packages): Incorrect base URL in the icinga-rpm-release packages for Fedora
* [#3658](https://github.com/icinga/icinga2/issues/3658) (Packages): Add application manifest for the Windows agent wizard
* [#2998](https://github.com/icinga/icinga2/issues/2998) (Installation): logrotate fails since the "su" directive was removed

## 2.4.4 (2016-03-16)

### Notes

* Bugfixes

### Bug

* [#4036](https://github.com/icinga/icinga2/issues/4036) (CLI): Add the executed cli command to the Windows wizard error messages
* [#4019](https://github.com/icinga/icinga2/issues/4019) (Configuration): Segmentation fault during 'icinga2 daemon -C'
* [#4017](https://github.com/icinga/icinga2/issues/4017) (CLI): 'icinga2 feature list' fails when all features are disabled
* [#4008](https://github.com/icinga/icinga2/issues/4008) (Configuration): Windows wizard error "too many arguments"
* [#4006](https://github.com/icinga/icinga2/issues/4006): Volatile transitions from HARD NOT-OK-\>NOT-OK do not trigger notifications
* [#3996](https://github.com/icinga/icinga2/issues/3996): epoll\_ctl might cause oops on Ubuntu trusty
* [#3990](https://github.com/icinga/icinga2/issues/3990): Services status updated multiple times within check\_interval even though no retry was triggered
* [#3987](https://github.com/icinga/icinga2/issues/3987): Incorrect check interval when passive check results are used
* [#3985](https://github.com/icinga/icinga2/issues/3985): Active checks are executed even though passive results are submitted
* [#3981](https://github.com/icinga/icinga2/issues/3981): DEL\_DOWNTIME\_BY\_HOST\_NAME does not accept optional arguments
* [#3961](https://github.com/icinga/icinga2/issues/3961) (CLI): Wrong log message for trusted cert in node setup command
* [#3939](https://github.com/icinga/icinga2/issues/3939) (CLI): Common name in node wizard isn't case sensitive
* [#3745](https://github.com/icinga/icinga2/issues/3745) (API): Status code 200 even if an object could not be deleted.
* [#3742](https://github.com/icinga/icinga2/issues/3742) (DB IDO): DB IDO: User notification type filters are incorrect
* [#3442](https://github.com/icinga/icinga2/issues/3442) (API): MkDirP not working on Windows
* [#3439](https://github.com/icinga/icinga2/issues/3439) (Notifications): Host notification type is PROBLEM but should be RECOVERY
* [#3303](https://github.com/icinga/icinga2/issues/3303) (Notifications): Problem notifications while Flapping is active
* [#3153](https://github.com/icinga/icinga2/issues/3153) (Notifications): Flapping notifications are sent for hosts/services which are in a downtime

### ITL

* [#3958](https://github.com/icinga/icinga2/issues/3958) (ITL): Add "query" option to check\_postgres command.
* [#3908](https://github.com/icinga/icinga2/issues/3908) (ITL): ITL: Missing documentation for nwc\_health "mode" parameter
* [#3484](https://github.com/icinga/icinga2/issues/3484) (ITL): ITL: Allow to enforce specific SSL versions using the http check command

### Documentation

* [#4033](https://github.com/icinga/icinga2/issues/4033) (Documentation): Update development docs to use 'thread apply all bt full'
* [#4018](https://github.com/icinga/icinga2/issues/4018) (Documentation): Docs: Add API examples for creating services and check commands
* [#4009](https://github.com/icinga/icinga2/issues/4009) (Documentation): Typo in API docs
* [#3845](https://github.com/icinga/icinga2/issues/3845) (Documentation): Explain how to join hosts/services for /v1/objects/comments
* [#3755](https://github.com/icinga/icinga2/issues/3755) (Documentation): http check's URI is really just Path

### Support

* [#4027](https://github.com/icinga/icinga2/issues/4027) (Packages): Chocolatey package is missing uninstall function
* [#4011](https://github.com/icinga/icinga2/issues/4011) (Packages): Update build requirements for SLES 11 SP4
* [#3960](https://github.com/icinga/icinga2/issues/3960) (Installation): CMake does not find MySQL libraries on Windows

## 2.4.3 (2016-02-24)

### Notes

* Bugfixes

### Bug

* [#3963](https://github.com/icinga/icinga2/issues/3963): Wrong permissions for files in /var/cache/icinga2/\* 
* [#3962](https://github.com/icinga/icinga2/issues/3962) (Configuration): Permission problem after running icinga2 node wizard

## 2.4.2 (2016-02-23)

### Notes

* ITL
    Additional arguments for check_disk
    Fix incorrect path for the check_hpasm plugin
    New command: check_iostat
    Fix incorrect variable names for the check_impi plugin
* Cluster
    Improve cluster performance
    Fix connection handling problems (multiple connections for the same endpoint)
* Performance improvements for the DB IDO modules
* Lots and lots of various other bugfixes
* Documentation updates

### Enhancement

* [#3878](https://github.com/icinga/icinga2/issues/3878) (Configuration): Add String\#trim
* [#3857](https://github.com/icinga/icinga2/issues/3857) (Cluster): Support TLSv1.1 and TLSv1.2 for the cluster transport encryption
* [#3810](https://github.com/icinga/icinga2/issues/3810) (Plugins): Add Timeout parameter to snmpv3 check
* [#3785](https://github.com/icinga/icinga2/issues/3785) (DB IDO): Log DB IDO query queue stats
* [#3784](https://github.com/icinga/icinga2/issues/3784) (DB IDO): DB IDO: Add a log message when the connection handling is completed
* [#3760](https://github.com/icinga/icinga2/issues/3760) (Configuration): Raise a config error for "Checkable" objects in global zones
* [#3754](https://github.com/icinga/icinga2/issues/3754) (Plugins): Add "-x" parameter in command definition for disk-windows CheckCommand

### Bug

* [#3957](https://github.com/icinga/icinga2/issues/3957) (CLI): "node setup" tries to chown\(\) files before they're created
* [#3947](https://github.com/icinga/icinga2/issues/3947): CentOS 5 doesn't support epoll\_create1
* [#3922](https://github.com/icinga/icinga2/issues/3922) (Configuration): YYYY-MM-DD time specs are parsed incorrectly
* [#3915](https://github.com/icinga/icinga2/issues/3915) (API): Connections are not cleaned up properly
* [#3913](https://github.com/icinga/icinga2/issues/3913) (Cluster): Cluster WQ thread dies after fork\(\)
* [#3910](https://github.com/icinga/icinga2/issues/3910): Clean up unused variables a bit
* [#3905](https://github.com/icinga/icinga2/issues/3905) (DB IDO): Problem with hostgroup\_members table cleanup
* [#3898](https://github.com/icinga/icinga2/issues/3898) (API): API queries on non-existant objects cause exception
* [#3897](https://github.com/icinga/icinga2/issues/3897) (Configuration): Crash in ConfigItem::RunWithActivationContext
* [#3896](https://github.com/icinga/icinga2/issues/3896) (Cluster): Ensure that config sync updates are always sent on reconnect
* [#3889](https://github.com/icinga/icinga2/issues/3889) (DB IDO): Deleting an object via API does not disable it in DB IDO
* [#3871](https://github.com/icinga/icinga2/issues/3871) (Cluster): Master reloads with agents generate false alarms
* [#3870](https://github.com/icinga/icinga2/issues/3870) (DB IDO): next\_check noise in the IDO
* [#3866](https://github.com/icinga/icinga2/issues/3866) (Cluster): Check event duplication with parallel connections involved
* [#3863](https://github.com/icinga/icinga2/issues/3863) (Cluster): Segfault in ApiListener::ConfigUpdateObjectAPIHandler
* [#3859](https://github.com/icinga/icinga2/issues/3859): Stream buffer size is 512 bytes, could be raised
* [#3858](https://github.com/icinga/icinga2/issues/3858) (CLI): Escaped sequences not properly generated with 'node update-config'
* [#3848](https://github.com/icinga/icinga2/issues/3848) (Configuration): Mistake in mongodb command definition \(mongodb\_replicaset\)
* [#3843](https://github.com/icinga/icinga2/issues/3843): Modified attributes do not work for the IcingaApplication object w/ external commands
* [#3835](https://github.com/icinga/icinga2/issues/3835) (Cluster): high load and memory consumption on icinga2 agent v2.4.1
* [#3827](https://github.com/icinga/icinga2/issues/3827) (Configuration): Icinga state file corruption with temporary file creation
* [#3817](https://github.com/icinga/icinga2/issues/3817) (Cluster): Cluster config sync: Ensure that /var/lib/icinga2/api/zones/\* exists
* [#3816](https://github.com/icinga/icinga2/issues/3816) (Cluster): Exception stack trace on icinga2 client when the master reloads the configuration
* [#3812](https://github.com/icinga/icinga2/issues/3812) (API): API actions: Decide whether fixed: false is the right default
* [#3798](https://github.com/icinga/icinga2/issues/3798) (DB IDO): is\_active in IDO is only re-enabled on "every second" restart
* [#3797](https://github.com/icinga/icinga2/issues/3797): Remove superfluous \#ifdef
* [#3794](https://github.com/icinga/icinga2/issues/3794) (DB IDO): Icinga2 crashes in IDO when removing a comment
* [#3787](https://github.com/icinga/icinga2/issues/3787) (CLI): "repository add" cli command writes invalid "type" attribute
* [#3786](https://github.com/icinga/icinga2/issues/3786) (DB IDO): Evaluate if CanExecuteQuery/FieldToEscapedString lead to exceptions on !m\_Connected
* [#3783](https://github.com/icinga/icinga2/issues/3783) (DB IDO): Implement support for re-ordering groups of IDO queries
* [#3775](https://github.com/icinga/icinga2/issues/3775) (Configuration): Config validation doesn't fail when templates are used as object names
* [#3774](https://github.com/icinga/icinga2/issues/3774) (DB IDO): IDO breaks when writing to icinga\_programstatus with latest snapshots
* [#3773](https://github.com/icinga/icinga2/issues/3773) (Configuration): Relative path in include\_zones does not work
* [#3766](https://github.com/icinga/icinga2/issues/3766) (API): Cluster config sync ignores zones.d from API packages
* [#3765](https://github.com/icinga/icinga2/issues/3765): Use NodeName in null and random checks
* [#3764](https://github.com/icinga/icinga2/issues/3764) (DB IDO): Failed IDO query for icinga\_downtimehistory
* [#3752](https://github.com/icinga/icinga2/issues/3752): Incorrect information in --version on Linux
* [#3741](https://github.com/icinga/icinga2/issues/3741) (DB IDO): Avoid duplicate config and status updates on startup
* [#3735](https://github.com/icinga/icinga2/issues/3735) (Configuration): Disallow lambda expressions where side-effect-free expressions are not allowed
* [#3730](https://github.com/icinga/icinga2/issues/3730): Missing path in mkdir\(\) exceptions
* [#3728](https://github.com/icinga/icinga2/issues/3728) (DB IDO): build of icinga2 with gcc 4.4.7 segfaulting with ido
* [#3722](https://github.com/icinga/icinga2/issues/3722) (API): Missing num\_hosts\_pending in /v1/status/CIB
* [#3715](https://github.com/icinga/icinga2/issues/3715) (CLI): node wizard does not remember user defined port
* [#3712](https://github.com/icinga/icinga2/issues/3712) (CLI): Remove the local zone name question in node wizard
* [#3705](https://github.com/icinga/icinga2/issues/3705) (API): API is not working on wheezy
* [#3704](https://github.com/icinga/icinga2/issues/3704) (Cluster): ApiListener::ReplayLog can block with a lot of clients
* [#3702](https://github.com/icinga/icinga2/issues/3702) (Cluster): Zone::CanAccessObject is very expensive
* [#3697](https://github.com/icinga/icinga2/issues/3697) (Compat): Crash in ExternalCommandListener
* [#3677](https://github.com/icinga/icinga2/issues/3677) (API): API queries cause memory leaks 
* [#3613](https://github.com/icinga/icinga2/issues/3613) (DB IDO): Non-UTF8 characters from plugins causes IDO to fail
* [#3606](https://github.com/icinga/icinga2/issues/3606) (Plugins): check\_network performance data in invalid format
* [#3571](https://github.com/icinga/icinga2/issues/3571) (Plugins): check\_memory and check\_swap plugins do unit conversion and rounding before percentage calculations resulting in imprecise percentages
* [#3540](https://github.com/icinga/icinga2/issues/3540) (Livestatus): Livestatus log query - filter "class" yields empty results
* [#3440](https://github.com/icinga/icinga2/issues/3440): Icinga2 reload timeout results in killing old and new process because of systemd
* [#2866](https://github.com/icinga/icinga2/issues/2866) (DB IDO): DB IDO: notification\_id for contact notifications is out of range
* [#2746](https://github.com/icinga/icinga2/issues/2746) (DB IDO): Add priority queue for disconnect/programstatus update events 
* [#2009](https://github.com/icinga/icinga2/issues/2009): Re-checks scheduling w/ retry\_interval

### ITL

* [#3927](https://github.com/icinga/icinga2/issues/3927) (ITL): Checkcommand Disk : Option Freespace-ignore-reserved
* [#3749](https://github.com/icinga/icinga2/issues/3749) (ITL): The hpasm check command is using the PluginDir constant
* [#3747](https://github.com/icinga/icinga2/issues/3747) (ITL): Add check\_iostat to ITL
* [#3729](https://github.com/icinga/icinga2/issues/3729) (ITL): ITL check command possibly mistyped variable names

### Documentation

* [#3946](https://github.com/icinga/icinga2/issues/3946) (Documentation): Documentation: Unescaped pipe character in tables
* [#3893](https://github.com/icinga/icinga2/issues/3893) (Documentation): Outdated link to icingaweb2-module-nagvis
* [#3892](https://github.com/icinga/icinga2/issues/3892) (Documentation): Partially missing escaping in doc/7-icinga-template-library.md
* [#3861](https://github.com/icinga/icinga2/issues/3861) (Documentation): Incorrect IdoPgSqlConnection Example in Documentation
* [#3850](https://github.com/icinga/icinga2/issues/3850) (Documentation): Incorrect name in AUTHORS
* [#3836](https://github.com/icinga/icinga2/issues/3836) (Documentation): Troubleshooting: Explain how to fetch the executed command 
* [#3833](https://github.com/icinga/icinga2/issues/3833) (Documentation): Better explaination for array values in "disk" CheckCommand docs
* [#3826](https://github.com/icinga/icinga2/issues/3826) (Documentation): Add example how to use custom functions in attributes
* [#3808](https://github.com/icinga/icinga2/issues/3808) (Documentation): Typos in the "troubleshooting" section of the documentation
* [#3793](https://github.com/icinga/icinga2/issues/3793) (Documentation): "setting up check plugins" section should be enhanced with package manager examples
* [#3781](https://github.com/icinga/icinga2/issues/3781) (Documentation): Formatting problem in "Advanced Filter" chapter
* [#3770](https://github.com/icinga/icinga2/issues/3770) (Documentation): Missing documentation for API packages zones.d config sync 
* [#3759](https://github.com/icinga/icinga2/issues/3759) (Documentation): Missing SUSE repository for monitoring plugins documentation
* [#3748](https://github.com/icinga/icinga2/issues/3748) (Documentation): Wrong postgresql-setup initdb command for RHEL7
* [#3550](https://github.com/icinga/icinga2/issues/3550) (Documentation): A PgSQL DB for the IDO can't be created w/ UTF8
* [#3549](https://github.com/icinga/icinga2/issues/3549) (Documentation): Incorrect SQL command for creating the user of the PostgreSQL DB for the IDO

### Support

* [#3900](https://github.com/icinga/icinga2/issues/3900) (Packages): Windows build fails on InterlockedIncrement type
* [#3838](https://github.com/icinga/icinga2/issues/3838) (Installation): Race condition when using systemd unit file
* [#3832](https://github.com/icinga/icinga2/issues/3832) (Installation): Compiler warnings in lib/remote/base64.cpp
* [#3818](https://github.com/icinga/icinga2/issues/3818) (Installation): Logrotate on systemd distros should use systemctl not service
* [#3771](https://github.com/icinga/icinga2/issues/3771) (Installation): Build error with older CMake versions on VERSION\_LESS compare
* [#3769](https://github.com/icinga/icinga2/issues/3769) (Packages): Windows build fails with latest git master
* [#3746](https://github.com/icinga/icinga2/issues/3746) (Packages): chcon partial context error in safe-reload prevents reload 
* [#3723](https://github.com/icinga/icinga2/issues/3723) (Installation): Crash on startup with incorrect directory permissions
* [#3679](https://github.com/icinga/icinga2/issues/3679) (Installation): Add CMake flag for disabling the unit tests

## 2.4.1 (2015-11-26)

### Notes

* ITL
    * Add running_kernel_use_sudo option for the running_kernel check
* Configuration
    * Add global constants: `PlatformName`. `PlatformVersion`, `PlatformKernel` and `PlatformKernelVersion`
* CLI
    * Use NodeName and ZoneName constants for 'node setup' and 'node wizard'

### Enhancement

* [#3706](https://github.com/icinga/icinga2/issues/3706) (CLI): Use NodeName and ZoneName constants for 'node setup' and 'node wizard'

### Bug

* [#3710](https://github.com/icinga/icinga2/issues/3710) (CLI): Remove --master\_zone from --help because it is currently not implemented
* [#3689](https://github.com/icinga/icinga2/issues/3689) (CLI): CLI command 'repository add' doesn't work
* [#3685](https://github.com/icinga/icinga2/issues/3685) (CLI): node wizard checks for /var/lib/icinga2/ca directory but not the files
* [#3674](https://github.com/icinga/icinga2/issues/3674): lib/base/process.cpp SIGSEGV on Debian squeeze / RHEL 6
* [#3671](https://github.com/icinga/icinga2/issues/3671) (API): Icinga 2 crashes when ScheduledDowntime objects are used
* [#3670](https://github.com/icinga/icinga2/issues/3670) (CLI): API setup command incorrectly overwrites existing certificates
* [#3665](https://github.com/icinga/icinga2/issues/3665) (CLI): "node wizard" does not ask user to verify SSL certificate

### ITL

* [#3691](https://github.com/icinga/icinga2/issues/3691) (ITL): Add running\_kernel\_use\_sudo option for the running\_kernel check
* [#3682](https://github.com/icinga/icinga2/issues/3682) (ITL): Indentation in command-plugins.conf
* [#3657](https://github.com/icinga/icinga2/issues/3657) (ITL): Add by\_ssh\_options argument for the check\_by\_ssh plugin

### Documentation

* [#3701](https://github.com/icinga/icinga2/issues/3701) (Documentation): Incorrect path for icinga2 binary in development documentation
* [#3690](https://github.com/icinga/icinga2/issues/3690) (Documentation): Fix typos in the documentation
* [#3673](https://github.com/icinga/icinga2/issues/3673) (Documentation): Documentation for schedule-downtime is missing required paremeters
* [#3594](https://github.com/icinga/icinga2/issues/3594) (Documentation): Documentation example in "Access Object Attributes at Runtime" doesn't work correctly
* [#3391](https://github.com/icinga/icinga2/issues/3391) (Documentation): Incorrect web inject URL in documentation

### Support

* [#3699](https://github.com/icinga/icinga2/issues/3699) (Installation): Windows setup wizard crashes when InstallDir registry key is not set
* [#3680](https://github.com/icinga/icinga2/issues/3680) (Installation): Incorrect redirect for stderr in /usr/lib/icinga2/prepare-dirs
* [#3656](https://github.com/icinga/icinga2/issues/3656) (Packages): Build fails on SLES 11 SP3 with GCC 4.8

## 2.4.0 (2015-11-16)

### Notes

* API
    * RESTful API with basic auth or client certificates
    * Filters, types, permissions
    * configuration package management
    * query/create/modify/delete config objects at runtime
    * status queries for global stats
    * actions (e.g. acknowledge all service problems)
    * event streams
* ITL and Plugin Check Command definitions
    * The 'running_kernel' check command was moved to the plugins-contrib section. You have to update your config to include 'plugins-contrib'
* Configuration
    * The global constants Enable* and Vars have been removed. Use the IcingaApplication object attributes instead.
* Features
    * New Graphite tree. Please check the documentation how enable the legacy schema.
    * IcingaStatusWriter feature has been deprecated and will be removed in future versions.
    * Modified attributes are not exposed as bit mask to external interfaces anymore (api related changes). External commands like CHANGE_*_MODATTR have been removed.

### Enhancement

* [#3642](https://github.com/icinga/icinga2/issues/3642): Release 2.4.0
* [#3624](https://github.com/icinga/icinga2/issues/3624) (API): Enhance programmatic examples for the API docs
* [#3611](https://github.com/icinga/icinga2/issues/3611) (API): Change object query result set
* [#3609](https://github.com/icinga/icinga2/issues/3609) (API): Change 'api setup' into a manual step while configuring the API
* [#3608](https://github.com/icinga/icinga2/issues/3608) (CLI): Icinga 2 script debugger
* [#3591](https://github.com/icinga/icinga2/issues/3591) (CLI): Change output format for 'icinga2 console'
* [#3580](https://github.com/icinga/icinga2/issues/3580): Change GetLastStateUp/Down to host attributes
* [#3576](https://github.com/icinga/icinga2/issues/3576) (Plugins): Missing parameters for check jmx4perl
* [#3561](https://github.com/icinga/icinga2/issues/3561) (CLI): Use ZoneName variable for parent\_zone in node update-config
* [#3537](https://github.com/icinga/icinga2/issues/3537) (CLI): Rewrite man page
* [#3531](https://github.com/icinga/icinga2/issues/3531) (DB IDO): Add the name for comments/downtimes next to legacy\_id to DB IDO
* [#3515](https://github.com/icinga/icinga2/issues/3515): Remove api.cpp, api.hpp 
* [#3508](https://github.com/icinga/icinga2/issues/3508) (Cluster): Add getter for endpoint 'connected' attribute
* [#3507](https://github.com/icinga/icinga2/issues/3507) (API): Hide internal attributes
* [#3506](https://github.com/icinga/icinga2/issues/3506) (API): Original attributes list in IDO
* [#3503](https://github.com/icinga/icinga2/issues/3503) (API): Log a warning message on unauthorized http request
* [#3502](https://github.com/icinga/icinga2/issues/3502) (API): Use the API for "icinga2 console"
* [#3498](https://github.com/icinga/icinga2/issues/3498) (DB IDO): DB IDO should provide its connected state via /v1/status
* [#3488](https://github.com/icinga/icinga2/issues/3488) (API): Document that modified attributes require accept\_config for cluster/clients
* [#3469](https://github.com/icinga/icinga2/issues/3469) (Configuration): Pretty-print arrays and dictionaries when converting them to strings
* [#3463](https://github.com/icinga/icinga2/issues/3463) (API): Change object version to timestamps for diff updates on config sync
* [#3452](https://github.com/icinga/icinga2/issues/3452) (Configuration): Provide keywords to retrieve the current file name at parse time
* [#3435](https://github.com/icinga/icinga2/issues/3435) (API): Move /v1/\<type\> to /v1/objects/\<type\>
* [#3432](https://github.com/icinga/icinga2/issues/3432) (API): Rename statusqueryhandler to objectqueryhandler
* [#3419](https://github.com/icinga/icinga2/issues/3419) (API): Sanitize error status codes and messages
* [#3414](https://github.com/icinga/icinga2/issues/3414): Make ConfigObject::{G,S}etField\(\) method public
* [#3386](https://github.com/icinga/icinga2/issues/3386) (API): Add global status handler for the API
* [#3357](https://github.com/icinga/icinga2/issues/3357) (API): Implement CSRF protection for the API
* [#3354](https://github.com/icinga/icinga2/issues/3354) (API): Implement joins for status queries
* [#3343](https://github.com/icinga/icinga2/issues/3343) (API): Implement a demo API client: Icinga Studio
* [#3341](https://github.com/icinga/icinga2/issues/3341) (API): URL class improvements
* [#3340](https://github.com/icinga/icinga2/issues/3340) (API): Add plural\_name field to /v1/types
* [#3332](https://github.com/icinga/icinga2/issues/3332) (Configuration): Use an AST node for the 'library' keyword
* [#3297](https://github.com/icinga/icinga2/issues/3297) (Configuration): Implement ignore\_on\_error keyword
* [#3296](https://github.com/icinga/icinga2/issues/3296) (API): Rename config/modules to config/packages
* [#3291](https://github.com/icinga/icinga2/issues/3291) (API): Remove debug messages in HttpRequest class
* [#3290](https://github.com/icinga/icinga2/issues/3290): Add String::ToLower/ToUpper
* [#3287](https://github.com/icinga/icinga2/issues/3287) (API): Add package attribute for ConfigObject and set its origin
* [#3285](https://github.com/icinga/icinga2/issues/3285) (API): Implement support for restoring modified attributes
* [#3283](https://github.com/icinga/icinga2/issues/3283) (API): Implement support for indexers in ConfigObject::RestoreAttribute
* [#3282](https://github.com/icinga/icinga2/issues/3282): Implement Object\#clone and rename Array/Dictionary\#clone to shallow\_clone
* [#3280](https://github.com/icinga/icinga2/issues/3280): Add override keyword for all relevant methods
* [#3278](https://github.com/icinga/icinga2/issues/3278) (API): Figure out how to sync dynamically created objects inside the cluster
* [#3277](https://github.com/icinga/icinga2/issues/3277) (API): Ensure that runtime config objects are persisted on disk
* [#3272](https://github.com/icinga/icinga2/issues/3272): Implement the 'base' field for the Type class
* [#3267](https://github.com/icinga/icinga2/issues/3267): Rename DynamicObject/DynamicType to ConfigObject/ConfigType
* [#3240](https://github.com/icinga/icinga2/issues/3240): Implement support for attaching GDB to the Icinga process on crash
* [#3238](https://github.com/icinga/icinga2/issues/3238) (API): Implement global modified attributes
* [#3233](https://github.com/icinga/icinga2/issues/3233) (API): Implement support for . in modify\_attribute
* [#3232](https://github.com/icinga/icinga2/issues/3232) (API): Remove GetModifiedAttributes/SetModifiedAttributes
* [#3231](https://github.com/icinga/icinga2/issues/3231) (API): Re-implement events for attribute changes
* [#3230](https://github.com/icinga/icinga2/issues/3230) (API): Validation for modified attributes
* [#3203](https://github.com/icinga/icinga2/issues/3203) (Configuration): Setting global variables with i2tcl doesn't work
* [#3197](https://github.com/icinga/icinga2/issues/3197) (API): Make Comments and Downtime types available as ConfigObject type in the API
* [#3193](https://github.com/icinga/icinga2/issues/3193) (API): Update the url parsers behaviour
* [#3177](https://github.com/icinga/icinga2/issues/3177) (API): Documentation for config management API
* [#3173](https://github.com/icinga/icinga2/issues/3173) (API): Add real path sanity checks to provided file paths
* [#3172](https://github.com/icinga/icinga2/issues/3172): String::Trim\(\) should return a new string rather than modifying the current string
* [#3169](https://github.com/icinga/icinga2/issues/3169) (API): Implement support for X-HTTP-Method-Override
* [#3168](https://github.com/icinga/icinga2/issues/3168): Add Array::FromVector\(\) method
* [#3167](https://github.com/icinga/icinga2/issues/3167): Add exceptions for Utility::MkDir{,P}
* [#3154](https://github.com/icinga/icinga2/issues/3154): Move url to /lib/remote from /lib/base
* [#3144](https://github.com/icinga/icinga2/issues/3144): Register ServiceOK, ServiceWarning, HostUp, etc. as constants
* [#3140](https://github.com/icinga/icinga2/issues/3140) (API): Implement base64 de- and encoder
* [#3094](https://github.com/icinga/icinga2/issues/3094) (API): Implement ApiUser type
* [#3093](https://github.com/icinga/icinga2/issues/3093) (API): Implement URL parser
* [#3090](https://github.com/icinga/icinga2/issues/3090) (Graphite): New Graphite schema
* [#3089](https://github.com/icinga/icinga2/issues/3089) (API): Implement support for filter\_vars
* [#3083](https://github.com/icinga/icinga2/issues/3083) (API): Define RESTful url schema
* [#3082](https://github.com/icinga/icinga2/issues/3082) (API): Implement support for HTTP
* [#3065](https://github.com/icinga/icinga2/issues/3065): Allow comments when parsing JSON
* [#3025](https://github.com/icinga/icinga2/issues/3025) (DB IDO): DB IDO/Livestatus: Add zone object table w/ endpoint members
* [#2934](https://github.com/icinga/icinga2/issues/2934) (API): API Documentation
* [#2933](https://github.com/icinga/icinga2/issues/2933) (API): Implement config file management commands
* [#2932](https://github.com/icinga/icinga2/issues/2932) (API): Staging for configuration validation
* [#2931](https://github.com/icinga/icinga2/issues/2931) (API): Support validating configuration changes
* [#2930](https://github.com/icinga/icinga2/issues/2930) (API): Commands for adding and removing objects
* [#2929](https://github.com/icinga/icinga2/issues/2929) (API): Multiple sources for zone configuration tree
* [#2928](https://github.com/icinga/icinga2/issues/2928) (API): Implement support for writing configuration files 
* [#2927](https://github.com/icinga/icinga2/issues/2927) (API): Update modules to support adding and removing objects at runtime
* [#2926](https://github.com/icinga/icinga2/issues/2926) (API): Dependency tracking for objects
* [#2925](https://github.com/icinga/icinga2/issues/2925) (API): Disallow changes for certain config attributes at runtime
* [#2923](https://github.com/icinga/icinga2/issues/2923) (API): Changelog for modified attributes
* [#2921](https://github.com/icinga/icinga2/issues/2921) (API): API status queries
* [#2918](https://github.com/icinga/icinga2/issues/2918) (API): API permissions
* [#2917](https://github.com/icinga/icinga2/issues/2917) (API): Create default administrative user
* [#2916](https://github.com/icinga/icinga2/issues/2916) (API): Password-based authentication for the API
* [#2915](https://github.com/icinga/icinga2/issues/2915) (API): Certificate-based authentication for the API
* [#2914](https://github.com/icinga/icinga2/issues/2914) (API): Enable the ApiListener by default
* [#2913](https://github.com/icinga/icinga2/issues/2913) (API): Configuration file management for the API
* [#2912](https://github.com/icinga/icinga2/issues/2912) (API): Runtime configuration for the API
* [#2911](https://github.com/icinga/icinga2/issues/2911) (API): Add modified attribute support for the API
* [#2910](https://github.com/icinga/icinga2/issues/2910) (API): Add commands \(actions\) for the API
* [#2909](https://github.com/icinga/icinga2/issues/2909) (API): Implement status queries for the API
* [#2908](https://github.com/icinga/icinga2/issues/2908) (API): Event stream support for the API
* [#2907](https://github.com/icinga/icinga2/issues/2907) (API): Implement filters for the API
* [#2906](https://github.com/icinga/icinga2/issues/2906) (API): Reflection support for the API
* [#2904](https://github.com/icinga/icinga2/issues/2904) (API): Basic API framework
* [#2901](https://github.com/icinga/icinga2/issues/2901) (Configuration): Implement sandbox mode for the config parser
* [#2887](https://github.com/icinga/icinga2/issues/2887) (Configuration): Remove the ScopeCurrent constant
* [#2857](https://github.com/icinga/icinga2/issues/2857): Avoid unnecessary dictionary lookups
* [#2838](https://github.com/icinga/icinga2/issues/2838): Move implementation code from thpp files into separate files
* [#2826](https://github.com/icinga/icinga2/issues/2826) (Configuration): Use DebugHint information when reporting validation errors
* [#2814](https://github.com/icinga/icinga2/issues/2814): Add support for the C++11 keyword 'override'
* [#2809](https://github.com/icinga/icinga2/issues/2809) (Configuration): Implement constructor-style casts
* [#2788](https://github.com/icinga/icinga2/issues/2788) (Configuration): Refactor the startup process
* [#2785](https://github.com/icinga/icinga2/issues/2785) (CLI): Implement support for libedit
* [#2757](https://github.com/icinga/icinga2/issues/2757): Deprecate IcingaStatusWriter feature
* [#2755](https://github.com/icinga/icinga2/issues/2755) (DB IDO): Implement support for CLIENT\_MULTI\_STATEMENTS
* [#2741](https://github.com/icinga/icinga2/issues/2741) (DB IDO): Add support for current and current-1 db ido schema version
* [#2740](https://github.com/icinga/icinga2/issues/2740) (DB IDO): Add embedded DB IDO version health check
* [#2722](https://github.com/icinga/icinga2/issues/2722): Allow some of the Array and Dictionary methods to be inlined by the compiler
* [#2514](https://github.com/icinga/icinga2/issues/2514): 'icinga2 console' should serialize temporary attributes \(rather than just config + state\)
* [#2474](https://github.com/icinga/icinga2/issues/2474) (Graphite): graphite writer should pass "-" in host names and "." in perf data 
* [#2438](https://github.com/icinga/icinga2/issues/2438) (API): Add icinga, cluster, cluster-zone check information to the ApiListener status handler
* [#2268](https://github.com/icinga/icinga2/issues/2268) (Configuration): Validators should be implemented in \(auto-generated\) native code

### Bug

* [#3669](https://github.com/icinga/icinga2/issues/3669): Use notify\_one in WorkQueue::Enqueue
* [#3667](https://github.com/icinga/icinga2/issues/3667): Utility::FormatErrorNumber fails when error message uses arguments
* [#3649](https://github.com/icinga/icinga2/issues/3649) (DB IDO): Group memberships are not updated for runtime created objects
* [#3648](https://github.com/icinga/icinga2/issues/3648) (API): API overwrites \(and then deletes\) config file when trying to create an object that already exists
* [#3647](https://github.com/icinga/icinga2/issues/3647) (API): Don't allow users to set state attributes via PUT
* [#3645](https://github.com/icinga/icinga2/issues/3645): Deadlock in MacroProcessor::EvaluateFunction
* [#3635](https://github.com/icinga/icinga2/issues/3635): modify\_attribute: object cannot be cloned
* [#3633](https://github.com/icinga/icinga2/issues/3633) (API): Detailed error message is missing when object creation via API fails
* [#3632](https://github.com/icinga/icinga2/issues/3632) (API): API call doesn't fail when trying to use a template that doesn't exist
* [#3625](https://github.com/icinga/icinga2/issues/3625): Improve location information for errors in API filters
* [#3622](https://github.com/icinga/icinga2/issues/3622) (API): /v1/console should only use a single permission
* [#3620](https://github.com/icinga/icinga2/issues/3620) (API): 'remove-comment' action does not support filters
* [#3619](https://github.com/icinga/icinga2/issues/3619) (CLI): 'api setup' should create a user even when api feature is already enabled
* [#3618](https://github.com/icinga/icinga2/issues/3618) (CLI): Autocompletion doesn't work in the debugger
* [#3617](https://github.com/icinga/icinga2/issues/3617) (API): There's a variable called 'string' in filter expressions
* [#3607](https://github.com/icinga/icinga2/issues/3607) (CLI): Broken build - unresolved external symbol "public: void \_\_thiscall icinga::ApiClient::ExecuteScript...
* [#3602](https://github.com/icinga/icinga2/issues/3602) (DB IDO): Async mysql queries aren't logged in the debug log
* [#3601](https://github.com/icinga/icinga2/issues/3601): Don't validate custom attributes that aren't strings
* [#3600](https://github.com/icinga/icinga2/issues/3600): Crash in ConfigWriter::EmitIdentifier
* [#3598](https://github.com/icinga/icinga2/issues/3598) (CLI): Spaces do not work in command arguments
* [#3595](https://github.com/icinga/icinga2/issues/3595) (DB IDO): Change session\_token to integer timestamp
* [#3593](https://github.com/icinga/icinga2/issues/3593): Fix indentation for Dictionary::ToString
* [#3587](https://github.com/icinga/icinga2/issues/3587): Crash in ConfigWriter::GetKeywords
* [#3586](https://github.com/icinga/icinga2/issues/3586) (Cluster): Circular reference between \*Connection and TlsStream objects
* [#3583](https://github.com/icinga/icinga2/issues/3583) (API): Mismatch on {comment,downtime}\_id vs internal name in the API
* [#3581](https://github.com/icinga/icinga2/issues/3581): CreatePipeOverlapped is not thread-safe
* [#3579](https://github.com/icinga/icinga2/issues/3579): Figure out whether we need the Checkable attributes state\_raw, last\_state\_raw, hard\_state\_raw
* [#3577](https://github.com/icinga/icinga2/issues/3577) (Plugins): Increase the default timeout for OS checks
* [#3574](https://github.com/icinga/icinga2/issues/3574) (API): Plural name rule not treating edge case correcly
* [#3572](https://github.com/icinga/icinga2/issues/3572) (API): IcingaStudio: Accessing non-ConfigObjects causes ugly exception
* [#3569](https://github.com/icinga/icinga2/issues/3569) (API): Incorrect JSON-RPC message causes Icinga 2 to crash
* [#3566](https://github.com/icinga/icinga2/issues/3566) (DB IDO): Unique constraint violation with multiple comment inserts in DB IDO
* [#3558](https://github.com/icinga/icinga2/issues/3558) (DB IDO): IDO tries to execute empty UPDATE queries
* [#3554](https://github.com/icinga/icinga2/issues/3554) (Configuration): Crash in IndexerExpression::GetReference when attempting to set an attribute on an object other than the current one
* [#3551](https://github.com/icinga/icinga2/issues/3551) (Configuration): Line continuation is broken in 'icinga2 console'
* [#3548](https://github.com/icinga/icinga2/issues/3548) (Configuration): Don't allow scripts to access FANoUserView attributes in sandbox mode
* [#3546](https://github.com/icinga/icinga2/issues/3546) (Cluster): Improve error handling during log replay
* [#3536](https://github.com/icinga/icinga2/issues/3536) (CLI): Improve --help output for the --log-level option
* [#3535](https://github.com/icinga/icinga2/issues/3535) (CLI): "Command options" is empty when executing icinga2 without any argument.
* [#3534](https://github.com/icinga/icinga2/issues/3534) (DB IDO): Custom variables aren't removed from the IDO database
* [#3524](https://github.com/icinga/icinga2/issues/3524) (DB IDO): Changing a group's attributes causes duplicate rows in the icinga\_\*group\_members table
* [#3517](https://github.com/icinga/icinga2/issues/3517): OpenBSD: hang during ConfigItem::ActivateItems\(\) in daemon startup
* [#3514](https://github.com/icinga/icinga2/issues/3514) (CLI): Misleading wording in generated zones.conf
* [#3501](https://github.com/icinga/icinga2/issues/3501) (API): restore\_attribute does not work in clusters
* [#3489](https://github.com/icinga/icinga2/issues/3489) (API): Ensure that modified attributes work with clients with local config and no zone attribute
* [#3485](https://github.com/icinga/icinga2/issues/3485) (API): Icinga2 API performance regression
* [#3482](https://github.com/icinga/icinga2/issues/3482) (API): Version updates are not working properly
* [#3468](https://github.com/icinga/icinga2/issues/3468) (CLI): icinga2 repository host add does not work
* [#3462](https://github.com/icinga/icinga2/issues/3462): ConfigWriter::EmitValue should format floating point values properly
* [#3461](https://github.com/icinga/icinga2/issues/3461) (API): Config sync does not set endpoint syncing and plays disconnect-sync ping-pong
* [#3459](https://github.com/icinga/icinga2/issues/3459) (API): /v1/objects/\<type\> returns an HTTP error when there are no objects of that type
* [#3457](https://github.com/icinga/icinga2/issues/3457) (API): Config Sync shouldn't send updates for objects the client doesn't have access to
* [#3451](https://github.com/icinga/icinga2/issues/3451) (API): Properly encode URLs in Icinga Studio
* [#3448](https://github.com/icinga/icinga2/issues/3448) (API): Use a temporary file for modified-attributes.conf updates
* [#3445](https://github.com/icinga/icinga2/issues/3445) (Configuration): ASCII NULs don't work in string values
* [#3438](https://github.com/icinga/icinga2/issues/3438) (API): URL parser is cutting off last character
* [#3434](https://github.com/icinga/icinga2/issues/3434) (API): PerfdataValue is not properly serialised in status queries
* [#3433](https://github.com/icinga/icinga2/issues/3433) (API): Move the Collection status handler to /v1/status
* [#3422](https://github.com/icinga/icinga2/issues/3422) (Configuration): Detect infinite recursion in user scripts
* [#3411](https://github.com/icinga/icinga2/issues/3411) (API): API actions do not follow REST guidelines
* [#3383](https://github.com/icinga/icinga2/issues/3383) (DB IDO): Add object\_id where clause for icinga\_downtimehistory
* [#3345](https://github.com/icinga/icinga2/issues/3345) (API): Error handling in HttpClient/icinga-studio
* [#3338](https://github.com/icinga/icinga2/issues/3338) (CLI): Unused variable console\_type in consolecommand.cpp
* [#3336](https://github.com/icinga/icinga2/issues/3336) (API): Filtering by name doesn't work
* [#3335](https://github.com/icinga/icinga2/issues/3335) (API): HTTP keep-alive does not work with .NET WebClient
* [#3330](https://github.com/icinga/icinga2/issues/3330): Unused variable 'dobj' in configobject.tcpp
* [#3328](https://github.com/icinga/icinga2/issues/3328) (Configuration): Don't parse config files for branches not taken
* [#3315](https://github.com/icinga/icinga2/issues/3315) (Configuration): Crash in ConfigCompiler::RegisterZoneDir
* [#3302](https://github.com/icinga/icinga2/issues/3302) (API): Implement support for '.' when persisting modified attributes
* [#3301](https://github.com/icinga/icinga2/issues/3301): Fix formatting in mkclass
* [#3264](https://github.com/icinga/icinga2/issues/3264) (API): Do not let API users create objects with invalid names
* [#3250](https://github.com/icinga/icinga2/issues/3250) (API): Missing conf.d or zones.d cause parse failure
* [#3248](https://github.com/icinga/icinga2/issues/3248): Crash during cluster log replay
* [#3244](https://github.com/icinga/icinga2/issues/3244) (CLI): Color codes in console prompt break line editing
* [#3242](https://github.com/icinga/icinga2/issues/3242) (CLI): Crash in ScriptFrame::~ScriptFrame
* [#3227](https://github.com/icinga/icinga2/issues/3227) (CLI): console autocompletion should take into account parent classes' prototypes
* [#3215](https://github.com/icinga/icinga2/issues/3215) (API): win32 build: S\_ISDIR is undefined
* [#3205](https://github.com/icinga/icinga2/issues/3205) (Configuration): ScriptFrame's 'Self' attribute gets corrupted when an expression throws an exception
* [#3202](https://github.com/icinga/icinga2/issues/3202) (Configuration): Operator - should not work with "" and numbers
* [#3198](https://github.com/icinga/icinga2/issues/3198): Accessing field ID 0 \("prototype"\) fails
* [#3182](https://github.com/icinga/icinga2/issues/3182) (API): Broken cluster config sync w/o include\_zones
* [#3171](https://github.com/icinga/icinga2/issues/3171) (API): Problem with child nodes in http url registry
* [#3138](https://github.com/icinga/icinga2/issues/3138) (CLI): 'node wizard/setup' should always generate new CN certificates
* [#3131](https://github.com/icinga/icinga2/issues/3131) (DB IDO): Overflow in freshness\_threshold column \(smallint\) w/ DB IDO MySQL
* [#3109](https://github.com/icinga/icinga2/issues/3109) (API): build failure: demo module
* [#3087](https://github.com/icinga/icinga2/issues/3087) (DB IDO): Fix incorrect datatype for the check\_source column in icinga\_statehistory table
* [#2974](https://github.com/icinga/icinga2/issues/2974) (Configuration): Remove incorrect 'ignore where' expression from 'ssh' apply example
* [#2939](https://github.com/icinga/icinga2/issues/2939) (Cluster): Wrong vars changed handler in api events
* [#2884](https://github.com/icinga/icinga2/issues/2884) (DB IDO): PostgreSQL schema sets default timestamps w/o time zone
* [#2879](https://github.com/icinga/icinga2/issues/2879): Compiler warnings with latest HEAD 5ac5f98
* [#2870](https://github.com/icinga/icinga2/issues/2870) (DB IDO): pgsql driver does not have latest mysql changes synced
* [#2863](https://github.com/icinga/icinga2/issues/2863) (Configuration): Crash in VMOps::FunctionCall
* [#2850](https://github.com/icinga/icinga2/issues/2850) (Configuration): Validation fails even though field is not required 
* [#2824](https://github.com/icinga/icinga2/issues/2824) (DB IDO): Failed assertion in IdoMysqlConnection::FieldToEscapedString  
* [#2808](https://github.com/icinga/icinga2/issues/2808) (Configuration): Make default notifications include users from host.vars.notification.mail.users
* [#2803](https://github.com/icinga/icinga2/issues/2803): Don't allow users to instantiate the StreamLogger class

### ITL

* [#3584](https://github.com/icinga/icinga2/issues/3584) (ITL): Add ipv4/ipv6 only to tcp and http CheckCommand
* [#3582](https://github.com/icinga/icinga2/issues/3582) (ITL): Add check command mysql
* [#3578](https://github.com/icinga/icinga2/issues/3578) (ITL): Add check command negate
* [#3532](https://github.com/icinga/icinga2/issues/3532) (ITL): 'dig\_lookup' custom attribute for the 'dig' check command isn't optional
* [#3525](https://github.com/icinga/icinga2/issues/3525) (ITL): Ability to set port on SNMP Checks
* [#3490](https://github.com/icinga/icinga2/issues/3490) (ITL): Add check command nginx\_status
* [#2964](https://github.com/icinga/icinga2/issues/2964) (ITL): Move 'running\_kernel' check command to plugins-contrib 'operating system' section
* [#2784](https://github.com/icinga/icinga2/issues/2784) (ITL): Move the base command templates into libmethods

### Documentation

* [#3663](https://github.com/icinga/icinga2/issues/3663) (Documentation): Update wxWidgets documentation for Icinga Studio
* [#3640](https://github.com/icinga/icinga2/issues/3640) (Documentation): Explain DELETE for config stages/packages
* [#3638](https://github.com/icinga/icinga2/issues/3638) (Documentation): Documentation for /v1/types
* [#3631](https://github.com/icinga/icinga2/issues/3631) (Documentation): Documentation for the script debugger
* [#3630](https://github.com/icinga/icinga2/issues/3630) (Documentation): Explain variable names for joined objects in filter expressions
* [#3629](https://github.com/icinga/icinga2/issues/3629) (Documentation): Documentation for /v1/console
* [#3628](https://github.com/icinga/icinga2/issues/3628) (Documentation): Mention wxWidget \(optional\) requirement in INSTALL.md
* [#3626](https://github.com/icinga/icinga2/issues/3626) (Documentation): Icinga 2 API Docs
* [#3621](https://github.com/icinga/icinga2/issues/3621) (Documentation): Documentation should not reference real host names
* [#3563](https://github.com/icinga/icinga2/issues/3563) (Documentation): Documentation: Reorganize Livestatus and alternative frontends
* [#3547](https://github.com/icinga/icinga2/issues/3547) (Documentation): Incorrect attribute name in the documentation
* [#3516](https://github.com/icinga/icinga2/issues/3516) (Documentation): Add documentation for apply+for in the language reference chapter
* [#3511](https://github.com/icinga/icinga2/issues/3511) (Documentation): Escaping $ not documented
* [#3500](https://github.com/icinga/icinga2/issues/3500) (Documentation): Add 'support' tracker to changelog.py
* [#3477](https://github.com/icinga/icinga2/issues/3477) (Documentation): Remove duplicated text in section "Apply Notifications to Hosts and Services"
* [#3426](https://github.com/icinga/icinga2/issues/3426) (Documentation): Add documentation for api-users.conf and app.conf
* [#3281](https://github.com/icinga/icinga2/issues/3281) (Documentation): Document Object\#clone

### Support

* [#3662](https://github.com/icinga/icinga2/issues/3662) (Packages): Download URL for NSClient++ is incorrect
* [#3615](https://github.com/icinga/icinga2/issues/3615) (Packages): Update OpenSSL for the Windows builds
* [#3614](https://github.com/icinga/icinga2/issues/3614) (Installation): Don't try to use --gc-sections on Solaris
* [#3522](https://github.com/icinga/icinga2/issues/3522) (Packages): 'which' isn't available in a minimal CentOS container
* [#3063](https://github.com/icinga/icinga2/issues/3063) (Installation): "-Wno-deprecated-register" compiler option breaks builds on SLES 11
* [#2893](https://github.com/icinga/icinga2/issues/2893) (Installation): icinga demo module can not be built
* [#2858](https://github.com/icinga/icinga2/issues/2858) (Packages): Specify pidfile for status\_of\_proc in the init script
* [#2802](https://github.com/icinga/icinga2/issues/2802) (Packages): Update OpenSSL for the Windows builds

## 2.3.11 (2015-10-20)

### Notes

* Function for performing CIDR matches: cidr_match()
* New methods: String#reverse and Array#reverse
* New ITL command definitions: nwc_health, hpasm, squid, pgsql
* Additional arguments for ITL command definitions: by_ssh, dig, pop, spop, imap, simap
* Documentation updates
* Various bugfixes

### Enhancement

* [#3494](https://github.com/icinga/icinga2/issues/3494) (DB IDO): Add a debug log message for updating the program status table in DB IDO
* [#3481](https://github.com/icinga/icinga2/issues/3481): New method: cidr\_match\(\)
* [#3421](https://github.com/icinga/icinga2/issues/3421): Implement the Array\#reverse and String\#reverse methods
* [#3327](https://github.com/icinga/icinga2/issues/3327): Implement a way for users to resolve commands+arguments in the same way Icinga does
* [#3326](https://github.com/icinga/icinga2/issues/3326): escape\_shell\_arg\(\) method
* [#2969](https://github.com/icinga/icinga2/issues/2969) (Metrics): Add timestamp support for OpenTsdbWriter

### Bug

* [#3492](https://github.com/icinga/icinga2/issues/3492) (Cluster): Wrong connection log message for global zones
* [#3491](https://github.com/icinga/icinga2/issues/3491): cidr\_match\(\) doesn't properly validate IP addresses
* [#3487](https://github.com/icinga/icinga2/issues/3487) (Cluster): ApiListener::SyncRelayMessage doesn't send message to all zone members
* [#3476](https://github.com/icinga/icinga2/issues/3476) (Compat): Missing Start call for base class in CheckResultReader
* [#3475](https://github.com/icinga/icinga2/issues/3475) (Compat): Checkresultreader is unable to process host checks
* [#3466](https://github.com/icinga/icinga2/issues/3466): "Not after" value overflows in X509 certificates on RHEL5
* [#3464](https://github.com/icinga/icinga2/issues/3464) (Cluster): Don't log messages we've already relayed to all relevant zones
* [#3460](https://github.com/icinga/icinga2/issues/3460) (Metrics): Performance Data Labels including '=' will not be displayed correct
* [#3454](https://github.com/icinga/icinga2/issues/3454): Percent character whitespace on Windows
* [#3449](https://github.com/icinga/icinga2/issues/3449) (Cluster): Don't throw an exception when replaying the current replay log file
* [#3446](https://github.com/icinga/icinga2/issues/3446): Deadlock in TlsStream::Close
* [#3428](https://github.com/icinga/icinga2/issues/3428) (Configuration): config checker reports wrong error on apply for rules
* [#3427](https://github.com/icinga/icinga2/issues/3427) (Configuration): Config parser problem with parenthesis and newlines 
* [#3423](https://github.com/icinga/icinga2/issues/3423) (Configuration): Remove unnecessary MakeLiteral calls in SetExpression::DoEvaluate
* [#3417](https://github.com/icinga/icinga2/issues/3417) (Configuration): null + null should not be ""
* [#3416](https://github.com/icinga/icinga2/issues/3416) (API): Problem with customvariable table update/insert queries
* [#3373](https://github.com/icinga/icinga2/issues/3373) (Livestatus): Improve error message for socket errors in Livestatus
* [#3324](https://github.com/icinga/icinga2/issues/3324) (Cluster): Deadlock in WorkQueue::Enqueue
* [#3204](https://github.com/icinga/icinga2/issues/3204) (Configuration): String methods cannot be invoked on an empty string
* [#3038](https://github.com/icinga/icinga2/issues/3038) (Livestatus): sending multiple Livestatus commands rejects all except the first
* [#2568](https://github.com/icinga/icinga2/issues/2568) (Cluster): check cluster-zone returns wrong log lag

### ITL

* [#3437](https://github.com/icinga/icinga2/issues/3437) (ITL): Add timeout argument for pop, spop, imap, simap commands
* [#3407](https://github.com/icinga/icinga2/issues/3407) (ITL): Make check\_disk.exe CheckCommand Config more verbose
* [#3399](https://github.com/icinga/icinga2/issues/3399) (ITL): expand check command dig
* [#3394](https://github.com/icinga/icinga2/issues/3394) (ITL): Add ipv4/ipv6 only to nrpe CheckCommand
* [#3385](https://github.com/icinga/icinga2/issues/3385) (ITL): Add check command pgsql
* [#3382](https://github.com/icinga/icinga2/issues/3382) (ITL): Add check command squid
* [#3235](https://github.com/icinga/icinga2/issues/3235) (ITL): check\_command for plugin check\_hpasm
* [#3214](https://github.com/icinga/icinga2/issues/3214) (ITL): add check command for check\_nwc\_health

### Documentation

* [#3479](https://github.com/icinga/icinga2/issues/3479) (Documentation): Improve timeperiod documentation
* [#3478](https://github.com/icinga/icinga2/issues/3478) (Documentation): Broken table layout in chapter 20
* [#3436](https://github.com/icinga/icinga2/issues/3436) (Documentation): Clarify on cluster/client naming convention and add troubleshooting section
* [#3430](https://github.com/icinga/icinga2/issues/3430) (Documentation): Find a better description for cluster communication requirements
* [#3409](https://github.com/icinga/icinga2/issues/3409) (Documentation): Windows Check Update -\> Access denied
* [#3408](https://github.com/icinga/icinga2/issues/3408) (Documentation): Improve documentation for check\_memory
* [#3406](https://github.com/icinga/icinga2/issues/3406) (Documentation): Update graphing section in the docs
* [#3402](https://github.com/icinga/icinga2/issues/3402) (Documentation): Update debug docs for core dumps and full backtraces
* [#3351](https://github.com/icinga/icinga2/issues/3351) (Documentation): Command Execution Bridge: Use of same endpoint names in examples for a better understanding
* [#3092](https://github.com/icinga/icinga2/issues/3092) (Documentation): Add FreeBSD setup to getting started

### Support

* [#3379](https://github.com/icinga/icinga2/issues/3379) (Installation): Rather use unique SID when granting rights for folders in NSIS on Windows Client
* [#3045](https://github.com/icinga/icinga2/issues/3045) (Packages): icinga2 ido mysql misspelled database username

## 2.3.10 (2015-09-05)

### Notes

* Feature 9218: Use the command_endpoint name as check_source value if defined

### Enhancement

* [#2985](https://github.com/icinga/icinga2/issues/2985): Use the command\_endpoint name as check\_source value if defined

### Bug

* [#3369](https://github.com/icinga/icinga2/issues/3369): Missing zero padding for generated CA serial.txt
* [#3352](https://github.com/icinga/icinga2/issues/3352): Wrong calculation for host compat state "UNREACHABLE" in DB IDO
* [#3348](https://github.com/icinga/icinga2/issues/3348) (Cluster): Missing fix for reload on Windows in 2.3.9
* [#3325](https://github.com/icinga/icinga2/issues/3325): Nested "outer" macro calls fails on \(handled\) missing "inner" values
* [#2811](https://github.com/icinga/icinga2/issues/2811) (DB IDO): String escape problem with PostgreSQL \>= 9.1 and standard\_conforming\_strings=on

## 2.3.9 (2015-08-26)

### Notes

* Fix that the first SOFT state is recognized as second SOFT state
* Implemented reload functionality for Windows
* New ITL check commands
* Documentation updates
* Various other bugfixes

### Enhancement

* [#3254](https://github.com/icinga/icinga2/issues/3254) (Livestatus): Use an empty dictionary for the 'this' scope when executing commands with Livestatus
* [#3253](https://github.com/icinga/icinga2/issues/3253): Implement the Dictionary\#keys method
* [#3206](https://github.com/icinga/icinga2/issues/3206): Implement Dictionary\#get and Array\#get
* [#3170](https://github.com/icinga/icinga2/issues/3170) (Configuration): Adding "-r" parameter to the check\_load command for dividing the load averages by the number of CPUs.

### Bug

* [#3305](https://github.com/icinga/icinga2/issues/3305) (Configuration): Icinga2 - too many open files - Exception
* [#3299](https://github.com/icinga/icinga2/issues/3299): Utility::Glob on Windows doesn't support wildcards in all but the last path component
* [#3292](https://github.com/icinga/icinga2/issues/3292): Serial number field is not properly initialized for CA certificates
* [#3279](https://github.com/icinga/icinga2/issues/3279) (DB IDO): Add missing category for IDO query
* [#3266](https://github.com/icinga/icinga2/issues/3266) (Plugins): Default disk checks on Windows fail because check\_disk doesn't support -K
* [#3260](https://github.com/icinga/icinga2/issues/3260): First SOFT state is recognized as second SOFT state
* [#3255](https://github.com/icinga/icinga2/issues/3255) (Cluster): Warning about invalid API function icinga::Hello
* [#3241](https://github.com/icinga/icinga2/issues/3241): Agent freezes when the check returns massive output
* [#3222](https://github.com/icinga/icinga2/issues/3222) (Configuration): Dict initializer incorrectly re-initializes field that is set to an empty string
* [#3211](https://github.com/icinga/icinga2/issues/3211) (Configuration): Operator + is inconsistent when used with empty and non-empty strings
* [#3200](https://github.com/icinga/icinga2/issues/3200) (CLI): icinga2 node wizard don't take zone\_name input
* [#3199](https://github.com/icinga/icinga2/issues/3199): Trying to set a field for a non-object instance fails
* [#3196](https://github.com/icinga/icinga2/issues/3196) (Cluster): Add log for missing EventCommand for command\_endpoints
* [#3194](https://github.com/icinga/icinga2/issues/3194): Set correct X509 version for certificates
* [#3149](https://github.com/icinga/icinga2/issues/3149) (CLI): missing config warning on empty port in endpoints
* [#3010](https://github.com/icinga/icinga2/issues/3010) (Cluster): cluster check w/ immediate parent and child zone endpoints
* [#2867](https://github.com/icinga/icinga2/issues/2867): Missing DEL\_DOWNTIME\_BY\_HOST\_NAME command required by Classic UI 1.x
* [#2352](https://github.com/icinga/icinga2/issues/2352) (Cluster): Reload does not work on Windows

### ITL

* [#3320](https://github.com/icinga/icinga2/issues/3320) (ITL): Add new arguments openvmtools for Open VM Tools
* [#3313](https://github.com/icinga/icinga2/issues/3313) (ITL): add check command nscp-local-counter
* [#3312](https://github.com/icinga/icinga2/issues/3312) (ITL): fix check command nscp-local
* [#3265](https://github.com/icinga/icinga2/issues/3265) (ITL): check\_command interfaces option match\_aliases has to be boolean
* [#3219](https://github.com/icinga/icinga2/issues/3219) (ITL): snmpv3 CheckCommand section improved
* [#3213](https://github.com/icinga/icinga2/issues/3213) (ITL): add check command for check\_mailq
* [#3208](https://github.com/icinga/icinga2/issues/3208) (ITL): Add check\_jmx4perl to ITL
* [#3186](https://github.com/icinga/icinga2/issues/3186) (ITL): check\_command for plugin check\_clamd
* [#3164](https://github.com/icinga/icinga2/issues/3164) (ITL): Add check\_redis to ITL
* [#3162](https://github.com/icinga/icinga2/issues/3162) (ITL): Add check\_yum to ITL
* [#3111](https://github.com/icinga/icinga2/issues/3111) (ITL): CheckCommand for check\_interfaces

### Documentation

* [#3319](https://github.com/icinga/icinga2/issues/3319) (Documentation): Duplicate severity type in the documentation for SyslogLogger
* [#3308](https://github.com/icinga/icinga2/issues/3308) (Documentation): Fix global Zone example to  "Global Configuration Zone for Templates"
* [#3262](https://github.com/icinga/icinga2/issues/3262) (Documentation): typo in docs
* [#3166](https://github.com/icinga/icinga2/issues/3166) (Documentation): Update gdb pretty printer docs w/ Python 3

### Support

* [#3298](https://github.com/icinga/icinga2/issues/3298) (Packages): Don't re-download NSCP for every build
* [#3239](https://github.com/icinga/icinga2/issues/3239) (Packages): missing check\_perfmon.exe 
* [#3216](https://github.com/icinga/icinga2/issues/3216) (Tests): Build fix for Boost 1.59

## 2.3.8 (2015-07-21)

### Notes

* Bugfixes

### Bug

* [#3160](https://github.com/icinga/icinga2/issues/3160) (Metrics): Escaping does not work for OpenTSDB perfdata plugin
* [#3151](https://github.com/icinga/icinga2/issues/3151) (DB IDO): DB IDO: Do not update endpointstatus table on config updates
* [#3120](https://github.com/icinga/icinga2/issues/3120) (Configuration): Don't allow "ignore where" for groups when there's no "assign where"

### ITL

* [#3161](https://github.com/icinga/icinga2/issues/3161) (ITL): checkcommand disk does not check free inode - check\_disk
* [#3152](https://github.com/icinga/icinga2/issues/3152) (ITL): Wrong parameter for CheckCommand "ping-common-windows"

## 2.3.7 (2015-07-15)

### Notes

* Bugfixes

### Bug

* [#3148](https://github.com/icinga/icinga2/issues/3148): Missing lock in ScriptUtils::Union
* [#3147](https://github.com/icinga/icinga2/issues/3147): Assertion failed in icinga::ScriptUtils::Intersection
* [#3136](https://github.com/icinga/icinga2/issues/3136) (DB IDO): DB IDO: endpoint\* tables are cleared on reload causing constraint violations
* [#3134](https://github.com/icinga/icinga2/issues/3134): Incorrect return value for the macro\(\) function
* [#3114](https://github.com/icinga/icinga2/issues/3114) (Configuration): Config parser ignores "ignore" in template definition
* [#3061](https://github.com/icinga/icinga2/issues/3061) (Cluster): Selective cluster reconnecting breaks client communication

### Documentation

* [#3142](https://github.com/icinga/icinga2/issues/3142) (Documentation): Enhance troubleshooting ssl errors & cluster replay log
* [#3135](https://github.com/icinga/icinga2/issues/3135) (Documentation): Wrong formatting in DB IDO extensions docs

## 2.3.6 (2015-07-08)

### Notes

* Require openssl1 on sles11sp3 from Security Module repository
  * Bug in SLES 11's OpenSSL version 0.9.8j preventing verification of generated certificates.
  * Re-create these certificates with 2.3.6 linking against openssl1 (cli command or CSR auto-signing).
* ITL: Add ldap, ntp_peer, mongodb and elasticsearch CheckCommand definitions
* Bugfixes

### Bug

* [#3118](https://github.com/icinga/icinga2/issues/3118) (Cluster): Generated certificates cannot be verified w/ openssl 0.9.8j on SLES 11
* [#3098](https://github.com/icinga/icinga2/issues/3098) (Cluster): Add log message for discarded cluster events \(e.g. from unauthenticated clients\)
* [#3097](https://github.com/icinga/icinga2/issues/3097): Fix stability issues in the TlsStream/Stream classes
* [#3088](https://github.com/icinga/icinga2/issues/3088) (Cluster): Windows client w/ command\_endpoint broken with $nscp\_path$ and NscpPath detection
* [#3084](https://github.com/icinga/icinga2/issues/3084) (CLI): node setup: indent accept\_config and accept\_commands
* [#3074](https://github.com/icinga/icinga2/issues/3074) (Notifications): Functions can't be specified as command arguments
* [#2979](https://github.com/icinga/icinga2/issues/2979) (CLI): port empty when using icinga2 node wizard

### ITL

* [#3132](https://github.com/icinga/icinga2/issues/3132) (ITL): new options for smtp CheckCommand
* [#3125](https://github.com/icinga/icinga2/issues/3125) (ITL): Add new options for ntp\_time CheckCommand
* [#3110](https://github.com/icinga/icinga2/issues/3110) (ITL): Add ntp\_peer CheckCommand
* [#3103](https://github.com/icinga/icinga2/issues/3103) (ITL): itl/plugins-contrib.d/\*.conf should point to PluginContribDir
* [#3091](https://github.com/icinga/icinga2/issues/3091) (ITL): Incorrect check\_ping.exe parameter in the ITL
* [#3066](https://github.com/icinga/icinga2/issues/3066) (ITL): snmpv3 CheckCommand: Add possibility to set securityLevel
* [#3064](https://github.com/icinga/icinga2/issues/3064) (ITL): Add elasticsearch checkcommand to itl
* [#3031](https://github.com/icinga/icinga2/issues/3031) (ITL): Missing 'snmp\_is\_cisco' in Manubulon snmp-memory command definition
* [#3002](https://github.com/icinga/icinga2/issues/3002) (ITL): Incorrect variable name in the ITL
* [#2975](https://github.com/icinga/icinga2/issues/2975) (ITL): Add "mongodb" CheckCommand definition
* [#2963](https://github.com/icinga/icinga2/issues/2963) (ITL): Add "ldap" CheckCommand for "check\_ldap" plugin

### Documentation

* [#3126](https://github.com/icinga/icinga2/issues/3126) (Documentation): Update getting started for Debian Jessie
* [#3108](https://github.com/icinga/icinga2/issues/3108) (Documentation): wrong default port documentated for nrpe
* [#3099](https://github.com/icinga/icinga2/issues/3099) (Documentation): Missing openssl verify in cluster troubleshooting docs
* [#3096](https://github.com/icinga/icinga2/issues/3096) (Documentation): Documentation for checks in an HA zone is wrong
* [#3086](https://github.com/icinga/icinga2/issues/3086) (Documentation): Wrong file reference in README.md
* [#3085](https://github.com/icinga/icinga2/issues/3085) (Documentation): Merge documentation fixes from GitHub
* [#1793](https://github.com/icinga/icinga2/issues/1793) (Documentation): add pagerduty notification documentation

### Support

* [#3123](https://github.com/icinga/icinga2/issues/3123) (Packages): Require gcc47-c++ on sles11 from SLES software development kit repository
* [#3122](https://github.com/icinga/icinga2/issues/3122) (Packages): mysql-devel is not available in sles11sp3
* [#3081](https://github.com/icinga/icinga2/issues/3081) (Installation): changelog.py: Allow to define project, make custom\_fields and changes optional
* [#3073](https://github.com/icinga/icinga2/issues/3073) (Installation): Enhance changelog.py with wordpress blogpost output
* [#2651](https://github.com/icinga/icinga2/issues/2651) (Packages): Add Icinga 2 to Chocolatey Windows Repository

## 2.3.5 (2015-06-17)

### Notes

* NSClient++ is now bundled with the Windows setup wizard and can optionally be installed
* Windows Wizard: "include <nscp>" is set by default
* Windows Wizard: Add update mode
* Plugins: Add check_perfmon plugin for Windows
* ITL: Add CheckCommand objects for Windows plugins ("include <windows-plugins>")
* ITL: Add CheckCommand definitions for "mongodb", "iftraffic", "disk_smb"
* ITL: Add arguments to CheckCommands "dns", "ftp", "tcp", "nscp"

### Enhancement

* [#3009](https://github.com/icinga/icinga2/issues/3009) (Configuration): Add the --load-all and --log options for nscp-local
* [#3008](https://github.com/icinga/icinga2/issues/3008) (Configuration): Include \<nscp\> by default on Windows
* [#2971](https://github.com/icinga/icinga2/issues/2971) (Metrics): Add timestamp support for PerfdataWriter
* [#2817](https://github.com/icinga/icinga2/issues/2817) (Configuration): Add CheckCommand objects for Windows plugins
* [#2794](https://github.com/icinga/icinga2/issues/2794) (Plugins): Add check\_perfmon plugin for Windows

### Bug

* [#3051](https://github.com/icinga/icinga2/issues/3051) (Plugins): plugins-contrib.d/databases.conf: wrong argument for mssql\_health
* [#3043](https://github.com/icinga/icinga2/issues/3043) (Compat): Multiline vars are broken in objects.cache output
* [#3039](https://github.com/icinga/icinga2/issues/3039) (Compat): Multi line output not correctly handled from compat channels
* [#3007](https://github.com/icinga/icinga2/issues/3007) (Configuration): Disk and 'icinga' services are missing in the default Windows config
* [#3006](https://github.com/icinga/icinga2/issues/3006) (Configuration): Some checks in the default Windows configuration fail
* [#2986](https://github.com/icinga/icinga2/issues/2986) (DB IDO): Missing custom attributes in backends if name is equal to object attribute
* [#2952](https://github.com/icinga/icinga2/issues/2952) (DB IDO): Incorrect type and state filter mapping for User objects in DB IDO
* [#2951](https://github.com/icinga/icinga2/issues/2951) (DB IDO): Downtimes are always "fixed"
* [#2945](https://github.com/icinga/icinga2/issues/2945) (DB IDO): Possible DB deadlock
* [#2940](https://github.com/icinga/icinga2/issues/2940) (Configuration): node update-config reports critical and warning
* [#2935](https://github.com/icinga/icinga2/issues/2935) (Configuration): WIN: syslog is not an enable-able feature in windows
* [#2894](https://github.com/icinga/icinga2/issues/2894) (DB IDO): Wrong timestamps w/ historical data replay in DB IDO
* [#2839](https://github.com/icinga/icinga2/issues/2839) (CLI): Node wont connect properly to master if host is is not set for Endpoint on new installs
* [#2836](https://github.com/icinga/icinga2/issues/2836): Icinga2 --version: Error showing Distribution
* [#2819](https://github.com/icinga/icinga2/issues/2819) (Configuration): Syntax Highlighting: host.address vs host.add 

### ITL

* [#3019](https://github.com/icinga/icinga2/issues/3019) (ITL): Add 'iftraffic' to plugins-contrib check command definitions
* [#3003](https://github.com/icinga/icinga2/issues/3003) (ITL): Add 'disk\_smb' Plugin CheckCommand definition
* [#2959](https://github.com/icinga/icinga2/issues/2959) (ITL): 'disk': wrong order of threshold command arguments
* [#2956](https://github.com/icinga/icinga2/issues/2956) (ITL): Add arguments to "tcp" CheckCommand
* [#2955](https://github.com/icinga/icinga2/issues/2955) (ITL): Add arguments to "ftp" CheckCommand
* [#2954](https://github.com/icinga/icinga2/issues/2954) (ITL): Add arguments to "dns" CheckCommand
* [#2949](https://github.com/icinga/icinga2/issues/2949) (ITL): Add 'check\_drivesize' as nscp-local check command
* [#2938](https://github.com/icinga/icinga2/issues/2938) (ITL): Add SHOWALL to NSCP Checkcommand
* [#2880](https://github.com/icinga/icinga2/issues/2880) (ITL): Including \<nscp\> on Linux fails with unregistered function

### Documentation

* [#3072](https://github.com/icinga/icinga2/issues/3072) (Documentation): Documentation: Move configuration before advanced topics
* [#3069](https://github.com/icinga/icinga2/issues/3069) (Documentation): Enhance cluster docs with HA command\_endpoints
* [#3068](https://github.com/icinga/icinga2/issues/3068) (Documentation): Enhance cluster/client troubleshooting
* [#3062](https://github.com/icinga/icinga2/issues/3062) (Documentation): Documentation: Update the link to register a new Icinga account
* [#3059](https://github.com/icinga/icinga2/issues/3059) (Documentation): Documentation: Typo
* [#3057](https://github.com/icinga/icinga2/issues/3057) (Documentation): Documentation: Extend Custom Attributes with the boolean type
* [#3056](https://github.com/icinga/icinga2/issues/3056) (Documentation): Wrong service table attributes in Livestatus documentation
* [#3055](https://github.com/icinga/icinga2/issues/3055) (Documentation): Documentation: Typo
* [#3049](https://github.com/icinga/icinga2/issues/3049) (Documentation): Update documentation for escape sequences
* [#3036](https://github.com/icinga/icinga2/issues/3036) (Documentation): Explain string concatenation in objects by real-world example
* [#3035](https://github.com/icinga/icinga2/issues/3035) (Documentation): Use a more simple example for passing command parameters
* [#3033](https://github.com/icinga/icinga2/issues/3033) (Documentation): Add local variable scope for \*Command to documentation \(host, service, etc\)
* [#3032](https://github.com/icinga/icinga2/issues/3032) (Documentation): Add typeof in 'assign/ignore where' expression as example
* [#3030](https://github.com/icinga/icinga2/issues/3030) (Documentation): Add examples for function usage in "set\_if" and "command" attributes
* [#3024](https://github.com/icinga/icinga2/issues/3024) (Documentation): Best practices: cluster config sync
* [#3017](https://github.com/icinga/icinga2/issues/3017) (Documentation): Update service apply for documentation
* [#3015](https://github.com/icinga/icinga2/issues/3015) (Documentation): Typo in Configuration Best Practice
* [#2966](https://github.com/icinga/icinga2/issues/2966) (Documentation): Include Windows support details in the documentation
* [#2965](https://github.com/icinga/icinga2/issues/2965) (Documentation): ITL Documentation: Add a link for passing custom attributes as command parameters
* [#2950](https://github.com/icinga/icinga2/issues/2950) (Documentation): Missing "\)" in last Apply Rules example
* [#2279](https://github.com/icinga/icinga2/issues/2279) (Documentation): Add documentation and CheckCommands for the windows plugins

### Support

* [#3016](https://github.com/icinga/icinga2/issues/3016) (Installation): Wrong permission etc on windows
* [#3011](https://github.com/icinga/icinga2/issues/3011) (Installation): Add support for installing NSClient++ in the Icinga 2 Windows wizard
* [#3005](https://github.com/icinga/icinga2/issues/3005) (Installation): Determine NSClient++ installation path using MsiGetComponentPath
* [#3004](https://github.com/icinga/icinga2/issues/3004) (Installation): --scm-installs fails when the service is already installed
* [#2994](https://github.com/icinga/icinga2/issues/2994) (Installation): Bundle NSClient++ in Windows Installer
* [#2973](https://github.com/icinga/icinga2/issues/2973) (Packages): SPEC: Give group write permissions for perfdata dir
* [#2451](https://github.com/icinga/icinga2/issues/2451) (Installation): Extend Windows installer with an update mode

## 2.3.4 (2015-04-20)

### Notes

* ITL: Check commands for various databases
* Improve validation messages for time periods
* Update max_check_attempts in generic-{host,service} templates
* Update logrotate configuration
* Bugfixes

### Enhancement

* [#2841](https://github.com/icinga/icinga2/issues/2841): Improve timeperiod validation error messages
* [#2791](https://github.com/icinga/icinga2/issues/2791) (Cluster): Agent Wizard: add options for API defaults

### Bug

* [#2903](https://github.com/icinga/icinga2/issues/2903) (Configuration): custom attributes with recursive macro function calls causing sigabrt
* [#2898](https://github.com/icinga/icinga2/issues/2898) (CLI): troubleshoot truncates crash reports
* [#2886](https://github.com/icinga/icinga2/issues/2886): Acknowledging problems w/ expire time does not add the expiry information to the related comment for IDO and compat
* [#2883](https://github.com/icinga/icinga2/issues/2883) (Notifications): Multiple log messages w/ "Attempting to send notifications for notification object"
* [#2882](https://github.com/icinga/icinga2/issues/2882) (DB IDO): scheduled\_downtime\_depth column is not reset when a downtime ends or when a downtime is being removed
* [#2881](https://github.com/icinga/icinga2/issues/2881) (DB IDO): Downtimes which have been triggered are not properly recorded in the database
* [#2878](https://github.com/icinga/icinga2/issues/2878) (DB IDO): Don't update scheduleddowntime table w/ trigger\_time column when only adding a downtime
* [#2855](https://github.com/icinga/icinga2/issues/2855): Fix complexity class for Dictionary::Get
* [#2853](https://github.com/icinga/icinga2/issues/2853) (CLI): Node wizard should only accept 'y', 'n', 'Y' and 'N' as answers for boolean questions  
* [#2842](https://github.com/icinga/icinga2/issues/2842) (Configuration): Default max\_check\_attempts should be lower for hosts than for services
* [#2840](https://github.com/icinga/icinga2/issues/2840) (Configuration): Validation errors for time ranges which span the DST transition
* [#2827](https://github.com/icinga/icinga2/issues/2827) (Configuration): logrotate does not work
* [#2801](https://github.com/icinga/icinga2/issues/2801) (Cluster): command\_endpoint check\_results are not replicated to other endpoints in the same zone

### ITL

* [#2891](https://github.com/icinga/icinga2/issues/2891) (ITL): web.conf is not in the RPM package
* [#2890](https://github.com/icinga/icinga2/issues/2890) (ITL): check\_disk order of command arguments 
* [#2834](https://github.com/icinga/icinga2/issues/2834) (ITL): Add arguments to the UPS check
* [#2770](https://github.com/icinga/icinga2/issues/2770) (ITL): Add database plugins to ITL

### Documentation

* [#2902](https://github.com/icinga/icinga2/issues/2902) (Documentation): Documentation: set\_if usage with boolean values and functions
* [#2876](https://github.com/icinga/icinga2/issues/2876) (Documentation): Typo in graphite feature enable documentation
* [#2868](https://github.com/icinga/icinga2/issues/2868) (Documentation): Fix a typo
* [#2843](https://github.com/icinga/icinga2/issues/2843) (Documentation): Add explanatory note for Icinga2 client documentation
* [#2837](https://github.com/icinga/icinga2/issues/2837) (Documentation): Fix a minor markdown error
* [#2832](https://github.com/icinga/icinga2/issues/2832) (Documentation): Reword documentation of check\_address

### Support

* [#2888](https://github.com/icinga/icinga2/issues/2888) (Installation): Vim syntax: Match groups before host/service/user objects
* [#2852](https://github.com/icinga/icinga2/issues/2852) (Installation): Windows Build: Flex detection
* [#2793](https://github.com/icinga/icinga2/issues/2793) (Packages): logrotate doesn't work on Ubuntu

## 2.3.3 (2015-03-26)

### Notes

* New function: parse_performance_data
* Include more details in --version
* Improve documentation
* Bugfixes

### Enhancement

* [#2771](https://github.com/icinga/icinga2/issues/2771): Include more details in --version
* [#2743](https://github.com/icinga/icinga2/issues/2743): New function: parse\_performance\_data
* [#2737](https://github.com/icinga/icinga2/issues/2737) (Notifications): Show state/type filter names in notice/debug log

### Bug

* [#2828](https://github.com/icinga/icinga2/issues/2828): Array in command arguments doesn't work
* [#2818](https://github.com/icinga/icinga2/issues/2818) (Configuration): Local variables in "apply for" are overridden
* [#2816](https://github.com/icinga/icinga2/issues/2816) (CLI): Segmentation fault when executing "icinga2 pki new-cert"
* [#2812](https://github.com/icinga/icinga2/issues/2812) (Configuration): Return doesn't work inside loops
* [#2807](https://github.com/icinga/icinga2/issues/2807) (Configuration): Figure out why command validators are not triggered 
* [#2778](https://github.com/icinga/icinga2/issues/2778) (Configuration): object Notification + apply Service fails with error "...refers to service which doesn't exist"
* [#2772](https://github.com/icinga/icinga2/issues/2772) (Plugins): Plugin "check\_http" is missing in Windows environments
* [#2768](https://github.com/icinga/icinga2/issues/2768) (Configuration): Add missing keywords in the syntax highlighting files
* [#2760](https://github.com/icinga/icinga2/issues/2760): Don't ignore extraneous arguments for functions
* [#2753](https://github.com/icinga/icinga2/issues/2753) (DB IDO): Don't update custom vars for each status update
* [#2752](https://github.com/icinga/icinga2/issues/2752): startup.log broken when the DB schema needs an update
* [#2749](https://github.com/icinga/icinga2/issues/2749) (Configuration): Missing config validator for command arguments 'set\_if'
* [#2718](https://github.com/icinga/icinga2/issues/2718) (Configuration): Update syntax highlighting for 2.3 features
* [#2557](https://github.com/icinga/icinga2/issues/2557) (Configuration): Improve error message for invalid field access
* [#2548](https://github.com/icinga/icinga2/issues/2548) (Configuration): Fix VIM syntax highlighting for comments

### ITL

* [#2823](https://github.com/icinga/icinga2/issues/2823) (ITL): wrong 'dns\_lookup' custom attribute default in command-plugins.conf 
* [#2799](https://github.com/icinga/icinga2/issues/2799) (ITL): Add "random" CheckCommand for test and demo purposes

### Documentation

* [#2825](https://github.com/icinga/icinga2/issues/2825) (Documentation): Fix incorrect perfdata templates in the documentation 
* [#2806](https://github.com/icinga/icinga2/issues/2806) (Documentation): Move release info in INSTALL.md into a separate file
* [#2779](https://github.com/icinga/icinga2/issues/2779) (Documentation): Correct HA documentation
* [#2777](https://github.com/icinga/icinga2/issues/2777) (Documentation): Typo and invalid example in the runtime macro documentation
* [#2776](https://github.com/icinga/icinga2/issues/2776) (Documentation): Remove prompt to create a TicketSalt from the wizard
* [#2775](https://github.com/icinga/icinga2/issues/2775) (Documentation): Explain processing logic/order of apply rules with for loops
* [#2774](https://github.com/icinga/icinga2/issues/2774) (Documentation): Revamp migration documentation
* [#2773](https://github.com/icinga/icinga2/issues/2773) (Documentation): Typo in doc library-reference
* [#2765](https://github.com/icinga/icinga2/issues/2765) (Documentation): Fix a typo in the documentation of ICINGA2\_WITH\_MYSQL and ICINGA2\_WITH\_PGSQL
* [#2756](https://github.com/icinga/icinga2/issues/2756) (Documentation): Add "access objects at runtime" examples to advanced section
* [#2738](https://github.com/icinga/icinga2/issues/2738) (Documentation): Update documentation for "apply for" rules
* [#2501](https://github.com/icinga/icinga2/issues/2501) (Documentation): Re-order the object types in alphabetical order

### Support

* [#2762](https://github.com/icinga/icinga2/issues/2762) (Installation): Flex version check does not reject unsupported versions
* [#2761](https://github.com/icinga/icinga2/issues/2761) (Installation): Build warnings with CMake 3.1.3

## 2.3.2 (2015-03-12)

### Notes

* Bugfixes

### Bug

* [#2747](https://github.com/icinga/icinga2/issues/2747): Log message for cli commands breaks the init script

## 2.3.1 (2015-03-12)

### Notes

* Bugfixes

Please note that this version fixes the default thresholds for the disk check which were inadvertently broken in 2.3.0; if you're using percent-based custom thresholds you will need to add the '%' sign to your custom attributes

### Enhancement

* [#2717](https://github.com/icinga/icinga2/issues/2717) (Configuration): Implement String\#contains

### Bug

* [#2739](https://github.com/icinga/icinga2/issues/2739): Crash in Dependency::Stop
* [#2736](https://github.com/icinga/icinga2/issues/2736): Fix formatting for the GDB stacktrace
* [#2735](https://github.com/icinga/icinga2/issues/2735): Make sure that the /var/log/icinga2/crash directory exists
* [#2731](https://github.com/icinga/icinga2/issues/2731) (Configuration): Config validation fail because of unexpected new-line
* [#2727](https://github.com/icinga/icinga2/issues/2727) (Cluster): Api heartbeat message response time problem
* [#2716](https://github.com/icinga/icinga2/issues/2716) (CLI): Missing program name in 'icinga2 --version'
* [#2672](https://github.com/icinga/icinga2/issues/2672): Kill signal sent only to check process, not whole process group

### ITL

* [#2483](https://github.com/icinga/icinga2/issues/2483) (ITL): Fix check\_disk thresholds: make sure partitions are the last arguments

### Documentation

* [#2732](https://github.com/icinga/icinga2/issues/2732) (Documentation): Update documentation for DB IDO HA Run-Once
* [#2728](https://github.com/icinga/icinga2/issues/2728) (Documentation): Fix check\_disk default thresholds and document the change of unit

### Support

* [#2742](https://github.com/icinga/icinga2/issues/2742) (Packages): Debian packages do not create /var/log/icinga2/crash

## 2.3.0 (2015-03-10)

### Notes

* Improved configuration validation
    * Unnecessary escapes are no longer permitted (e.g. \')
    * Dashes are no longer permitted in identifier names (as their semantics are ambiguous)
    * Unused values are detected (e.g. { "-M" })
    * Validation for time ranges has been improved
    * Additional validation rules for some object types (Notification and User)
* New language features
    * Implement a separate type for boolean values
    * Support for user-defined functions
    * Support for conditional statements (if/else)
    * Support for 'for' and 'while' loops
    * Support for local variables using the 'var' keyword
    * New operators: % (modulo), ^ (xor), - (unary minus) and + (unary plus)
    * Implemented prototype-based methods for most built-in types (e.g. [ 3, 2 ].sort())
    * Explicit access to local and global variables using the 'locals' and 'globals' keywords
    * Changed the order in which filters are evaluated for apply rules with 'for'
    * Make type objects accessible as global variables
    * Support for using functions in custom attributes
    * Access objects and their runtime attributes in functions (e.g. get_host(NodeName).state)
* ITL improvements
    * Additional check commands were added to the ITL
    * Additional arguments for existing check commands
* CLI improvements
    * Add the 'icinga2 console' CLI command which can be used to test expressions
    * Add the 'icinga2 troubleshoot' CLI command for collecting troubleshooting information
    * Performance improvements for the 'icinga2 node update-config' CLI command
    * Implement argument auto-completion for short options (e.g. daemon -c)
    * 'node setup' and 'node wizard' create backups for existing certificate files
* Add ignore_soft_states option for Dependency object configuration
* Fewer threads are used for socket I/O
* Flapping detection for hosts and services is disabled by default
* Added support for OpenTSDB
* New Livestatus tables: hostsbygroup, servicesbygroup, servicesbyhostgroup
* Include GDB backtrace in crash reports
* Various documentation improvements
* Solved a number of issues where cluster instances would not reconnect after intermittent connection problems
* A lot of other, minor changes

* [DB IDO schema upgrade](17-upgrading-icinga-2.md#upgrading-icinga-2) to `1.13.0` required!

### Enhancement

* [#2704](https://github.com/icinga/icinga2/issues/2704): Support the SNI TLS extension
* [#2702](https://github.com/icinga/icinga2/issues/2702): Add validator for time ranges in ScheduledDowntime objects
* [#2701](https://github.com/icinga/icinga2/issues/2701): Remove macro argument for IMPL\_TYPE\_LOOKUP
* [#2696](https://github.com/icinga/icinga2/issues/2696): Include GDB backtrace in crash reports
* [#2678](https://github.com/icinga/icinga2/issues/2678) (Configuration): Add support for else-if
* [#2663](https://github.com/icinga/icinga2/issues/2663) (Livestatus): Change Livestatus query log level to 'notice'
* [#2657](https://github.com/icinga/icinga2/issues/2657) (Cluster): Show slave lag for the cluster-zone check
* [#2635](https://github.com/icinga/icinga2/issues/2635) (Configuration): introduce time dependent variable values
* [#2634](https://github.com/icinga/icinga2/issues/2634) (Cluster): Add the ability to use a CA certificate as a way of verifying hosts for CSR autosigning
* [#2609](https://github.com/icinga/icinga2/issues/2609): udp check command is missing arguments.
* [#2604](https://github.com/icinga/icinga2/issues/2604) (CLI): Backup certificate files in 'node setup'
* [#2601](https://github.com/icinga/icinga2/issues/2601) (Configuration): Implement continue/break keywords
* [#2600](https://github.com/icinga/icinga2/issues/2600) (Configuration): Implement support for Json.encode and Json.decode
* [#2591](https://github.com/icinga/icinga2/issues/2591) (Metrics): Add timestamp support for Graphite
* [#2588](https://github.com/icinga/icinga2/issues/2588) (Configuration): Add path information for objects in object list
* [#2578](https://github.com/icinga/icinga2/issues/2578) (Configuration): Implement Array\#join
* [#2553](https://github.com/icinga/icinga2/issues/2553) (Configuration): Implement validator support for function objects
* [#2552](https://github.com/icinga/icinga2/issues/2552) (Configuration): Make operators &&, || behave like in JavaScript
* [#2546](https://github.com/icinga/icinga2/issues/2546): Add macros $host.check\_source$ and $service.check\_source$
* [#2544](https://github.com/icinga/icinga2/issues/2544) (Configuration): Implement the while keyword
* [#2531](https://github.com/icinga/icinga2/issues/2531) (Configuration): Implement keywords to explicitly access globals/locals
* [#2522](https://github.com/icinga/icinga2/issues/2522) (CLI): Make invalid log-severity option output an error instead of a warning
* [#2509](https://github.com/icinga/icinga2/issues/2509): Host/Service runtime macro downtime\_depth
* [#2491](https://github.com/icinga/icinga2/issues/2491) (Configuration): Assignments shouldn't have a "return" value
* [#2488](https://github.com/icinga/icinga2/issues/2488): Implement additional methods for strings
* [#2487](https://github.com/icinga/icinga2/issues/2487) (CLI): Figure out what to do about libreadline \(license\)
* [#2486](https://github.com/icinga/icinga2/issues/2486) (CLI): Figure out a better name for the repl command
* [#2466](https://github.com/icinga/icinga2/issues/2466) (Configuration): Implement line-continuation for the "console" command
* [#2456](https://github.com/icinga/icinga2/issues/2456) (CLI): feature enable should use relative symlinks
* [#2439](https://github.com/icinga/icinga2/issues/2439) (Configuration): Document the new language features in 2.3
* [#2437](https://github.com/icinga/icinga2/issues/2437) (CLI): Implement readline support for the "console" CLI command
* [#2432](https://github.com/icinga/icinga2/issues/2432) (CLI): Backport i2tcl's error reporting functionality into "icinga2 console"
* [#2429](https://github.com/icinga/icinga2/issues/2429) (Configuration): Figure out how variable scopes should work
* [#2426](https://github.com/icinga/icinga2/issues/2426) (Configuration): Implement a way to call methods on objects
* [#2421](https://github.com/icinga/icinga2/issues/2421) (Configuration): Implement a way to remove dictionary keys
* [#2418](https://github.com/icinga/icinga2/issues/2418) (Plugins): Windows plugins should behave like their Linux cousins
* [#2408](https://github.com/icinga/icinga2/issues/2408) (Configuration): ConfigCompiler::HandleInclude should return an inline dictionary
* [#2407](https://github.com/icinga/icinga2/issues/2407) (Configuration): Implement a boolean sub-type for the Value class
* [#2405](https://github.com/icinga/icinga2/issues/2405): Disallow calling strings as functions
* [#2396](https://github.com/icinga/icinga2/issues/2396) (Configuration): Evaluate usage of function\(\)
* [#2391](https://github.com/icinga/icinga2/issues/2391): Improve output of ToString for type objects
* [#2390](https://github.com/icinga/icinga2/issues/2390): Register type objects as global variables
* [#2367](https://github.com/icinga/icinga2/issues/2367) (Configuration): The lexer shouldn't accept escapes for characters which don't have to be escaped
* [#2365](https://github.com/icinga/icinga2/issues/2365) (DB IDO): Implement socket\_path attribute for the IdoMysqlConnection class
* [#2355](https://github.com/icinga/icinga2/issues/2355) (Configuration): Implement official support for user-defined functions and the "for" keyword
* [#2351](https://github.com/icinga/icinga2/issues/2351) (Plugins): Windows agent is missing the standard plugin check\_ping
* [#2348](https://github.com/icinga/icinga2/issues/2348) (Plugins): Plugin Check Commands: Add icmp
* [#2324](https://github.com/icinga/icinga2/issues/2324) (Configuration): Implement the "if" and "else" keywords
* [#2323](https://github.com/icinga/icinga2/issues/2323) (Configuration): Figure out whether Number + String should implicitly convert the Number argument to a string
* [#2322](https://github.com/icinga/icinga2/issues/2322) (Configuration): Make the config parser thread-safe
* [#2318](https://github.com/icinga/icinga2/issues/2318) (Configuration): Implement the % operator
* [#2312](https://github.com/icinga/icinga2/issues/2312): Move the cast functions into libbase
* [#2310](https://github.com/icinga/icinga2/issues/2310) (Configuration): Implement unit tests for the config parser
* [#2304](https://github.com/icinga/icinga2/issues/2304): Implement an option to disable building the Demo component
* [#2303](https://github.com/icinga/icinga2/issues/2303): Implement an option to disable building the Livestatus module
* [#2300](https://github.com/icinga/icinga2/issues/2300) (Notifications): Implement the DISABLE\_HOST\_SVC\_NOTIFICATIONS and ENABLE\_HOST\_SVC\_NOTIFICATIONS commands
* [#2298](https://github.com/icinga/icinga2/issues/2298) (Plugins): Missing check\_disk output on Windows
* [#2294](https://github.com/icinga/icinga2/issues/2294) (Configuration): Implement an AST Expression for T\_CONST
* [#2290](https://github.com/icinga/icinga2/issues/2290): Rename \_DEBUG to I2\_DEBUG
* [#2286](https://github.com/icinga/icinga2/issues/2286) (Configuration): Redesign how stack frames work for scripts
* [#2265](https://github.com/icinga/icinga2/issues/2265): ConfigCompiler::Compile\* should return an AST node
* [#2264](https://github.com/icinga/icinga2/issues/2264) (Configuration): ConfigCompiler::HandleInclude\* should return an AST node
* [#2262](https://github.com/icinga/icinga2/issues/2262) (CLI): Add an option that hides CLI commands
* [#2260](https://github.com/icinga/icinga2/issues/2260) (Configuration): Evaluate apply/object rules when the parent objects are created
* [#2211](https://github.com/icinga/icinga2/issues/2211) (Configuration): Variable from for loop not usable in assign statement
* [#2186](https://github.com/icinga/icinga2/issues/2186) (Configuration): Access object runtime attributes in custom vars & command arguments
* [#2176](https://github.com/icinga/icinga2/issues/2176) (Configuration): Please add labels in SNMP checks
* [#2043](https://github.com/icinga/icinga2/issues/2043) (Livestatus): Livestatus: Add GroupBy tables: hostsbygroup, servicesbygroup, servicesbyhostgroup
* [#2027](https://github.com/icinga/icinga2/issues/2027) (Configuration): Add parent soft states option to Dependency object configuration
* [#2000](https://github.com/icinga/icinga2/issues/2000) (Metrics): Add OpenTSDB Writer
* [#1959](https://github.com/icinga/icinga2/issues/1959) (Configuration): extended Manubulon SNMP Check Plugin Command 
* [#1890](https://github.com/icinga/icinga2/issues/1890) (DB IDO): IDO should fill program\_end\_time on a clean shutdown
* [#1866](https://github.com/icinga/icinga2/issues/1866) (Notifications): Disable flapping detection by default
* [#1859](https://github.com/icinga/icinga2/issues/1859): Run CheckCommands with C locale \(workaround for comma vs dot and plugin api bug\)
* [#1783](https://github.com/icinga/icinga2/issues/1783) (Plugins): Plugin Check Commands: add check\_vmware\_esx
* [#1733](https://github.com/icinga/icinga2/issues/1733) (Configuration): Disallow side-effect-free r-value expressions in expression lists
* [#1507](https://github.com/icinga/icinga2/issues/1507): Don't spawn threads for network connections
* [#404](https://github.com/icinga/icinga2/issues/404) (CLI): Add troubleshooting collect cli command

### Bug

* [#2707](https://github.com/icinga/icinga2/issues/2707) (DB IDO): Crash when using ido-pgsql
* [#2706](https://github.com/icinga/icinga2/issues/2706): Icinga2 shuts down when service is reloaded
* [#2703](https://github.com/icinga/icinga2/issues/2703) (Configuration): Attribute hints don't work for nested attributes
* [#2699](https://github.com/icinga/icinga2/issues/2699) (Configuration): Dependency: Validate \*\_{host,service}\_name objects on their existance
* [#2698](https://github.com/icinga/icinga2/issues/2698) (Livestatus): Improve Livestatus query performance
* [#2697](https://github.com/icinga/icinga2/issues/2697) (Configuration): Memory leak in Expression::GetReference
* [#2695](https://github.com/icinga/icinga2/issues/2695) (Configuration): else if doesn't work without an else branch
* [#2693](https://github.com/icinga/icinga2/issues/2693): Check whether the new TimePeriod validator is working as expected
* [#2692](https://github.com/icinga/icinga2/issues/2692) (CLI): Resource leak in TroubleshootCommand::ObjectInfo
* [#2691](https://github.com/icinga/icinga2/issues/2691) (CLI): Resource leak in TroubleshootCommand::Run
* [#2689](https://github.com/icinga/icinga2/issues/2689): Check if scheduled downtimes work properly
* [#2688](https://github.com/icinga/icinga2/issues/2688) (Plugins): check\_memory tool shows incorrect memory size on windows
* [#2685](https://github.com/icinga/icinga2/issues/2685) (Cluster): Don't accept config updates for zones for which we have an authoritative copy of the config
* [#2684](https://github.com/icinga/icinga2/issues/2684) (Cluster): Icinga crashed on SocketEvent
* [#2683](https://github.com/icinga/icinga2/issues/2683) (Cluster): Crash in ApiClient::TimeoutTimerHandler
* [#2680](https://github.com/icinga/icinga2/issues/2680): Deadlock in TlsStream::Handshake
* [#2679](https://github.com/icinga/icinga2/issues/2679) (Cluster): Deadlock in ApiClient::Disconnect
* [#2677](https://github.com/icinga/icinga2/issues/2677): Crash in SocketEvents::Register
* [#2676](https://github.com/icinga/icinga2/issues/2676) (Livestatus): Windows build fails
* [#2674](https://github.com/icinga/icinga2/issues/2674) (DB IDO): Hosts: process\_performance\_data = 0 in database even though enable\_perfdata = 1 in config
* [#2671](https://github.com/icinga/icinga2/issues/2671) (DB IDO): Crash in DbObject::SendStatusUpdate
* [#2670](https://github.com/icinga/icinga2/issues/2670) (Compat): Valgrind warning for ExternalCommandListener::CommandPipeThread
* [#2669](https://github.com/icinga/icinga2/issues/2669): Crash in ApiEvents::RepositoryTimerHandler
* [#2665](https://github.com/icinga/icinga2/issues/2665) (Livestatus): livestatus limit header not working
* [#2660](https://github.com/icinga/icinga2/issues/2660) (Configuration): apply-for incorrectly converts loop var to string
* [#2659](https://github.com/icinga/icinga2/issues/2659) (Configuration): Config parser fails non-deterministic on Notification missing Checkable
* [#2658](https://github.com/icinga/icinga2/issues/2658) (CLI): Crash in icinga2 console
* [#2654](https://github.com/icinga/icinga2/issues/2654) (DB IDO): Deadlock with DB IDO dump and forcing a scheduled check
* [#2650](https://github.com/icinga/icinga2/issues/2650) (CLI): SIGSEGV in CLI
* [#2647](https://github.com/icinga/icinga2/issues/2647) (DB IDO): Icinga doesn't update long\_output in DB
* [#2646](https://github.com/icinga/icinga2/issues/2646) (Cluster): Misleading ApiListener connection log messages on a master \(Endpoint vs Zone\)
* [#2644](https://github.com/icinga/icinga2/issues/2644) (CLI): Figure out why 'node update-config' becomes slow over time
* [#2642](https://github.com/icinga/icinga2/issues/2642): Icinga 2 sometimes doesn't reconnect to the master
* [#2641](https://github.com/icinga/icinga2/issues/2641) (Cluster): ICINGA process crashes every night
* [#2639](https://github.com/icinga/icinga2/issues/2639) (CLI): Build fails on Debian squeeze
* [#2636](https://github.com/icinga/icinga2/issues/2636): Exception in WorkQueue::StatusTimerHandler
* [#2631](https://github.com/icinga/icinga2/issues/2631) (Cluster): deadlock in client connection
* [#2630](https://github.com/icinga/icinga2/issues/2630) (Cluster): Don't request heartbeat messages until after we've synced the log
* [#2627](https://github.com/icinga/icinga2/issues/2627) (Livestatus): Livestatus query on commands table with custom vars fails
* [#2626](https://github.com/icinga/icinga2/issues/2626) (DB IDO): Icinga2 segfaults when issuing postgresql queries
* [#2622](https://github.com/icinga/icinga2/issues/2622): "node wizard" crashes
* [#2621](https://github.com/icinga/icinga2/issues/2621): Don't attempt to restore program state from non-existing state file
* [#2618](https://github.com/icinga/icinga2/issues/2618) (DB IDO): DB IDO {host,service}checks command\_line value is "Object of type 'icinga::Array'"
* [#2617](https://github.com/icinga/icinga2/issues/2617) (DB IDO): Indicate that Icinga2 is shutting down in case of a fatal error
* [#2615](https://github.com/icinga/icinga2/issues/2615): Make the arguments for the stats functions const-ref
* [#2613](https://github.com/icinga/icinga2/issues/2613) (DB IDO): DB IDO: Duplicate entry icinga\_scheduleddowntime
* [#2608](https://github.com/icinga/icinga2/issues/2608) (Plugins): Ignore the -X option for check\_disk on Windows
* [#2605](https://github.com/icinga/icinga2/issues/2605): Compiler warnings
* [#2599](https://github.com/icinga/icinga2/issues/2599) (Cluster): Agent writes CR CR LF in synchronized config files
* [#2598](https://github.com/icinga/icinga2/issues/2598): Added downtimes must be triggered immediately if checkable is Not-OK
* [#2597](https://github.com/icinga/icinga2/issues/2597) (Cluster): Config sync authoritative file never created
* [#2596](https://github.com/icinga/icinga2/issues/2596) (Compat): StatusDataWriter: Wrong host notification filters \(broken fix in \#8192\)
* [#2593](https://github.com/icinga/icinga2/issues/2593) (Compat): last\_hard\_state missing in StatusDataWriter
* [#2589](https://github.com/icinga/icinga2/issues/2589) (Configuration): Stacktrace on Endpoint not belonging to a zone or multiple zones
* [#2586](https://github.com/icinga/icinga2/issues/2586): Icinga2 master doesn't change check-status when "accept\_commands = true" is not set at client node
* [#2579](https://github.com/icinga/icinga2/issues/2579) (Configuration): Apply rule '' for host does not match anywhere!
* [#2572](https://github.com/icinga/icinga2/issues/2572) (Cluster): Incorrectly formatted timestamp in .timestamp file
* [#2570](https://github.com/icinga/icinga2/issues/2570): Crash in ScheduledDowntime::CreateNextDowntime
* [#2569](https://github.com/icinga/icinga2/issues/2569): PidPath, VarsPath, ObjectsPath and StatePath no longer read from init.conf
* [#2566](https://github.com/icinga/icinga2/issues/2566) (Configuration): Don't allow comparison of strings and numbers
* [#2562](https://github.com/icinga/icinga2/issues/2562) (Cluster): ApiListener::ReplayLog shouldn't hold mutex lock during call to Socket::Poll
* [#2560](https://github.com/icinga/icinga2/issues/2560): notify flag is ignored in ACKNOWLEDGE\_\*\_PROBLEM commands
* [#2559](https://github.com/icinga/icinga2/issues/2559) (DB IDO): Duplicate entry on icinga\_hoststatus
* [#2556](https://github.com/icinga/icinga2/issues/2556) (CLI): Running icinga2 command as non privilged user raises error
* [#2551](https://github.com/icinga/icinga2/issues/2551) (Livestatus): Livestatus operator =~ is not case-insensitive
* [#2542](https://github.com/icinga/icinga2/issues/2542) (CLI): icinga2 node wizard: Create backups of certificates
* [#2539](https://github.com/icinga/icinga2/issues/2539) (Cluster): Report missing command objects on remote agent
* [#2533](https://github.com/icinga/icinga2/issues/2533) (Cluster): Problems using command\_endpoint inside HA zone
* [#2529](https://github.com/icinga/icinga2/issues/2529) (CLI): CLI console fails to report errors in included files
* [#2526](https://github.com/icinga/icinga2/issues/2526) (Configuration): Deadlock when accessing loop variable inside of the loop
* [#2525](https://github.com/icinga/icinga2/issues/2525) (Configuration): Lexer term for T\_ANGLE\_STRING is too aggressive
* [#2513](https://github.com/icinga/icinga2/issues/2513) (CLI): icinga2 node update should not write config for blacklisted zones/host
* [#2503](https://github.com/icinga/icinga2/issues/2503) (CLI): Argument auto-completion doesn't work for short options
* [#2502](https://github.com/icinga/icinga2/issues/2502): group assign fails with bad lexical cast when evaluating rules
* [#2497](https://github.com/icinga/icinga2/issues/2497): Exception on missing config files
* [#2494](https://github.com/icinga/icinga2/issues/2494) (Livestatus): Error messages when stopping Icinga
* [#2493](https://github.com/icinga/icinga2/issues/2493): Compiler warnings
* [#2492](https://github.com/icinga/icinga2/issues/2492): Segfault on icinga::String::operator= when compiling configuration
* [#2485](https://github.com/icinga/icinga2/issues/2485) (Configuration): parsing include\_recursive
* [#2482](https://github.com/icinga/icinga2/issues/2482) (Configuration): escaped backslash in string literals
* [#2467](https://github.com/icinga/icinga2/issues/2467) (CLI): Icinga crashes when config file name is invalid
* [#2465](https://github.com/icinga/icinga2/issues/2465) (Configuration): Debug info for indexer is incorrect
* [#2457](https://github.com/icinga/icinga2/issues/2457): Config file passing validation causes segfault
* [#2452](https://github.com/icinga/icinga2/issues/2452) (Cluster): Agent checks fail when there's already a host with the same name
* [#2448](https://github.com/icinga/icinga2/issues/2448) (Configuration): User::ValidateFilters isn't being used
* [#2447](https://github.com/icinga/icinga2/issues/2447) (Configuration): ConfigCompilerContext::WriteObject crashes after ConfigCompilerContext::FinishObjectsFile was called
* [#2445](https://github.com/icinga/icinga2/issues/2445) (Configuration): segfault on startup
* [#2442](https://github.com/icinga/icinga2/issues/2442) (DB IDO): POSTGRES IDO: invalid syntax for integer: "true" while trying to update table icinga\_hoststatus
* [#2441](https://github.com/icinga/icinga2/issues/2441) (CLI): console: Don't repeat line when we're reporting an error for the last line
* [#2436](https://github.com/icinga/icinga2/issues/2436) (Configuration): Modulo 0 crashes Icinga
* [#2435](https://github.com/icinga/icinga2/issues/2435) (Configuration): Location info for strings is incorrect
* [#2434](https://github.com/icinga/icinga2/issues/2434) (Configuration): Setting an attribute on an r-value fails
* [#2433](https://github.com/icinga/icinga2/issues/2433) (Configuration): Confusing error message when trying to set a field on a string
* [#2431](https://github.com/icinga/icinga2/issues/2431) (Configuration): icinga 2 Config Error needs to be more verbose
* [#2428](https://github.com/icinga/icinga2/issues/2428) (Configuration): Debug visualizer for the Value class is broken
* [#2427](https://github.com/icinga/icinga2/issues/2427) (Configuration): if doesn't work for non-boolean arguments
* [#2423](https://github.com/icinga/icinga2/issues/2423) (Configuration): Require at least one user for notification objects \(user or as member of user\_groups\)
* [#2419](https://github.com/icinga/icinga2/issues/2419) (Configuration): Confusing error message for import
* [#2410](https://github.com/icinga/icinga2/issues/2410): The Boolean type change broke set\_if
* [#2406](https://github.com/icinga/icinga2/issues/2406) (Configuration): len\(\) overflows
* [#2395](https://github.com/icinga/icinga2/issues/2395) (Configuration): operator precedence for % and \> is incorrect
* [#2388](https://github.com/icinga/icinga2/issues/2388): Value\(""\).IsEmpty\(\) should return true
* [#2379](https://github.com/icinga/icinga2/issues/2379) (Cluster): Windows Agent: Missing directory "zones" in setup
* [#2375](https://github.com/icinga/icinga2/issues/2375) (Configuration): Config validator doesn't show in which file the error was found
* [#2362](https://github.com/icinga/icinga2/issues/2362): Serialize\(\) fails to serialize objects which don't have a registered type
* [#2361](https://github.com/icinga/icinga2/issues/2361): Fix warnings when using CMake 3.1.0
* [#2346](https://github.com/icinga/icinga2/issues/2346) (DB IDO): Missing persistent\_comment, notify\_contact columns for acknowledgement table
* [#2329](https://github.com/icinga/icinga2/issues/2329) (Configuration): - shouldn't be allowed in identifiers
* [#2326](https://github.com/icinga/icinga2/issues/2326): Compiler warnings
* [#2320](https://github.com/icinga/icinga2/issues/2320) (Configuration): - operator doesn't work in expressions
* [#2319](https://github.com/icinga/icinga2/issues/2319) (Configuration): Set expression should check whether LHS is a null pointer
* [#2317](https://github.com/icinga/icinga2/issues/2317) (Configuration): Validate array subscripts
* [#2316](https://github.com/icinga/icinga2/issues/2316) (Configuration): The \_\_return keyword is broken
* [#2315](https://github.com/icinga/icinga2/issues/2315) (Configuration): Return values for functions are broken
* [#2314](https://github.com/icinga/icinga2/issues/2314): Scoping rules for "for" are broken
* [#2313](https://github.com/icinga/icinga2/issues/2313) (Configuration): Unterminated string literals should cause parser to return an error
* [#2308](https://github.com/icinga/icinga2/issues/2308) (Configuration): Change parameter type for include and include\_recursive to T\_STRING
* [#2307](https://github.com/icinga/icinga2/issues/2307) (Configuration): Fix the shift/reduce conflicts in the parser
* [#2289](https://github.com/icinga/icinga2/issues/2289) (DB IDO): DB IDO: Duplicate entry icinga\_{host,service}dependencies
* [#2274](https://github.com/icinga/icinga2/issues/2274) (Notifications): Reminder notifications not being sent but logged every 5 secs
* [#2234](https://github.com/icinga/icinga2/issues/2234): Avoid rebuilding libbase when the version number changes
* [#2232](https://github.com/icinga/icinga2/issues/2232): Unity build doesn't work with MSVC
* [#2194](https://github.com/icinga/icinga2/issues/2194) (Configuration): validate configured legacy timeperiod ranges
* [#2174](https://github.com/icinga/icinga2/issues/2174) (Configuration): Update validators for CustomVarObject
* [#2020](https://github.com/icinga/icinga2/issues/2020) (Configuration): Invalid macro results in exception
* [#1899](https://github.com/icinga/icinga2/issues/1899): Scheduled start time will be ignored if the host or service is already in a problem state
* [#1530](https://github.com/icinga/icinga2/issues/1530): Remove name and return value for stats functions

### ITL

* [#2705](https://github.com/icinga/icinga2/issues/2705) (ITL): Add check commands for NSClient++
* [#2661](https://github.com/icinga/icinga2/issues/2661) (ITL): ITL: The procs check command uses spaces instead of tabs
* [#2652](https://github.com/icinga/icinga2/issues/2652) (ITL): Rename PluginsContribDir to PluginContribDir
* [#2649](https://github.com/icinga/icinga2/issues/2649) (ITL): Snmp CheckCommand misses various options
* [#2614](https://github.com/icinga/icinga2/issues/2614) (ITL): add webinject checkcommand
* [#2610](https://github.com/icinga/icinga2/issues/2610) (ITL): Add ITL check command for check\_ipmi\_sensor
* [#2573](https://github.com/icinga/icinga2/issues/2573) (ITL): Extend disk checkcommand
* [#2541](https://github.com/icinga/icinga2/issues/2541) (ITL): The check "hostalive" is not working with ipv6
* [#2012](https://github.com/icinga/icinga2/issues/2012) (ITL): ITL: ESXi-Hardware
* [#2011](https://github.com/icinga/icinga2/issues/2011) (ITL): ITL: Check\_Mem.pl
* [#1984](https://github.com/icinga/icinga2/issues/1984) (ITL): ITL: Interfacetable

### Documentation

* [#2711](https://github.com/icinga/icinga2/issues/2711) (Documentation): Document closures \('use'\)
* [#2709](https://github.com/icinga/icinga2/issues/2709) (Documentation): Fix a typo in documentation
* [#2662](https://github.com/icinga/icinga2/issues/2662) (Documentation): Update Remote Client/Distributed Monitoring Documentation
* [#2595](https://github.com/icinga/icinga2/issues/2595) (Documentation): Add documentation for cli command 'console'
* [#2575](https://github.com/icinga/icinga2/issues/2575) (Documentation): Remote Clients: Add manual setup cli commands
* [#2555](https://github.com/icinga/icinga2/issues/2555) (Documentation): The Zone::global attribute is not documented
* [#2399](https://github.com/icinga/icinga2/issues/2399) (Documentation): Allow name changed from inside the object
* [#2387](https://github.com/icinga/icinga2/issues/2387) (Documentation): Documentation enhancement for snmp traps and passive checks.
* [#2321](https://github.com/icinga/icinga2/issues/2321) (Documentation): Document operator precedence
* [#2198](https://github.com/icinga/icinga2/issues/2198) (Documentation): Variable expansion is single quoted.
* [#1860](https://github.com/icinga/icinga2/issues/1860) (Documentation): Add some more PNP details

### Support

* [#2616](https://github.com/icinga/icinga2/issues/2616) (Installation): Build fails on OpenBSD
* [#2602](https://github.com/icinga/icinga2/issues/2602) (Packages): Icinga2 config reset after package update \(centos6.6\)
* [#2511](https://github.com/icinga/icinga2/issues/2511) (Packages): '../features-available/checker.conf' does not exist \[Windows\]
* [#2374](https://github.com/icinga/icinga2/issues/2374) (Packages): Move the config file for the ido-\*sql features into the icinga2-ido-\* packages
* [#2302](https://github.com/icinga/icinga2/issues/2302) (Installation): Don't build db\_ido when both MySQL and PostgreSQL aren't enabled

## 2.2.4 (2015-02-05)

### Notes

* Bugfixes

### Bug

* [#2587](https://github.com/icinga/icinga2/issues/2587) (CLI): Output in "node wizard" is confusing
* [#2577](https://github.com/icinga/icinga2/issues/2577) (Compat): enable\_event\_handlers attribute is missing in status.dat
* [#2571](https://github.com/icinga/icinga2/issues/2571): Segfault in Checkable::AddNotification
* [#2561](https://github.com/icinga/icinga2/issues/2561): Scheduling downtime for host and all services only schedules services
* [#2558](https://github.com/icinga/icinga2/issues/2558) (CLI): Restart of Icinga hangs
* [#2550](https://github.com/icinga/icinga2/issues/2550) (DB IDO): Crash in DbConnection::ProgramStatusHandler
* [#2538](https://github.com/icinga/icinga2/issues/2538) (CLI): Restart fails after deleting a Host
* [#2508](https://github.com/icinga/icinga2/issues/2508) (Compat): Feature statusdata shows wrong host notification options
* [#2481](https://github.com/icinga/icinga2/issues/2481) (CLI): Satellite doesn't use manually supplied 'local zone name'
* [#2464](https://github.com/icinga/icinga2/issues/2464): vfork\(\) hangs on OS X
* [#2256](https://github.com/icinga/icinga2/issues/2256) (Notifications): kUn-Bashify mail-{host,service}-notification.sh
* [#2242](https://github.com/icinga/icinga2/issues/2242): livestatus / nsca / etc submits are ignored during reload
* [#1893](https://github.com/icinga/icinga2/issues/1893): Configured recurring downtimes not applied on saturdays

### ITL

* [#2532](https://github.com/icinga/icinga2/issues/2532) (ITL): check\_ssmtp command does NOT support mail\_from

### Documentation

* [#2521](https://github.com/icinga/icinga2/issues/2521) (Documentation): Typos in readme file for windows plugins
* [#2520](https://github.com/icinga/icinga2/issues/2520) (Documentation): inconsistent URL http\(s\)://www.icinga.org
* [#2512](https://github.com/icinga/icinga2/issues/2512) (Documentation): Update Icinga Web 2 uri to /icingaweb2

### Support

* [#2517](https://github.com/icinga/icinga2/issues/2517) (Packages): Fix YAJL detection on Debian squeeze
* [#2462](https://github.com/icinga/icinga2/issues/2462) (Packages): Icinga 2.2.2 build fails on SLES11SP3 because of changed boost dependency

## 2.2.3 (2015-01-12)

### Notes

* Bugfixes

### Bug

* [#2499](https://github.com/icinga/icinga2/issues/2499) (CLI): Segfault on update-config old empty config
* [#2498](https://github.com/icinga/icinga2/issues/2498) (CLI): icinga2 node update config shows hex instead of human readable names
* [#2496](https://github.com/icinga/icinga2/issues/2496): Icinga 2.2.2 segfaults on FreeBSD
* [#2477](https://github.com/icinga/icinga2/issues/2477): DB IDO query queue limit reached on reload
* [#2473](https://github.com/icinga/icinga2/issues/2473) (CLI): check\_interval must be greater than 0 error on update-config
* [#2471](https://github.com/icinga/icinga2/issues/2471) (Cluster): Arguments without values are not used on plugin exec
* [#2470](https://github.com/icinga/icinga2/issues/2470) (Plugins): Windows plugin check\_service.exe can't find service NTDS
* [#2459](https://github.com/icinga/icinga2/issues/2459) (CLI): Incorrect ticket shouldn't cause "node wizard" to terminate
* [#2420](https://github.com/icinga/icinga2/issues/2420) (Notifications): Volatile checks trigger invalid notifications on OK-\>OK state changes

### Documentation

* [#2490](https://github.com/icinga/icinga2/issues/2490) (Documentation): Typo in example of StatusDataWriter

### Support

* [#2460](https://github.com/icinga/icinga2/issues/2460) (Packages): Icinga 2.2.2 doesn't build on i586 SUSE distributions

## 2.2.2 (2014-12-18)

### Notes

* Bugfixes

### Bug

* [#2446](https://github.com/icinga/icinga2/issues/2446) (Compat): StatusDataWriter: Wrong export of event\_handler\_enabled
* [#2444](https://github.com/icinga/icinga2/issues/2444) (CLI): Remove usage info from --version
* [#2416](https://github.com/icinga/icinga2/issues/2416) (DB IDO): DB IDO: Missing last\_hard\_state column update in {host,service}status tables
* [#2411](https://github.com/icinga/icinga2/issues/2411): exception during config check
* [#2394](https://github.com/icinga/icinga2/issues/2394): typeof does not work for numbers
* [#2381](https://github.com/icinga/icinga2/issues/2381): SIGABRT while evaluating apply rules
* [#2380](https://github.com/icinga/icinga2/issues/2380) (Configuration): typeof\(\) seems to return null for arrays and dictionaries
* [#2376](https://github.com/icinga/icinga2/issues/2376) (Configuration): Apache 2.2 fails with new apache conf
* [#2371](https://github.com/icinga/icinga2/issues/2371) (Configuration): Test Classic UI config file with Apache 2.4
* [#2370](https://github.com/icinga/icinga2/issues/2370) (Cluster): update\_config not updating configuration
* [#2360](https://github.com/icinga/icinga2/issues/2360): CLI `icinga2 node update-config` doesn't sync configs from remote clients as expected
* [#2354](https://github.com/icinga/icinga2/issues/2354) (DB IDO): Improve error reporting when libmysqlclient or libpq are missing
* [#2350](https://github.com/icinga/icinga2/issues/2350) (Cluster): Segfault on issuing node update-config
* [#2341](https://github.com/icinga/icinga2/issues/2341) (Cluster): execute checks locally if command\_endpoint == local endpoint
* [#2283](https://github.com/icinga/icinga2/issues/2283) (Cluster): Cluster heartbeats need to be more aggressive
* [#2266](https://github.com/icinga/icinga2/issues/2266) (CLI): "node wizard" shouldn't crash when SaveCert fails
* [#2255](https://github.com/icinga/icinga2/issues/2255) (DB IDO): If a parent host goes down, the child host isn't marked as unrechable in the db ido
* [#2216](https://github.com/icinga/icinga2/issues/2216) (Cluster): Repository does not support services which have a slash in their name
* [#2202](https://github.com/icinga/icinga2/issues/2202) (Configuration): CPU usage at 100% when check\_interval = 0 in host object definition 
* [#2154](https://github.com/icinga/icinga2/issues/2154) (Cluster): update-config fails to create hosts
* [#2148](https://github.com/icinga/icinga2/issues/2148) (Compat): Feature `compatlog' should flush output buffer on every new line
* [#2021](https://github.com/icinga/icinga2/issues/2021): double macros in command arguments seems to lead to exception
* [#2016](https://github.com/icinga/icinga2/issues/2016) (Notifications): Docs: Better explaination of dependency state filters
* [#1947](https://github.com/icinga/icinga2/issues/1947) (Livestatus): Missing host downtimes/comments in Livestatus

### ITL

* [#2430](https://github.com/icinga/icinga2/issues/2430) (ITL): No option to specify timeout to check\_snmp and snmp manubulon commands

### Documentation

* [#2422](https://github.com/icinga/icinga2/issues/2422) (Documentation): Setting a dictionary key to null does not cause the key/value to be removed
* [#2412](https://github.com/icinga/icinga2/issues/2412) (Documentation): Update host examples in Dependencies for Network Reachability documentation
* [#2409](https://github.com/icinga/icinga2/issues/2409) (Documentation): Wrong command in documentation for installing Icinga 2 pretty printers.
* [#2404](https://github.com/icinga/icinga2/issues/2404) (Documentation): Livestatus: Replace unixcat with nc -U 
* [#2180](https://github.com/icinga/icinga2/issues/2180) (Documentation): Documentation: Add note on default notification interval in getting started notifications.conf

### Support

* [#2417](https://github.com/icinga/icinga2/issues/2417) (Tests): Unit tests fail on FreeBSD
* [#2369](https://github.com/icinga/icinga2/issues/2369) (Packages): SUSE packages %set\_permissions post statement wasn't moved to common
* [#2368](https://github.com/icinga/icinga2/issues/2368) (Packages): /usr/lib/icinga2 is not owned by a package
* [#2292](https://github.com/icinga/icinga2/issues/2292) (Tests): The unit tests still crash sometimes
* [#1942](https://github.com/icinga/icinga2/issues/1942) (Packages): icinga2 init-script doesn't validate configuration on reload action

## 2.2.1 (2014-12-01)

### Notes

* Support arrays in [command argument macros](#command-passing-parameters) #6709
    * Allows to define multiple parameters for [nrpe -a](#plugin-check-command-nrpe), [nscp -l](#plugin-check-command-nscp), [disk -p](#plugin-check-command-disk), [dns -a](#plugin-check-command-dns).
* Bugfixes

### Enhancement

* [#2366](https://github.com/icinga/icinga2/issues/2366): Release 2.2.1
* [#2277](https://github.com/icinga/icinga2/issues/2277) (Configuration): The classicui Apache conf doesn't support Apache 2.4
* [#1790](https://github.com/icinga/icinga2/issues/1790): Support for arrays in macros

### Bug

* [#2340](https://github.com/icinga/icinga2/issues/2340) (CLI): Segfault in CA handling
* [#2328](https://github.com/icinga/icinga2/issues/2328) (Cluster): Verify if master radio box is disabled in the Windows wizard
* [#2311](https://github.com/icinga/icinga2/issues/2311) (Configuration): !in operator returns incorrect result
* [#2293](https://github.com/icinga/icinga2/issues/2293) (Configuration): Objects created with node update-config can't be seen in Classic UI
* [#2288](https://github.com/icinga/icinga2/issues/2288) (Cluster): Incorrect error message for localhost
* [#2282](https://github.com/icinga/icinga2/issues/2282) (Cluster): Icinga2 node add failed with unhandled exception
* [#2273](https://github.com/icinga/icinga2/issues/2273): Restart Icinga - Error Restoring program state from file '/var/lib/icinga2/icinga2.state'
* [#2272](https://github.com/icinga/icinga2/issues/2272) (Cluster): Windows wizard is missing --zone argument
* [#2271](https://github.com/icinga/icinga2/issues/2271) (Cluster): Windows wizard uses incorrect CLI command
* [#2267](https://github.com/icinga/icinga2/issues/2267) (Cluster): Built-in commands shouldn't be run on the master instance in remote command execution mode
* [#2207](https://github.com/icinga/icinga2/issues/2207) (Livestatus): livestatus large amount of submitting unix socket command results in broken pipes

### ITL

* [#2285](https://github.com/icinga/icinga2/issues/2285) (ITL): Increase default timeout for NRPE checks

### Documentation

* [#2344](https://github.com/icinga/icinga2/issues/2344) (Documentation): Documentation: Explain how unresolved macros are handled
* [#2343](https://github.com/icinga/icinga2/issues/2343) (Documentation): Document how arrays in macros work
* [#2336](https://github.com/icinga/icinga2/issues/2336) (Documentation): Wrong information in section "Linux Client Setup Wizard for Remote Monitoring"
* [#2275](https://github.com/icinga/icinga2/issues/2275) (Documentation): 2.2.0 has out-of-date icinga2 man page
* [#2251](https://github.com/icinga/icinga2/issues/2251) (Documentation): object and template with the same name generate duplicate object error

### Support

* [#2363](https://github.com/icinga/icinga2/issues/2363) (Packages): Fix Apache config in the Debian package
* [#2359](https://github.com/icinga/icinga2/issues/2359) (Packages): Wrong permission in run directory after restart
* [#2301](https://github.com/icinga/icinga2/issues/2301) (Packages): Move the icinga2-prepare-dirs script elsewhere
* [#2280](https://github.com/icinga/icinga2/issues/2280) (Packages): Icinga 2.2 misses the build requirement libyajl-devel for SUSE distributions
* [#2278](https://github.com/icinga/icinga2/issues/2278) (Packages): /usr/sbin/icinga-prepare-dirs conflicts in the bin and common package
* [#2276](https://github.com/icinga/icinga2/issues/2276) (Packages): Systemd rpm scripts are run in wrong package
* [#2212](https://github.com/icinga/icinga2/issues/2212) (Packages): icinga2 checkconfig should fail if group given for command files does not exist
* [#2117](https://github.com/icinga/icinga2/issues/2117) (Packages): Update spec file to use yajl-devel
* [#1968](https://github.com/icinga/icinga2/issues/1968) (Packages): service icinga2 status gives wrong information when run as unprivileged user

## 2.2.0 (2014-11-17)

### Notes

* DB IDO schema update to version `1.12.0`
    * schema files in `lib/db_ido_{mysql,pgsql}/schema` (source)
    * Table `programstatus`: New column `program_version`
    * Table `customvariables` and `customvariablestatus`: New column `is_json` (required for custom attribute array/dictionary support)
* New features
    * [GelfWriter](#gelfwriter): Logging check results, state changes, notifications to GELF (graylog2, logstash) #7619
    * Agent/Client/Node framework #7249
    * Windows plugins for the client/agent parts #7242 #7243
* New CLI commands #7245
    * `icinga2 feature {enable,disable}` replaces `icinga2-{enable,disable}-feature` script  #7250
    * `icinga2 object list` replaces `icinga2-list-objects` script  #7251
    * `icinga2 pki` replaces` icinga2-build-{ca,key}` scripts  #7247
    * `icinga2 repository` manages `/etc/icinga2/repository.d` which must be included in `icinga2.conf` #7255
    * `icinga2 node` cli command provides node (master, satellite, agent) setup (wizard) and management functionality #7248
    * `icinga2 daemon` for existing daemon arguments (`-c`, `-C`). Removed `-u` and `-g` parameters in favor of [init.conf](#init-conf).
    * bash auto-completion & terminal colors #7396
* Configuration
    * Former `localhost` example host is now defined in [hosts.conf](#hosts-conf) #7594
    * All example services moved into advanced apply rules in [services.conf](#services-conf)
    * Updated downtimes configuration example in [downtimes.conf](#downtimes-conf) #7472
    * Updated notification apply example in [notifications.conf](#notifications-conf) #7594
    * Support for object attribute 'zone' #7400
    * Support setting [object variables in apply rules](#dependencies-apply-custom-attributes) #7479
    * Support arrays and dictionaries in [custom attributes](#custom-attributes-apply) #6544 #7560
    * Add [apply for rules](#using-apply-for) for advanced dynamic object generation #7561
    * New attribute `accept_commands` for [ApiListener](#objecttype-apilistener) #7559
    * New [init.conf](#init-conf) file included first containing new constants `RunAsUser` and `RunAsGroup`.
* Cluster
    * Add [CSR Auto-Signing support](#csr-autosigning-requirements) using generated ticket #7244
    * Allow to [execute remote commands](#icinga2-remote-monitoring-client-command-execution) on endpoint clients #7559
* Perfdata
    * [PerfdataWriter](#writing-performance-data-files): Don't change perfdata, pass through from plugins #7268
    * [GraphiteWriter](#graphite-carbon-cache-writer): Add warn/crit/min/max perfdata and downtime_depth stats values #7366 #6946
* Packages
    * `python-icinga2` package dropped in favor of integrated cli commands #7245
    * Windows Installer for the agent parts #7243

> **Note**
>
>  Please remove `conf.d/hosts/localhost*` after verifying your updated configuration!

### Enhancement

* [#2219](https://github.com/icinga/icinga2/issues/2219): Icinga 2 should use less RAM
* [#2217](https://github.com/icinga/icinga2/issues/2217) (Metrics): Add GelfWriter for writing log events to graylog2/logstash
* [#2213](https://github.com/icinga/icinga2/issues/2213): Optimize class layout
* [#2203](https://github.com/icinga/icinga2/issues/2203) (Configuration): Revamp sample configuration: add NodeName host, move services into apply rules schema
* [#2189](https://github.com/icinga/icinga2/issues/2189) (Configuration): Refactor AST into multiple classes
* [#2187](https://github.com/icinga/icinga2/issues/2187) (Configuration): Implement support for arbitrarily complex indexers
* [#2184](https://github.com/icinga/icinga2/issues/2184) (Configuration): Generate objects using apply with foreach in arrays or dictionaries \(key =\> value\)
* [#2183](https://github.com/icinga/icinga2/issues/2183) (Configuration): Support dictionaries in custom attributes
* [#2182](https://github.com/icinga/icinga2/issues/2182) (Cluster): Execute remote commands on the agent w/o local objects by passing custom attributes
* [#2179](https://github.com/icinga/icinga2/issues/2179): Implement keys\(\)
* [#2178](https://github.com/icinga/icinga2/issues/2178) (CLI): Cli command Node: Disable notifications feature on client nodes
* [#2161](https://github.com/icinga/icinga2/issues/2161) (CLI): Cli Command: Rename 'agent' to 'node'
* [#2158](https://github.com/icinga/icinga2/issues/2158) (Cluster): Require --zone to be specified for "node setup"
* [#2152](https://github.com/icinga/icinga2/issues/2152) (Cluster): Rename --agent to --zone \(for blacklist/whitelist\)
* [#2140](https://github.com/icinga/icinga2/issues/2140) (CLI): Cli: Use Node Blacklist functionality in 'node update-config'
* [#2138](https://github.com/icinga/icinga2/issues/2138) (CLI): Find a better name for 'repository commit --clear'
* [#2131](https://github.com/icinga/icinga2/issues/2131) (Configuration): Set host/service variable in apply rules
* [#2124](https://github.com/icinga/icinga2/issues/2124) (Configuration): Update downtimes.conf example config
* [#2119](https://github.com/icinga/icinga2/issues/2119) (Cluster): Remove virtual agent name feature for localhost
* [#2118](https://github.com/icinga/icinga2/issues/2118) (CLI): Cli command: Node Setup Wizard \(for Satellites and Agents\)
* [#2115](https://github.com/icinga/icinga2/issues/2115) (CLI): Cli command: Repository remove host should remove host.conf host/ dir with services
* [#2113](https://github.com/icinga/icinga2/issues/2113) (CLI): validate repository config updates
* [#2108](https://github.com/icinga/icinga2/issues/2108): Only build YAJL when there's no system-provided version available
* [#2107](https://github.com/icinga/icinga2/issues/2107): Replace cJSON with a better JSON parser
* [#2104](https://github.com/icinga/icinga2/issues/2104) (CLI): Use "variable get" for "pki ticket"
* [#2103](https://github.com/icinga/icinga2/issues/2103) (CLI): Validate number of arguments
* [#2098](https://github.com/icinga/icinga2/issues/2098) (CLI): Support for writing api.conf
* [#2096](https://github.com/icinga/icinga2/issues/2096) (CLI): Cli command: pki needs option to define the algorithm
* [#2092](https://github.com/icinga/icinga2/issues/2092) (CLI): Rename PKI arguments
* [#2088](https://github.com/icinga/icinga2/issues/2088) (CLI): Cli command: Node Setup
* [#2087](https://github.com/icinga/icinga2/issues/2087) (CLI): "pki request" should ask user to verify the peer's certificate
* [#2086](https://github.com/icinga/icinga2/issues/2086) (CLI): Add -h next to --help
* [#2085](https://github.com/icinga/icinga2/issues/2085) (CLI): Remove "available features" list from "feature list"
* [#2084](https://github.com/icinga/icinga2/issues/2084) (CLI): Implement "feature disable" for Windows
* [#2081](https://github.com/icinga/icinga2/issues/2081) (CLI): CLI: List disabled features in feature list too
* [#2079](https://github.com/icinga/icinga2/issues/2079): Move WSAStartup call to INITIALIZE\_ONCE
* [#2076](https://github.com/icinga/icinga2/issues/2076) (CLI): Implement field attribute to hide fields in command auto-completion
* [#2074](https://github.com/icinga/icinga2/issues/2074) (CLI): Add autocomplete to 'host/service add' for object attributes \(e.g. --check\_interval\)
* [#2073](https://github.com/icinga/icinga2/issues/2073) (Configuration): Remove zone keyword and allow to use object attribute 'zone'
* [#2071](https://github.com/icinga/icinga2/issues/2071) (Configuration): Move localhost config into repository
* [#2069](https://github.com/icinga/icinga2/issues/2069) (CLI): Implement generic color support for terminals
* [#2066](https://github.com/icinga/icinga2/issues/2066) (CLI): Implement support for serial files
* [#2064](https://github.com/icinga/icinga2/issues/2064) (DB IDO): Add program\_version column to programstatus table
* [#2062](https://github.com/icinga/icinga2/issues/2062): Release 2.2
* [#2059](https://github.com/icinga/icinga2/issues/2059) (CLI): Auto-completion for feature enable/disable
* [#2055](https://github.com/icinga/icinga2/issues/2055) (CLI): Windows support for cli command feature
* [#2054](https://github.com/icinga/icinga2/issues/2054) (CLI): CLI Commands: Remove timestamp prefix when logging output
* [#2053](https://github.com/icinga/icinga2/issues/2053) (CLI): autocomplete should support '--key value'
* [#2050](https://github.com/icinga/icinga2/issues/2050) (CLI): Cli command parser must support unregistered boost::program\_options
* [#2049](https://github.com/icinga/icinga2/issues/2049) (CLI): CLI command: variable
* [#2046](https://github.com/icinga/icinga2/issues/2046) (Graphite): GraphiteWriter: Add warn/crit/min/max perfdata values if existing
* [#2031](https://github.com/icinga/icinga2/issues/2031) (Graphite): GraphiteWriter: Add support for customized metric prefix names
* [#2003](https://github.com/icinga/icinga2/issues/2003): macro processor needs an array printer
* [#1999](https://github.com/icinga/icinga2/issues/1999) (CLI): Cli command: Repository
* [#1997](https://github.com/icinga/icinga2/issues/1997) (CLI): Cli Commands: Node Repository Blacklist & Whitelist
* [#1996](https://github.com/icinga/icinga2/issues/1996) (CLI): Cli command: SCM
* [#1995](https://github.com/icinga/icinga2/issues/1995) (CLI): Cli command: Object
* [#1994](https://github.com/icinga/icinga2/issues/1994) (CLI): Cli command: Feature
* [#1993](https://github.com/icinga/icinga2/issues/1993) (CLI): Node Repository
* [#1992](https://github.com/icinga/icinga2/issues/1992) (CLI): Cli command: Node
* [#1991](https://github.com/icinga/icinga2/issues/1991) (CLI): Cli command: pki
* [#1990](https://github.com/icinga/icinga2/issues/1990) (CLI): Cli command framework
* [#1989](https://github.com/icinga/icinga2/issues/1989) (CLI): Cli commands
* [#1988](https://github.com/icinga/icinga2/issues/1988) (Cluster): CSR auto-signing
* [#1987](https://github.com/icinga/icinga2/issues/1987) (Plugins): Windows plugins
* [#1986](https://github.com/icinga/icinga2/issues/1986) (Cluster): Windows Wizard
* [#1977](https://github.com/icinga/icinga2/issues/1977) (CLI): Cli commands: add filter capability to 'object list'
* [#1901](https://github.com/icinga/icinga2/issues/1901) (Cluster): Windows installer
* [#1895](https://github.com/icinga/icinga2/issues/1895) (Graphite): Add downtime depth as statistic metric for GraphiteWriter
* [#1717](https://github.com/icinga/icinga2/issues/1717) (Configuration): Support for array in custom variable.
* [#894](https://github.com/icinga/icinga2/issues/894): Add copyright header to .ti files and add support for comments in mkclass

### Bug

* [#2258](https://github.com/icinga/icinga2/issues/2258) (Configuration): Names for nested objects are evaluated at the wrong time
* [#2257](https://github.com/icinga/icinga2/issues/2257) (Configuration): DebugInfo is missing for nested dictionaries
* [#2254](https://github.com/icinga/icinga2/issues/2254): CreateProcess fails on Windows 7
* [#2241](https://github.com/icinga/icinga2/issues/2241) (Cluster): node wizard uses incorrect path for the CA certificate
* [#2237](https://github.com/icinga/icinga2/issues/2237) (Configuration): Wrong set of dependency state when a host depends on a service
* [#2235](https://github.com/icinga/icinga2/issues/2235): Unit tests fail to run
* [#2233](https://github.com/icinga/icinga2/issues/2233): Get rid of static boost::mutex variables
* [#2222](https://github.com/icinga/icinga2/issues/2222) (DB IDO): IDO module crashes on Windows
* [#2221](https://github.com/icinga/icinga2/issues/2221): Installation on Windows fails
* [#2220](https://github.com/icinga/icinga2/issues/2220) (Notifications): Missing state filter 'OK' must not prevent recovery notifications being sent
* [#2215](https://github.com/icinga/icinga2/issues/2215): mkclass crashes when called without arguments
* [#2214](https://github.com/icinga/icinga2/issues/2214) (Cluster): Removing multiple services fails
* [#2206](https://github.com/icinga/icinga2/issues/2206): Plugin execution on Windows does not work
* [#2205](https://github.com/icinga/icinga2/issues/2205): Compilation Error with boost 1.56 under Windows
* [#2201](https://github.com/icinga/icinga2/issues/2201): Exception when executing check
* [#2200](https://github.com/icinga/icinga2/issues/2200) (Configuration): Nested templates do not work \(anymore\)
* [#2199](https://github.com/icinga/icinga2/issues/2199) (CLI): Typo in output of 'icinga2 object list'
* [#2197](https://github.com/icinga/icinga2/issues/2197) (Notifications): only notify users on recovery which have been notified before \(not-ok state\)
* [#2195](https://github.com/icinga/icinga2/issues/2195) (Cluster): Invalid checkresult object causes Icinga 2 to crash
* [#2177](https://github.com/icinga/icinga2/issues/2177) (CLI): 'pki request' fails with serial permission error
* [#2172](https://github.com/icinga/icinga2/issues/2172) (Configuration): There is no \_\_name available to nested objects
* [#2171](https://github.com/icinga/icinga2/issues/2171) (Configuration): Nesting an object in a template causes the template to become non-abstract
* [#2170](https://github.com/icinga/icinga2/issues/2170) (Configuration): Object list dump erraneously evaluates template definitions
* [#2166](https://github.com/icinga/icinga2/issues/2166) (Cluster): Error message is always shown even when the host exists
* [#2165](https://github.com/icinga/icinga2/issues/2165) (Cluster): Incorrect warning message for "node update-config"
* [#2164](https://github.com/icinga/icinga2/issues/2164) (Cluster): Error in migrate-hosts
* [#2162](https://github.com/icinga/icinga2/issues/2162) (CLI): Change blacklist/whitelist storage
* [#2156](https://github.com/icinga/icinga2/issues/2156) (Cluster): Use ScriptVariable::Get for RunAsUser/RunAsGroup
* [#2155](https://github.com/icinga/icinga2/issues/2155) (Cluster): Agent health check must not have zone attribute
* [#2153](https://github.com/icinga/icinga2/issues/2153) (Cluster): Misleading error messages for blacklist/whitelist remove
* [#2142](https://github.com/icinga/icinga2/issues/2142) (Configuration): Icinga2 fails to start due to configuration errors
* [#2141](https://github.com/icinga/icinga2/issues/2141): Build fails
* [#2137](https://github.com/icinga/icinga2/issues/2137): Utility::GetFQDN doesn't work on OS X
* [#2134](https://github.com/icinga/icinga2/issues/2134): Hosts/services should not have themselves as parents
* [#2133](https://github.com/icinga/icinga2/issues/2133): OnStateLoaded isn't called for objects which don't have any state
* [#2132](https://github.com/icinga/icinga2/issues/2132) (CLI): cli command 'node setup update-config' overwrites existing constants.conf
* [#2128](https://github.com/icinga/icinga2/issues/2128) (CLI): Cli: Node Setup/Wizard running as root must chown\(\) generated files to icinga daemon user
* [#2127](https://github.com/icinga/icinga2/issues/2127) (Configuration): can't assign Service to Host in nested HostGroup
* [#2125](https://github.com/icinga/icinga2/issues/2125) (Metrics): Performance data via API is broken
* [#2116](https://github.com/icinga/icinga2/issues/2116) (CLI): Cli command: Repository should validate if object exists before add/remove
* [#2106](https://github.com/icinga/icinga2/issues/2106) (Cluster): When replaying logs the secobj attribute is ignored
* [#2091](https://github.com/icinga/icinga2/issues/2091) (CLI): Cli command: pki request throws exception on connection failure
* [#2083](https://github.com/icinga/icinga2/issues/2083): CMake warnings on OS X
* [#2077](https://github.com/icinga/icinga2/issues/2077) (CLI): CLI: Auto-completion with colliding arguments
* [#2070](https://github.com/icinga/icinga2/issues/2070) (DB IDO): CLI / MySQL error during vagrant provisioning
* [#2068](https://github.com/icinga/icinga2/issues/2068) (CLI): pki new-cert doesn't check whether the files were successfully written
* [#2065](https://github.com/icinga/icinga2/issues/2065) (DB IDO): Schema upgrade files are missing in /usr/share/icinga2-ido-{mysql,pgsql} 
* [#2063](https://github.com/icinga/icinga2/issues/2063) (CLI): Cli commands: Integers in arrays are printed incorrectly
* [#2057](https://github.com/icinga/icinga2/issues/2057) (CLI): failed en/disable feature should return error
* [#2056](https://github.com/icinga/icinga2/issues/2056) (CLI): Commands are auto-completed when they shouldn't be
* [#2051](https://github.com/icinga/icinga2/issues/2051) (Configuration): custom attribute name 'type' causes empty vars dictionary
* [#2048](https://github.com/icinga/icinga2/issues/2048) (Compat): Fix reading perfdata in compat/checkresultreader
* [#2042](https://github.com/icinga/icinga2/issues/2042) (Plugins): Setting snmp\_v2 can cause snmp-manubulon-command derived checks to fail
* [#2038](https://github.com/icinga/icinga2/issues/2038) (Configuration): snmp-load checkcommand has a wrong "-T" param value
* [#2034](https://github.com/icinga/icinga2/issues/2034) (Configuration): Importing a CheckCommand in a NotificationCommand results in an exception without stacktrace.
* [#2029](https://github.com/icinga/icinga2/issues/2029) (Configuration): Error messages for invalid imports missing
* [#2026](https://github.com/icinga/icinga2/issues/2026) (Configuration): config parser crashes on unknown attribute in assign
* [#2006](https://github.com/icinga/icinga2/issues/2006) (Configuration): snmp-load checkcommand has wrong threshold syntax
* [#2005](https://github.com/icinga/icinga2/issues/2005) (Metrics): icinga2 returns exponentail perfdata format with check\_nt
* [#2004](https://github.com/icinga/icinga2/issues/2004) (Metrics): Icinga2 changes perfdata order and removes maximum
* [#2001](https://github.com/icinga/icinga2/issues/2001) (Notifications): default value for "disable\_notifications" in service dependencies is set to "false"
* [#1950](https://github.com/icinga/icinga2/issues/1950) (Configuration): Typo for "HTTP Checks" match in groups.conf
* [#1720](https://github.com/icinga/icinga2/issues/1720) (Notifications): delaying notifications with times.begin should postpone first notification into that window

### ITL

* [#2204](https://github.com/icinga/icinga2/issues/2204) (ITL): Plugin Check Commands: disk is missing '-p', 'x' parameter
* [#2017](https://github.com/icinga/icinga2/issues/2017) (ITL): ITL: check\_procs and check\_http are missing arguments

### Documentation

* [#2218](https://github.com/icinga/icinga2/issues/2218) (Documentation): Documentation: Update Icinga Web 2 installation
* [#2191](https://github.com/icinga/icinga2/issues/2191) (Documentation): link missing in documentation about livestatus
* [#2175](https://github.com/icinga/icinga2/issues/2175) (Documentation): Documentation for arrays & dictionaries in custom attributes and their usage in apply rules for
* [#2160](https://github.com/icinga/icinga2/issues/2160) (Documentation): Documentation: Explain how to manage agent config in central repository
* [#2150](https://github.com/icinga/icinga2/issues/2150) (Documentation): Documentation: Move troubleshooting after the getting started chapter
* [#2143](https://github.com/icinga/icinga2/issues/2143) (Documentation): Documentation: Revamp getting started with 1 host and multiple \(service\) applies
* [#2130](https://github.com/icinga/icinga2/issues/2130) (Documentation): Documentation: Mention 'icinga2 object list' in config validation
* [#2129](https://github.com/icinga/icinga2/issues/2129) (Documentation): Fix typos and other small corrections in documentation
* [#2093](https://github.com/icinga/icinga2/issues/2093) (Documentation): Documentation: 1-about contribute links to non-existing report a bug howto
* [#2052](https://github.com/icinga/icinga2/issues/2052) (Documentation): Wrong usermod command for external command pipe setup
* [#2041](https://github.com/icinga/icinga2/issues/2041) (Documentation): Documentation: Cli Commands
* [#2037](https://github.com/icinga/icinga2/issues/2037) (Documentation): Documentation: Wrong check command for snmp-int\(erface\)
* [#2033](https://github.com/icinga/icinga2/issues/2033) (Documentation): Docs: Default command timeout is 60s not 5m
* [#2028](https://github.com/icinga/icinga2/issues/2028) (Documentation): Icinga2 docs: link supported operators from sections about apply rules
* [#2024](https://github.com/icinga/icinga2/issues/2024) (Documentation): Documentation: Add support for locally-scoped variables for host/service in applied Dependency
* [#2013](https://github.com/icinga/icinga2/issues/2013) (Documentation): Documentation: Add host/services variables in apply rules 
* [#1998](https://github.com/icinga/icinga2/issues/1998) (Documentation): Documentation: Agent/Satellite Setup
* [#1972](https://github.com/icinga/icinga2/issues/1972) (Documentation): Document how to use multiple assign/ignore statements with logical "and" & "or"

### Support

* [#2253](https://github.com/icinga/icinga2/issues/2253) (Packages): Conditionally enable MySQL and PostgresSQL, add support for FreeBSD and DragonFlyBSD
* [#2236](https://github.com/icinga/icinga2/issues/2236) (Packages): Enable parallel builds for the Debian package
* [#2147](https://github.com/icinga/icinga2/issues/2147) (Packages): Feature `checker' is not enabled when installing Icinga 2 using our lates RPM snapshot packages
* [#2136](https://github.com/icinga/icinga2/issues/2136) (Packages): Build fails on RHEL 6.6
* [#2123](https://github.com/icinga/icinga2/issues/2123) (Packages): Post-update script \(migrate-hosts\) isn't run on RPM-based distributions
* [#2095](https://github.com/icinga/icinga2/issues/2095) (Packages): Unity build fails on RHEL 5
* [#2058](https://github.com/icinga/icinga2/issues/2058) (Packages): Debian package root permissions interfere with icinga2 cli commands as icinga user
* [#2007](https://github.com/icinga/icinga2/issues/2007) (Packages): SLES \(Suse Linux Enterprise Server\) 11 SP3 package dependency failure

## 2.1.1 (2014-09-16)

### Enhancement

* [#1938](https://github.com/icinga/icinga2/issues/1938): Unity builds: Detect whether \_\_COUNTER\_\_ is available
* [#1933](https://github.com/icinga/icinga2/issues/1933): Implement support for unity builds
* [#1932](https://github.com/icinga/icinga2/issues/1932): Ensure that namespaces for INITIALIZE\_ONCE and REGISTER\_TYPE are truly unique
* [#1931](https://github.com/icinga/icinga2/issues/1931): Add include guards for mkclass files
* [#1797](https://github.com/icinga/icinga2/issues/1797): Change log message for checking/sending notifications

### Bug

* [#1975](https://github.com/icinga/icinga2/issues/1975): fix memory leak ido\_pgsql
* [#1971](https://github.com/icinga/icinga2/issues/1971) (Livestatus): Livestatus hangs from time to time
* [#1967](https://github.com/icinga/icinga2/issues/1967) (Plugins): fping4 doesn't work correctly with the shipped command-plugins.conf
* [#1966](https://github.com/icinga/icinga2/issues/1966) (Cluster): Segfault using cluster in TlsStream::IsEof
* [#1958](https://github.com/icinga/icinga2/issues/1958) (Configuration): Manubulon-Plugin conf Filename wrong
* [#1957](https://github.com/icinga/icinga2/issues/1957): Build fails on Haiku
* [#1955](https://github.com/icinga/icinga2/issues/1955) (Cluster): new SSL Errors with too many queued messages
* [#1954](https://github.com/icinga/icinga2/issues/1954): Missing differentiation between service and systemctl
* [#1952](https://github.com/icinga/icinga2/issues/1952) (Metrics): GraphiteWriter should ignore empty perfdata value
* [#1948](https://github.com/icinga/icinga2/issues/1948): pipe2 returns ENOSYS on GNU Hurd and Debian kfreebsd
* [#1946](https://github.com/icinga/icinga2/issues/1946): Exit code is not initialized for some failed checks
* [#1940](https://github.com/icinga/icinga2/issues/1940): icinga2-list-objects complains about Umlauts and stops output
* [#1935](https://github.com/icinga/icinga2/issues/1935): icinga2-list-objects doesn't work with Python 3
* [#1934](https://github.com/icinga/icinga2/issues/1934) (Configuration): Remove validator for the Script type
* [#1930](https://github.com/icinga/icinga2/issues/1930): "Error parsing performance data" in spite of "enable\_perfdata = false"
* [#1910](https://github.com/icinga/icinga2/issues/1910) (Cluster): SSL errors with interleaved SSL\_read/write
* [#1862](https://github.com/icinga/icinga2/issues/1862) (Cluster): SSL\_read errors during restart
* [#1849](https://github.com/icinga/icinga2/issues/1849) (Cluster): Too many queued messages
* [#1782](https://github.com/icinga/icinga2/issues/1782): make test fails on openbsd
* [#1522](https://github.com/icinga/icinga2/issues/1522): Link libcJSON against libm

### Documentation

* [#1985](https://github.com/icinga/icinga2/issues/1985) (Documentation): clarify on db ido upgrades
* [#1962](https://github.com/icinga/icinga2/issues/1962) (Documentation): Extend documentation for icinga-web on Debian systems
* [#1949](https://github.com/icinga/icinga2/issues/1949) (Documentation): Explain event commands and their integration by a real life example \(httpd restart via ssh\)
* [#1927](https://github.com/icinga/icinga2/issues/1927) (Documentation): Document how to use @ to escape keywords

### Support

* [#1960](https://github.com/icinga/icinga2/issues/1960) (Packages): GNUInstallDirs.cmake outdated
* [#1944](https://github.com/icinga/icinga2/issues/1944) (Packages): service icinga2 status - prints cat error if the service is stopped
* [#1941](https://github.com/icinga/icinga2/issues/1941) (Packages): icinga2 init-script terminates with exit code 0 if $DAEMON is not in place or not executable
* [#1939](https://github.com/icinga/icinga2/issues/1939) (Packages): Enable unity build for RPM/Debian packages
* [#1937](https://github.com/icinga/icinga2/issues/1937) (Packages): Figure out a better way to set the version for snapshot builds
* [#1936](https://github.com/icinga/icinga2/issues/1936) (Packages): Fix rpmlint errors
* [#1928](https://github.com/icinga/icinga2/issues/1928) (Packages): icinga2.spec: files-attr-not-set for python-icinga2 package

## 2.1.0 (2014-08-29)

### Notes

* DB IDO schema upgrade ([MySQL](#upgrading-mysql-db),[PostgreSQL](#upgrading-postgresql-db) required!
    * new schema version: **1.11.7**
    * RPMs install the schema files into `/usr/share/icinga2-ido*` instead of `/usr/share/doc/icinga2-ido*` #6881
* [Information for config objects](#list-configuration-objects) using `icinga2-list-objects` script #6702
* Add Python 2.4 as requirement #6702
* Add search path: If `-c /etc/icinga2/icinga2.conf` is omitted, use `SysconfDir + "/icinga2/icinga2.conf"` #6874
* Change log level for failed commands #6751
* Notifications are load-balanced in a [High Availability cluster setup](#high-availability-notifications) #6203
    * New config attribute: `enable_ha`
* DB IDO "run once" or "run everywhere" mode in a [High Availability cluster setup](#high-availability-db-ido) #6203 #6827
    * New config attributes: `enable_ha` and `failover_timeout`
* RPMs use the `icingacmd` group for /var/{cache,log,run}/icinga2 #6948

### Enhancement

* [#1879](https://github.com/icinga/icinga2/issues/1879): Enhance logging for perfdata/graphitewriter
* [#1871](https://github.com/icinga/icinga2/issues/1871) (Configuration): add search path for icinga2.conf
* [#1843](https://github.com/icinga/icinga2/issues/1843) (DB IDO): delay ido connect in ha cluster
* [#1810](https://github.com/icinga/icinga2/issues/1810): Change log level for failed commands
* [#1788](https://github.com/icinga/icinga2/issues/1788): Release 2.1
* [#1786](https://github.com/icinga/icinga2/issues/1786) (Configuration): Information for config objects
* [#1760](https://github.com/icinga/icinga2/issues/1760) (Plugins): Plugin Check Commands: add manubulon snmp plugins
* [#1548](https://github.com/icinga/icinga2/issues/1548) (Cluster): Log replay sends messages to instances which shouldn't get those messages
* [#1546](https://github.com/icinga/icinga2/issues/1546) (Cluster): Better cluster support for notifications / IDO
* [#1491](https://github.com/icinga/icinga2/issues/1491) (Cluster): Better log messages for cluster changes
* [#977](https://github.com/icinga/icinga2/issues/977) (Cluster): Cluster support for modified attributes

### Bug

* [#1916](https://github.com/icinga/icinga2/issues/1916): Build fails with Boost 1.56
* [#1903](https://github.com/icinga/icinga2/issues/1903) (Cluster): Host and service checks stuck in "pending" when hostname = localhost a parent/satellite setup
* [#1902](https://github.com/icinga/icinga2/issues/1902): Commands are processed multiple times
* [#1896](https://github.com/icinga/icinga2/issues/1896): check file permissions in /var/cache/icinga2
* [#1884](https://github.com/icinga/icinga2/issues/1884): External command pipe: Too many open files
* [#1819](https://github.com/icinga/icinga2/issues/1819): ExternalCommandListener fails open pipe: Too many open files

### Documentation

* [#1924](https://github.com/icinga/icinga2/issues/1924) (Documentation): add example selinux policy for external command pipe
* [#1915](https://github.com/icinga/icinga2/issues/1915) (Documentation): how to add a new cluster node
* [#1913](https://github.com/icinga/icinga2/issues/1913) (Documentation): Keyword "required" used inconsistently for host and service "icon\_image\*" attributes
* [#1905](https://github.com/icinga/icinga2/issues/1905) (Documentation): Update command arguments 'set\_if' and beautify error message
* [#1897](https://github.com/icinga/icinga2/issues/1897) (Documentation): Add documentation for icinga2-list-objects
* [#1889](https://github.com/icinga/icinga2/issues/1889) (Documentation): Enhance Graphite Writer description
* [#1881](https://github.com/icinga/icinga2/issues/1881) (Documentation): clarify on which config tools are available
* [#1872](https://github.com/icinga/icinga2/issues/1872) (Documentation): Wrong parent in Load Distribution
* [#1868](https://github.com/icinga/icinga2/issues/1868) (Documentation): Wrong object attribute 'enable\_flap\_detection'
* [#1867](https://github.com/icinga/icinga2/issues/1867) (Documentation): Add systemd options: enable, journal
* [#1865](https://github.com/icinga/icinga2/issues/1865) (Documentation): add section about disabling re-notifications
* [#1864](https://github.com/icinga/icinga2/issues/1864) (Documentation): Add section for reserved keywords
* [#1847](https://github.com/icinga/icinga2/issues/1847) (Documentation): Explain how the order attribute works in commands
* [#1807](https://github.com/icinga/icinga2/issues/1807) (Documentation): Better explanation for HA config cluster
* [#1787](https://github.com/icinga/icinga2/issues/1787) (Documentation): Documentation for zones and cluster permissions
* [#1761](https://github.com/icinga/icinga2/issues/1761) (Documentation): Migration: note on check command timeouts

### Support

* [#1923](https://github.com/icinga/icinga2/issues/1923) (Packages): 64-bit RPMs are not installable
* [#1888](https://github.com/icinga/icinga2/issues/1888) (Packages): Recommend related packages on SUSE distributions
* [#1887](https://github.com/icinga/icinga2/issues/1887) (Installation): Clean up spec file
* [#1885](https://github.com/icinga/icinga2/issues/1885) (Packages): enforce /usr/lib as base for the cgi path on SUSE distributions
* [#1883](https://github.com/icinga/icinga2/issues/1883) (Installation): use \_rundir macro for configuring the run directory
* [#1873](https://github.com/icinga/icinga2/issues/1873) (Packages): make install does not install the db-schema

## 2.0.2 (2014-08-07)

### Notes

* DB IDO schema upgrade required (new schema version: 1.11.6)

### Enhancement

* [#1830](https://github.com/icinga/icinga2/issues/1830) (Plugins): Plugin Check Commands: Add timeout option to check\_ssh
* [#1826](https://github.com/icinga/icinga2/issues/1826): Print application paths for --version
* [#1785](https://github.com/icinga/icinga2/issues/1785): Release 2.0.2
* [#1784](https://github.com/icinga/icinga2/issues/1784) (Configuration): Require command to be an array when the arguments attribute is used
* [#1781](https://github.com/icinga/icinga2/issues/1781) (Plugins): Plugin Check Commands: Add expect option to check\_http

### Bug

* [#1861](https://github.com/icinga/icinga2/issues/1861): write startup error messages to error.log
* [#1858](https://github.com/icinga/icinga2/issues/1858): event command execution does not call finish handler
* [#1855](https://github.com/icinga/icinga2/issues/1855): Startup logfile is not flushed to disk
* [#1853](https://github.com/icinga/icinga2/issues/1853) (DB IDO): exit application if ido schema version does not match
* [#1852](https://github.com/icinga/icinga2/issues/1852): Error handler for getaddrinfo must use gai\_strerror
* [#1848](https://github.com/icinga/icinga2/issues/1848): Missing space in error message
* [#1840](https://github.com/icinga/icinga2/issues/1840): \[Patch\] Fix build issue and crash found on Solaris, potentially other Unix OSes
* [#1839](https://github.com/icinga/icinga2/issues/1839): Icinga 2 crashes during startup
* [#1834](https://github.com/icinga/icinga2/issues/1834) (Cluster): High Availablity does not synchronise the data like expected
* [#1829](https://github.com/icinga/icinga2/issues/1829): Service icinga2 reload command does not cause effect
* [#1828](https://github.com/icinga/icinga2/issues/1828): Fix notification definition if no host\_name / service\_description given
* [#1816](https://github.com/icinga/icinga2/issues/1816): Config validation without filename argument fails with unhandled exception
* [#1813](https://github.com/icinga/icinga2/issues/1813) (Metrics): GraphiteWriter: Malformatted integer values
* [#1800](https://github.com/icinga/icinga2/issues/1800) (Cluster): TLS Connections still unstable in 2.0.1
* [#1796](https://github.com/icinga/icinga2/issues/1796): "order" attribute doesn't seem to work as expected
* [#1792](https://github.com/icinga/icinga2/issues/1792) (Configuration): sample config: add check commands location hint \(itl/plugin check commands\)
* [#1779](https://github.com/icinga/icinga2/issues/1779) (Configuration): Remove superfluous quotes and commas in dictionaries
* [#1778](https://github.com/icinga/icinga2/issues/1778): Event Commands are triggered in OK HARD state everytime
* [#1775](https://github.com/icinga/icinga2/issues/1775): additional group rights missing when Icinga started with -u and -g
* [#1774](https://github.com/icinga/icinga2/issues/1774) (Cluster): Missing detailed error messages on ApiListener SSL Errors
* [#1766](https://github.com/icinga/icinga2/issues/1766): RPMLint security warning - missing-call-to-setgroups-before-setuid /usr/sbin/icinga2
* [#1757](https://github.com/icinga/icinga2/issues/1757) (DB IDO): NULL vs empty string
* [#1752](https://github.com/icinga/icinga2/issues/1752) (Cluster): Infinite loop in TlsStream::Close
* [#1744](https://github.com/icinga/icinga2/issues/1744) (DB IDO): Two Custom Variables with same name, but Upper/Lowercase creating IDO duplicate entry
* [#1741](https://github.com/icinga/icinga2/issues/1741): Command pipe blocks when trying to open it more than once in parallel
* [#1730](https://github.com/icinga/icinga2/issues/1730): Check and retry intervals are incorrect
* [#1729](https://github.com/icinga/icinga2/issues/1729): $TOTALHOSTSERVICESWARNING$ and $TOTALHOSTSERVICESCRITICAL$ aren't getting converted
* [#1728](https://github.com/icinga/icinga2/issues/1728): Service dependencies aren't getting converted properly
* [#1726](https://github.com/icinga/icinga2/issues/1726): group names quoted twice in arrays
* [#1723](https://github.com/icinga/icinga2/issues/1723): add log message for invalid performance data
* [#1722](https://github.com/icinga/icinga2/issues/1722): GraphiteWriter regularly sends empty lines
* [#1721](https://github.com/icinga/icinga2/issues/1721) (Configuration): Add cmake constant for PluginDir
* [#1684](https://github.com/icinga/icinga2/issues/1684) (Notifications): Notifications not always triggered
* [#1674](https://github.com/icinga/icinga2/issues/1674): ipmi-sensors segfault due to stack size
* [#1666](https://github.com/icinga/icinga2/issues/1666) (DB IDO): objects and their ids are inserted twice

### ITL

* [#1825](https://github.com/icinga/icinga2/issues/1825) (ITL): The "ssl" check command always sets -D
* [#1821](https://github.com/icinga/icinga2/issues/1821) (ITL): Order doesn't work in check ssh command

### Documentation

* [#1802](https://github.com/icinga/icinga2/issues/1802) (Documentation): wrong path for the file 'localhost.conf'
* [#1801](https://github.com/icinga/icinga2/issues/1801) (Documentation): Missing documentation about implicit dependency
* [#1791](https://github.com/icinga/icinga2/issues/1791) (Documentation): icinga Web: wrong path to command pipe
* [#1789](https://github.com/icinga/icinga2/issues/1789) (Documentation): update installation with systemd usage
* [#1762](https://github.com/icinga/icinga2/issues/1762) (Documentation): clarify on which features are required for classic ui/web/web2

### Support

* [#1845](https://github.com/icinga/icinga2/issues/1845) (Packages): Remove if\(NOT DEFINED ICINGA2\_SYSCONFIGFILE\) in etc/initsystem/CMakeLists.txt
* [#1842](https://github.com/icinga/icinga2/issues/1842) (Packages): incorrect sysconfig path on sles11
* [#1820](https://github.com/icinga/icinga2/issues/1820) (Installation): Repo Error on RHEL 6.5
* [#1780](https://github.com/icinga/icinga2/issues/1780) (Packages): Rename README to README.md
* [#1763](https://github.com/icinga/icinga2/issues/1763) (Packages): Build packages for el7
* [#1754](https://github.com/icinga/icinga2/issues/1754) (Installation): Location of the run directory is hard coded and bound to "local\_state\_dir"
* [#1699](https://github.com/icinga/icinga2/issues/1699) (Packages): Classic UI Debian/Ubuntu: apache 2.4 requires 'a2enmod cgi' & apacheutils installed
* [#1338](https://github.com/icinga/icinga2/issues/1338) (Packages): SUSE packages

## 2.0.1 (2014-07-10)

### Notes

Bugfix release

### Enhancement

* [#1713](https://github.com/icinga/icinga2/issues/1713) (Configuration): Add port option to check imap/pop/smtp and a new dig
* [#1049](https://github.com/icinga/icinga2/issues/1049) (Livestatus): OutputFormat python

### Bug

* [#1773](https://github.com/icinga/icinga2/issues/1773) (Notifications): Problem with enable\_notifications and retained state
* [#1772](https://github.com/icinga/icinga2/issues/1772) (Notifications): enable\_notifications = false for users has no effect
* [#1771](https://github.com/icinga/icinga2/issues/1771) (Cluster): Icinga crashes after "Too many queued messages"
* [#1769](https://github.com/icinga/icinga2/issues/1769): Build fails when MySQL is not installed
* [#1767](https://github.com/icinga/icinga2/issues/1767): Increase icinga.cmd Limit
* [#1753](https://github.com/icinga/icinga2/issues/1753) (Configuration): icinga2-sign-key creates ".crt" and ".key" files when the CA passphrase is invalid
* [#1751](https://github.com/icinga/icinga2/issues/1751) (Configuration): icinga2-build-ca shouldn't prompt for DN
* [#1749](https://github.com/icinga/icinga2/issues/1749): TLS connections are still unstable
* [#1745](https://github.com/icinga/icinga2/issues/1745): Icinga stops updating IDO after a while
* [#1743](https://github.com/icinga/icinga2/issues/1743) (Configuration): Please add --sni option to http check command
* [#1740](https://github.com/icinga/icinga2/issues/1740) (Notifications): Notifications causing segfault from exim
* [#1737](https://github.com/icinga/icinga2/issues/1737) (DB IDO): icinga2-ido-pgsql snapshot package missing dependecy dbconfig-common
* [#1736](https://github.com/icinga/icinga2/issues/1736): Remove line number information from stack traces
* [#1734](https://github.com/icinga/icinga2/issues/1734): Check command result doesn't match
* [#1731](https://github.com/icinga/icinga2/issues/1731): Dependencies should cache their parent and child object
* [#1727](https://github.com/icinga/icinga2/issues/1727): $SERVICEDESC$ isn't getting converted correctly
* [#1724](https://github.com/icinga/icinga2/issues/1724): Improve systemd service definition
* [#1716](https://github.com/icinga/icinga2/issues/1716) (Cluster): Icinga doesn't send SetLogPosition messages when one of the endpoints fails to connect
* [#1712](https://github.com/icinga/icinga2/issues/1712): parsing of double defined command can generate unexpected errors
* [#1704](https://github.com/icinga/icinga2/issues/1704): Reminder notifications are sent on disabled services 
* [#1698](https://github.com/icinga/icinga2/issues/1698): icinga2 cannot be built with both systemd and init.d files
* [#1697](https://github.com/icinga/icinga2/issues/1697) (Livestatus): Thruk Panorama View cannot query Host Status
* [#1695](https://github.com/icinga/icinga2/issues/1695): icinga2.state could not be opened
* [#1691](https://github.com/icinga/icinga2/issues/1691): build warnings
* [#1644](https://github.com/icinga/icinga2/issues/1644) (Cluster): base64 on CentOS 5 fails to read certificate bundles
* [#1639](https://github.com/icinga/icinga2/issues/1639) (Cluster): Deadlock in ApiListener::RelayMessage
* [#1609](https://github.com/icinga/icinga2/issues/1609): application fails to start on wrong log file permissions but does not tell about it
* [#1206](https://github.com/icinga/icinga2/issues/1206) (DB IDO): PostgreSQL string escaping

### ITL

* [#1739](https://github.com/icinga/icinga2/issues/1739) (ITL): Add more options to snmp check

### Documentation

* [#1777](https://github.com/icinga/icinga2/issues/1777) (Documentation): event command execution cases are missing
* [#1765](https://github.com/icinga/icinga2/issues/1765) (Documentation): change docs.icinga.org/icinga2/latest to git master
* [#1742](https://github.com/icinga/icinga2/issues/1742) (Documentation): Documentation for || and && is missing
* [#1702](https://github.com/icinga/icinga2/issues/1702) (Documentation): Array section confusing

### Support

* [#1764](https://github.com/icinga/icinga2/issues/1764) (Installation): ICINGA2\_SYSCONFIGFILE should use full path using CMAKE\_INSTALL\_FULL\_SYSCONFDIR
* [#1709](https://github.com/icinga/icinga2/issues/1709) (Packages): htpasswd should be installed with icinga2-classicui on Ubuntu
* [#1696](https://github.com/icinga/icinga2/issues/1696) (Packages): Copyright problems
* [#1655](https://github.com/icinga/icinga2/issues/1655) (Packages): Debian package icinga2-classicui needs versioned dependency of icinga-cgi\*

## 2.0.0 (2014-06-16)

### Notes

First official release

### Enhancement

* [#1600](https://github.com/icinga/icinga2/issues/1600): Prepare 2.0.0 release
* [#1575](https://github.com/icinga/icinga2/issues/1575) (Cluster): Cluster: global zone for all nodes
* [#1348](https://github.com/icinga/icinga2/issues/1348): move vagrant box into dedicated demo project
* [#1341](https://github.com/icinga/icinga2/issues/1341): Revamp migration script
* [#1322](https://github.com/icinga/icinga2/issues/1322): Update website for release
* [#1320](https://github.com/icinga/icinga2/issues/1320): Update documentation for 2.0

### Bug

* [#1694](https://github.com/icinga/icinga2/issues/1694): Separate CMakeLists.txt for etc/initsystem
* [#1682](https://github.com/icinga/icinga2/issues/1682) (Configuration): logrotate.conf file should rotate log files as icinga user
* [#1680](https://github.com/icinga/icinga2/issues/1680) (Livestatus): Column 'host\_name' does not exist in table 'hosts'
* [#1678](https://github.com/icinga/icinga2/issues/1678) (Livestatus): Nagvis does not work with livestatus \(invalid format\)
* [#1673](https://github.com/icinga/icinga2/issues/1673): OpenSUSE Packages do not enable basic features
* [#1669](https://github.com/icinga/icinga2/issues/1669) (Cluster): Segfault with zones without endpoints on config compile
* [#1642](https://github.com/icinga/icinga2/issues/1642): Check if host recovery notifications work
* [#1615](https://github.com/icinga/icinga2/issues/1615) (Cluster): Subdirectories in the zone config are not synced
* [#1427](https://github.com/icinga/icinga2/issues/1427): fd-handling in Daemonize incorrect
* [#1312](https://github.com/icinga/icinga2/issues/1312): Permissions error on startup is only logged but not on stderr

### ITL

* [#1690](https://github.com/icinga/icinga2/issues/1690) (ITL): improve predefined command-plugins

### Documentation

* [#1689](https://github.com/icinga/icinga2/issues/1689) (Documentation): explain the icinga 2 reload
* [#1681](https://github.com/icinga/icinga2/issues/1681) (Documentation): Add instructions to install debug symbols on debian systems
* [#1675](https://github.com/icinga/icinga2/issues/1675) (Documentation): add a note on no length restrictions for plugin output / perfdata
* [#1636](https://github.com/icinga/icinga2/issues/1636) (Documentation): Update command definitions to use argument conditions
* [#1572](https://github.com/icinga/icinga2/issues/1572) (Documentation): change docs.icinga.org/icinga2/snapshot to 'latest'
* [#1302](https://github.com/icinga/icinga2/issues/1302) (Documentation): Replace Sphinx with Icinga Web 2 Doc Module

### Support

* [#1686](https://github.com/icinga/icinga2/issues/1686) (Installation): man pages for scripts
* [#1685](https://github.com/icinga/icinga2/issues/1685) (Installation): Cleanup installer for 2.0 supported features
* [#1683](https://github.com/icinga/icinga2/issues/1683) (Installation): remove 0.0.x schema upgrade files
* [#1670](https://github.com/icinga/icinga2/issues/1670) (Packages): Ubuntu package Release file lacks 'Suite' line
* [#1645](https://github.com/icinga/icinga2/issues/1645) (Packages): Packages are not installable on CentOS 5
* [#1342](https://github.com/icinga/icinga2/issues/1342) (Installation): Less verbose start output using the initscript
* [#1319](https://github.com/icinga/icinga2/issues/1319) (Tests): Release tests
* [#907](https://github.com/icinga/icinga2/issues/907) (Packages): icinga2-classicui is not installable on Debian
* [#788](https://github.com/icinga/icinga2/issues/788) (Packages): add systemd support

