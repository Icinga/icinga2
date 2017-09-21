# Icinga 2.x CHANGELOG

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

* [#5594](https://github.com/icinga/icinga2/issues/5594) (Documentation): Docs: Enhance certificate and configuration troubleshooting chapter
* [#5593](https://github.com/icinga/icinga2/issues/5593) (Documentation): Docs: Add a note for upgrading to 2.7
* [#5583](https://github.com/icinga/icinga2/issues/5583) (Documentation): Docs: Add example for Windows service monitoring with check\_nscp\_api
* [#5582](https://github.com/icinga/icinga2/issues/5582) (Documentation): Docs: Add firewall details for check\_nscp\_api
* [#5523](https://github.com/icinga/icinga2/issues/5523) (Cluster, Log): Enhance client connect/sync logging and include bytes/zone in logs
* [#5522](https://github.com/icinga/icinga2/issues/5522) (Documentation): Docs: Update freshness checks; add chapter for external check results
* [#5496](https://github.com/icinga/icinga2/issues/5496) (Documentation): Docs: Update examples for match/regex/cidr\_match and mode for arrays \(Match{All,Any}\)
* [#5494](https://github.com/icinga/icinga2/issues/5494) (Documentation): Docs: Add section for multiple template imports
* [#5491](https://github.com/icinga/icinga2/issues/5491) (Documentation): Update "Getting Started" documentation with Alpine Linux
* [#5487](https://github.com/icinga/icinga2/issues/5487) (Documentation): Docs: Enhance Troubleshooting with nscp-local, check\_source, wrong thresholds
* [#5476](https://github.com/icinga/icinga2/issues/5476) (Documentation): Docs: Fix ITL chapter TOC; add introduction with mini TOC
* [#5475](https://github.com/icinga/icinga2/issues/5475) (Documentation): Docs: Add a note on required configuration updates for new notification scripts in v2.7.0
* [#5474](https://github.com/icinga/icinga2/issues/5474) (Notifications): Notification scripts - make HOSTADDRESS optional
* [#5468](https://github.com/icinga/icinga2/issues/5468) (Notifications): Make notification mails more readable. Remove redundancy and cruft.
* [#5461](https://github.com/icinga/icinga2/issues/5461) (Documentation): Update Icinga repository release rpm location

### Bug

* [#5585](https://github.com/icinga/icinga2/issues/5585) (DB IDO): Fix where clause for non-matching {downtime,comment}history IDO database updates
* [#5566](https://github.com/icinga/icinga2/issues/5566) (Cluster, Log): Logs: Change config sync update to highlight an information, not an error
* [#5549](https://github.com/icinga/icinga2/issues/5549) (Documentation): Fix cli command used to enable debuglog feature on windows
* [#5543](https://github.com/icinga/icinga2/issues/5543) (ITL): ITL: Correct arguments for ipmi-sensor CheckCommand
* [#5539](https://github.com/icinga/icinga2/issues/5539) (Plugins, Windows): check\_nscp\_api: Allow arguments containing spaces
* [#5537](https://github.com/icinga/icinga2/issues/5537) (Plugins): check\_nscp\_api: support spaces in query arguments
* [#5536](https://github.com/icinga/icinga2/issues/5536) (Documentation): Fixed nscp-disk service example
* [#5524](https://github.com/icinga/icinga2/issues/5524) (Cluster): Change FIFO::Optimize\(\) frequency for large messages
* [#5513](https://github.com/icinga/icinga2/issues/5513) (Cluster): Node in Cluster loses connection
* [#5506](https://github.com/icinga/icinga2/issues/5506) (Documentation): Docs: Fix wrong parameter for ITL CheckCommand nscp\_api
* [#5504](https://github.com/icinga/icinga2/issues/5504) (InfluxDB): Fix TLS Race Connecting to InfluxDB
* [#5503](https://github.com/icinga/icinga2/issues/5503) (Livestatus): Fix grouping for Livestatus queries with 'Stats'
* [#5502](https://github.com/icinga/icinga2/issues/5502) (Notifications): Fix duplicate variable in notification scripts
* [#5501](https://github.com/icinga/icinga2/issues/5501) (Installation, Packages): SELinux: fixes for 2.7.0
* [#5495](https://github.com/icinga/icinga2/issues/5495) (Notifications): Fix parameter order for AcknowledgeSvcProblem / AcknowledgeHostProblem / apiactions:AcknowledgeProblem
* [#5492](https://github.com/icinga/icinga2/issues/5492) (DB IDO): Comments may not be removed correctly
* [#5484](https://github.com/icinga/icinga2/issues/5484) (Log): Timestamp comparison of config files logs a wrong message
* [#5483](https://github.com/icinga/icinga2/issues/5483) (DB IDO): Fix config validation for DB IDO categories 'DbCatEverything'
* [#5479](https://github.com/icinga/icinga2/issues/5479) (Packages): Icinga2 2.7.0 requires SELinux boolean icinga2\_can\_connect\_all on CentOS 7 even for default port
* [#5477](https://github.com/icinga/icinga2/issues/5477) (Installation, Packages): Systemd: Add DefaultTasksMax=infinity to service file
* [#5469](https://github.com/icinga/icinga2/issues/5469) (InfluxDB): Failure to connect to InfluxDB increases CPU utilisation by 100%  for every failure
* [#5466](https://github.com/icinga/icinga2/issues/5466) (DB IDO): DB IDO: Fix host's unreachable state in history tables
* [#5460](https://github.com/icinga/icinga2/issues/5460) (InfluxDB): Icinga 2.7 InfluxdbWriter fails to write metrics to InfluxDB over HTTPS
* [#5458](https://github.com/icinga/icinga2/issues/5458) (DB IDO): IDO donwtimehistory records orphaned from scheduleddowntime records following restart
* [#5428](https://github.com/icinga/icinga2/issues/5428) (Documentation): "Plugin Check Commands" section inside ITL docs needs adjustments
* [#5405](https://github.com/icinga/icinga2/issues/5405) (DB IDO): IDO statehistory table does not show hosts going to "UNREACHABLE" state.
* [#5392](https://github.com/icinga/icinga2/issues/5392) (Packages): Ensure the cache directory exists
* [#5078](https://github.com/icinga/icinga2/issues/5078) (Compat, Livestatus): Livestatus hostsbygroup and servicesbyhostgroup do not work
* [#4918](https://github.com/icinga/icinga2/issues/4918) (Packages): cgroup: fork rejected by pids controller in /system.slice/icinga2.service
* [#4414](https://github.com/icinga/icinga2/issues/4414) (Packages): /usr/lib/icinga2/prepare-dirs does not create /var/cache/icinga2

### Support

* [#5599](https://github.com/icinga/icinga2/issues/5599): changelog.py: Add "backported" to the list of ignored labels
* [#5590](https://github.com/icinga/icinga2/issues/5590) (Cluster, Log): Silence log level for configuration file updates
* [#5529](https://github.com/icinga/icinga2/issues/5529) (Log): Change two more loglines for checkables so checkable is quoted
* [#5528](https://github.com/icinga/icinga2/issues/5528) (Log): Change loglines for checkables so checkable is quoted
* [#5516](https://github.com/icinga/icinga2/issues/5516) (Documentation): Updates the install dependencies for Debian 9 'stretch'
* [#5457](https://github.com/icinga/icinga2/issues/5457) (Documentation): Add Changelog generation script for GitHub API

## 2.7.0 (2017-08-02)

### Notes

* New mail notification scripts. Please note that this requires a configuration update to NotificationCommand objects, Notification apply rules for specific settings and of course the notification scripts. More can be found [here](https://github.com/Icinga/icinga2/pull/5475).
* check_nscp_api plugin for NSClient++ REST API checks
* Work queues for features including logs & metrics
* More metrics for the "icinga" check
* Many bugfixes

### Enhancement

* [#5421](https://github.com/icinga/icinga2/issues/5421) (Plugins, Windows): Windows Plugins: Add new parameter to check\_disk to show used space
* [#5372](https://github.com/icinga/icinga2/issues/5372) (ITL): Update ITL CheckCommand description attribute, part 2
* [#5365](https://github.com/icinga/icinga2/issues/5365) (Documentation): Update package documentation for Debian Stretch
* [#5363](https://github.com/icinga/icinga2/issues/5363) (ITL): Update missing description attributes for ITL CheckCommand definitions
* [#5348](https://github.com/icinga/icinga2/issues/5348) (Configuration): Implement support for handling exceptions in user scripts
* [#5347](https://github.com/icinga/icinga2/issues/5347) (ITL): Improve ITL CheckCommand description attribute
* [#5339](https://github.com/icinga/icinga2/issues/5339) (Documentation, ITL): Add accept\_cname to dns CheckCommand
* [#5333](https://github.com/icinga/icinga2/issues/5333) (Documentation): Update documentation for enhanced notification scripts
* [#5331](https://github.com/icinga/icinga2/issues/5331) (Graylog): GelfWriter: Add 'check\_command' to CHECK RESULT/\* NOTIFICATION/STATE CHANGE messages
* [#5330](https://github.com/icinga/icinga2/issues/5330) (Graphite): GraphiteWriter: Add 'connected' to stats; fix reconnect exceptions
* [#5329](https://github.com/icinga/icinga2/issues/5329) (Graylog): GelfWriter: Use async work queue and add feature metric stats
* [#5320](https://github.com/icinga/icinga2/issues/5320) (Configuration): zones.conf: Add global-templates & director-global by default
* [#5309](https://github.com/icinga/icinga2/issues/5309) (Documentation): Docs: Replace the command pipe w/ the REST API as Icinga Web 2 requirement in 'Getting Started' chapter
* [#5287](https://github.com/icinga/icinga2/issues/5287) (Graphite, InfluxDB, Performance Data): Use workqueues in Graphite and InfluxDB features
* [#5284](https://github.com/icinga/icinga2/issues/5284) (Check Execution): Add feature stats to 'icinga' check as performance data metrics
* [#5280](https://github.com/icinga/icinga2/issues/5280) (API, Cluster, Log): Implement WorkQueue metric stats and periodic logging
* [#5279](https://github.com/icinga/icinga2/issues/5279) (Documentation): Docs: Add API query example for acknowledgements w/o expire time
* [#5266](https://github.com/icinga/icinga2/issues/5266) (API, Cluster): Add API & Cluster metric stats to /v1/status & icinga check incl. performance data
* [#5264](https://github.com/icinga/icinga2/issues/5264) (Configuration): Implement new array match functionality
* [#5252](https://github.com/icinga/icinga2/issues/5252) (Tests): travis: Update to trusty as CI environment
* [#5248](https://github.com/icinga/icinga2/issues/5248) (Tests): Travis: Run config validation at the end
* [#5247](https://github.com/icinga/icinga2/issues/5247) (Log): Add target object in cluster error messages to debug log
* [#5246](https://github.com/icinga/icinga2/issues/5246) (API, Cluster): Add subjectAltName X509 ext for certificate requests
* [#5244](https://github.com/icinga/icinga2/issues/5244) (Documentation): Add a PR review section to CONTRIBUTING.md
* [#5242](https://github.com/icinga/icinga2/issues/5242) (Configuration): Allow expressions for the type in object/template declarations
* [#5241](https://github.com/icinga/icinga2/issues/5241) (InfluxDB): Verbose InfluxDB Error Logging
* [#5239](https://github.com/icinga/icinga2/issues/5239) (Plugins, Windows): Add NSCP API check plugin for NSClient++ HTTP API
* [#5236](https://github.com/icinga/icinga2/issues/5236) (ITL): ITL: Add some missing arguments to ssl\_cert
* [#5212](https://github.com/icinga/icinga2/issues/5212) (Cluster, Log): Add additional logging for config sync
* [#5210](https://github.com/icinga/icinga2/issues/5210) (ITL): Add report mode to db2\_health
* [#5170](https://github.com/icinga/icinga2/issues/5170) (ITL): Enhance mail notifications scripts and add support for command line parameters
* [#5167](https://github.com/icinga/icinga2/issues/5167) (Documentation): Add more assign where expression examples
* [#5164](https://github.com/icinga/icinga2/issues/5164) (Documentation, ITL): ITL: Add CheckCommand ssl\_cert, fix ssl attributes
* [#5145](https://github.com/icinga/icinga2/issues/5145): Add a GitHub issue template
* [#5144](https://github.com/icinga/icinga2/issues/5144) (Documentation): Extend troubleshooting docs w/ environment analysis and common tools
* [#5143](https://github.com/icinga/icinga2/issues/5143) (Documentation): Docs: Explain how to include your own config tree instead of conf.d
* [#5142](https://github.com/icinga/icinga2/issues/5142) (Documentation): Add an Elastic Stack Integrations chapter to feature documentation
* [#5140](https://github.com/icinga/icinga2/issues/5140) (Documentation): Documentation should explain that runtime modifications are not immediately updated for "object list"
* [#5139](https://github.com/icinga/icinga2/issues/5139) (ITL): Add more options to ldap CheckCommand
* [#5137](https://github.com/icinga/icinga2/issues/5137) (Documentation): Doc updates: Getting Started w/ own config, Troubleshooting w/ debug console
* [#5133](https://github.com/icinga/icinga2/issues/5133) (API, wishlist): ApiListener: Metrics for cluster data
* [#5129](https://github.com/icinga/icinga2/issues/5129) (ITL): Additional parameters for perfout manubulon scripts
* [#5126](https://github.com/icinga/icinga2/issues/5126) (ITL): Added support to NRPE v2 in NRPE CheckCommand
* [#5106](https://github.com/icinga/icinga2/issues/5106) (Configuration): Add director-global as global zone to the default zones.conf configuration
* [#5090](https://github.com/icinga/icinga2/issues/5090) (Cluster, Documentation): EventHandler to be executed at the endpoint
* [#5077](https://github.com/icinga/icinga2/issues/5077) (Documentation): Replace the 'command' feature w/ the REST API for Icinga Web 2
* [#5063](https://github.com/icinga/icinga2/issues/5063) (ITL): Add additional arguments to mssql\_health
* [#5019](https://github.com/icinga/icinga2/issues/5019) (ITL): Added CheckCommand definitions for SMART, RAID controller and IPMI ping check
* [#5016](https://github.com/icinga/icinga2/issues/5016) (Documentation, ITL): Add fuse.gvfs-fuse-daemon to disk\_exclude\_type
* [#5010](https://github.com/icinga/icinga2/issues/5010) (Documentation): \[Documentation\] Missing parameter for SNMPv3 auth
* [#4985](https://github.com/icinga/icinga2/issues/4985) (ITL): Allow hpasm command from ITL to run in local mode
* [#4964](https://github.com/icinga/icinga2/issues/4964) (ITL): ITL: check\_icmp: add missing TTL attribute
* [#4945](https://github.com/icinga/icinga2/issues/4945) (API, Log): No hint for missing permissions in Icinga2 log for API user
* [#4925](https://github.com/icinga/icinga2/issues/4925): Update changelog generation scripts for GitHub
* [#4839](https://github.com/icinga/icinga2/issues/4839) (ITL): Remove deprecated dns\_expected\_answer attribute
* [#4826](https://github.com/icinga/icinga2/issues/4826) (ITL): Prepare icingacli-businessprocess for next release
* [#4781](https://github.com/icinga/icinga2/issues/4781) (Packages): Improve SELinux Policy
* [#4661](https://github.com/icinga/icinga2/issues/4661) (ITL): ITL - check\_oracle\_health - report option to shorten output
* [#4411](https://github.com/icinga/icinga2/issues/4411) (InfluxDB, Log, Performance Data): Better Debugging for InfluxdbWriter
* [#4288](https://github.com/icinga/icinga2/issues/4288) (Cluster, Log): Add check information to the debuglog when check result is discarded
* [#4242](https://github.com/icinga/icinga2/issues/4242) (Configuration): Default mail notification from header
* [#3560](https://github.com/icinga/icinga2/issues/3560) (Documentation): Explain check\_memorys and check\_disks thresholds
* [#3557](https://github.com/icinga/icinga2/issues/3557) (Log): Log started and stopped features 
* [#1880](https://github.com/icinga/icinga2/issues/1880) (Documentation): add a section for 'monitoring the icinga2 node'
* [#123](https://github.com/icinga/icinga2/issues/123) (ITL): ITL: Update ipmi CheckCommand attributes 
* [#120](https://github.com/icinga/icinga2/issues/120) (ITL): Add new parameter for check\_http: -L: Wrap output in HTML link
* [#117](https://github.com/icinga/icinga2/issues/117) (ITL): Support --only-critical for check\_apt
* [#115](https://github.com/icinga/icinga2/issues/115) (ITL): Inverse Interface Switch for snmp-interface
* [#114](https://github.com/icinga/icinga2/issues/114) (ITL): Adding -A to snmp interfaces check

### Bug

* [#5433](https://github.com/icinga/icinga2/issues/5433) (CLI): Fix: update feature list help text
* [#5384](https://github.com/icinga/icinga2/issues/5384) (ITL): Remove default value for 'dns\_query\_type'
* [#5383](https://github.com/icinga/icinga2/issues/5383) (ITL): Monitoring-Plugins check\_dns command does not support the `-q` flag
* [#5367](https://github.com/icinga/icinga2/issues/5367) (CLI, Crash): Unable to start icinga2 with kernel-3.10.0-514.21.2 RHEL7
* [#5366](https://github.com/icinga/icinga2/issues/5366) (Documentation): Fixed wrong node in documentation chapter Client/Satellite Linux Setup
* [#5354](https://github.com/icinga/icinga2/issues/5354) (Documentation): Docs: Fix built-in template description and URLs
* [#5350](https://github.com/icinga/icinga2/issues/5350) (Plugins): check\_nscp\_api not building on Debian wheezy
* [#5349](https://github.com/icinga/icinga2/issues/5349) (Documentation): Docs: Fix broken format for notes/tips in CLI command chapter
* [#5344](https://github.com/icinga/icinga2/issues/5344) (ITL): Add ip4-or-ipv6 import to logstash ITL command
* [#5343](https://github.com/icinga/icinga2/issues/5343) (ITL): logstash ITL command misses import
* [#5316](https://github.com/icinga/icinga2/issues/5316) (Livestatus): Fix for stats min operator
* [#5308](https://github.com/icinga/icinga2/issues/5308) (Configuration): Improve validation for attributes which must not be 'null'
* [#5297](https://github.com/icinga/icinga2/issues/5297): Fix compiler warnings
* [#5295](https://github.com/icinga/icinga2/issues/5295) (Notifications): Fix missing apostrophe in notification log
* [#5292](https://github.com/icinga/icinga2/issues/5292): Build fix for OpenSSL 0.9.8 and stack\_st\_X509\_EXTENSION
* [#5288](https://github.com/icinga/icinga2/issues/5288) (Configuration): Hostgroup using assign for Host with groups = null segfault
* [#5278](https://github.com/icinga/icinga2/issues/5278): Build fix for I2\_LEAK\_DEBUG
* [#5262](https://github.com/icinga/icinga2/issues/5262) (Graylog): Fix performance data processing in GelfWriter feature
* [#5259](https://github.com/icinga/icinga2/issues/5259) (API): Don't allow acknowledgement expire timestamps in the past
* [#5256](https://github.com/icinga/icinga2/issues/5256) (Configuration): Config type changes break object serialization \(JsonEncode\)
* [#5250](https://github.com/icinga/icinga2/issues/5250) (API, Compat): Acknowledgement expire time in the past
* [#5245](https://github.com/icinga/icinga2/issues/5245) (Notifications): Fix that host downtimes might be triggered even if their state is Up
* [#5224](https://github.com/icinga/icinga2/issues/5224) (Configuration, Notifications): Icinga sends notifications even though a Downtime object exists
* [#5223](https://github.com/icinga/icinga2/issues/5223) (Plugins, Windows): Wrong return Code for Windows ICMP
* [#5219](https://github.com/icinga/icinga2/issues/5219) (InfluxDB): InfluxDBWriter feature might block and leak memory
* [#5211](https://github.com/icinga/icinga2/issues/5211) (API, Cluster): Config received is always accepted by client even if own config is newer
* [#5194](https://github.com/icinga/icinga2/issues/5194) (API, CLI): No subjectAltName in Icinga CA created CSRs
* [#5168](https://github.com/icinga/icinga2/issues/5168) (Windows): include files from other volume/partition
* [#5146](https://github.com/icinga/icinga2/issues/5146) (Configuration): parsing of scheduled downtime object allow typing range instead of ranges
* [#5132](https://github.com/icinga/icinga2/issues/5132) (Graphite): GraphiteWriter can slow down Icinga's check result processing
* [#5101](https://github.com/icinga/icinga2/issues/5101) (Packages, Windows): Fix incorrect metadata for the Chocolatey package
* [#5075](https://github.com/icinga/icinga2/issues/5075) (ITL): fix mitigation for nwc\_health
* [#5062](https://github.com/icinga/icinga2/issues/5062) (Compat): icinga2 checkresults error
* [#5043](https://github.com/icinga/icinga2/issues/5043) (API): API POST request with 'attrs' as array returns bad\_cast error
* [#5040](https://github.com/icinga/icinga2/issues/5040) (Cluster): CRL loading fails due to incorrect return code check
* [#5033](https://github.com/icinga/icinga2/issues/5033) (DB IDO): Flexible downtimes which are not triggered must not update DB IDO's actual\_end\_time in downtimehistory table
* [#5015](https://github.com/icinga/icinga2/issues/5015) (ITL): nwc\_health\_report attribute requires a value
* [#4984](https://github.com/icinga/icinga2/issues/4984) (API): Wrong response type when unauthorized
* [#4983](https://github.com/icinga/icinga2/issues/4983) (Livestatus): Typo in livestatus key worst\_services\_state for hostgroups table
* [#4977](https://github.com/icinga/icinga2/issues/4977) (Cluster, Installation): icinga2/api/log directory is not created
* [#4956](https://github.com/icinga/icinga2/issues/4956) (DB IDO): Fix persistent comments for Acknowledgements
* [#4941](https://github.com/icinga/icinga2/issues/4941) (Performance Data): PerfData: Server Timeouts for InfluxDB Writer
* [#4927](https://github.com/icinga/icinga2/issues/4927) (InfluxDB, Performance Data): InfluxDbWriter error 500 hanging Icinga daemon
* [#4921](https://github.com/icinga/icinga2/issues/4921) (Installation, Packages): No network dependency for /etc/init.d/icinga2
* [#4913](https://github.com/icinga/icinga2/issues/4913) (API): acknowledge-problem api sending notifications when notify is false
* [#4909](https://github.com/icinga/icinga2/issues/4909) (CLI): icinga2 feature disable fails on already disabled feature
* [#4896](https://github.com/icinga/icinga2/issues/4896) (Plugins): Windows Agent: performance data of check\_perfmon
* [#4832](https://github.com/icinga/icinga2/issues/4832) (API, Configuration): API max\_check\_attempts validation
* [#4818](https://github.com/icinga/icinga2/issues/4818): Acknowledgements marked with Persistent Comment are not honored
* [#4779](https://github.com/icinga/icinga2/issues/4779): Superflous error messages for non-exisiting lsb\_release/sw\_vers commands \(on NetBSD\)
* [#4778](https://github.com/icinga/icinga2/issues/4778): Fix for traditional glob\(3\) behaviour
* [#4777](https://github.com/icinga/icinga2/issues/4777): NetBSD execvpe.c fix
* [#4776](https://github.com/icinga/icinga2/issues/4776) (Installation): NetBSD install path fixes
* [#4709](https://github.com/icinga/icinga2/issues/4709) (API): Posting config stage fails on FreeBSD
* [#4696](https://github.com/icinga/icinga2/issues/4696) (Notifications): Notifications are sent when reloading Icinga 2 even though they're deactivated via modified attributes
* [#4666](https://github.com/icinga/icinga2/issues/4666) (Graylog, Performance Data): GelfWriter with enable\_send\_perfdata breaks checks
* [#4621](https://github.com/icinga/icinga2/issues/4621) (Configuration, Notifications, Packages): notifications always enabled after update
* [#4532](https://github.com/icinga/icinga2/issues/4532) (Graylog, Performance Data): Icinga 2 "hangs" if the GelfWriter cannot send messages
* [#4440](https://github.com/icinga/icinga2/issues/4440) (DB IDO, Log): Exceptions might be better than exit in IDO
* [#3664](https://github.com/icinga/icinga2/issues/3664) (DB IDO): mysql\_error cannot be used for mysql\_init
* [#3483](https://github.com/icinga/icinga2/issues/3483) (Compat): Stacktrace on Command Pipe Error
* [#3410](https://github.com/icinga/icinga2/issues/3410) (Livestatus): Livestatus: Problem with stats min operator
* [#121](https://github.com/icinga/icinga2/issues/121) (CLI): give only warnings if feature is already disabled

### Support

* [#5448](https://github.com/icinga/icinga2/issues/5448) (Documentation): Update documentation for 2.7.0
* [#5440](https://github.com/icinga/icinga2/issues/5440) (Documentation): Add missing notification state filter to documentation 
* [#5425](https://github.com/icinga/icinga2/issues/5425) (Documentation): Fix formatting in API docs
* [#5410](https://github.com/icinga/icinga2/issues/5410) (Documentation): Update docs for better compatibility with mkdocs
* [#5393](https://github.com/icinga/icinga2/issues/5393) (Documentation): Fix typo in the documentation
* [#5378](https://github.com/icinga/icinga2/issues/5378) (Documentation): Fixed warnings when using mkdocs
* [#5370](https://github.com/icinga/icinga2/issues/5370) (Documentation): Rename ChangeLog to CHANGELOG.md
* [#5359](https://github.com/icinga/icinga2/issues/5359) (CLI): Fixed missing closing bracket in CLI command pki new-cert.
* [#5358](https://github.com/icinga/icinga2/issues/5358) (Documentation): Add documentation for securing mysql on Debian/Ubuntu.
* [#5357](https://github.com/icinga/icinga2/issues/5357) (Documentation, Notifications): Notification Scripts: Ensure that mail from address works on Debian/RHEL/SUSE \(mailutils vs mailx\)
* [#5336](https://github.com/icinga/icinga2/issues/5336) (Documentation): Docs: Fix formatting issues and broken URLs
* [#5332](https://github.com/icinga/icinga2/issues/5332) (Configuration, Notifications): Notification Scripts: notification\_type is always required
* [#5326](https://github.com/icinga/icinga2/issues/5326) (Documentation, Installation): Install the images directory containing the needed PNGs for the markd
* [#5324](https://github.com/icinga/icinga2/issues/5324) (Documentation): Fix phrasing in Getting Started chapter
* [#5317](https://github.com/icinga/icinga2/issues/5317) (Documentation): Fix typo in INSTALL.md
* [#5315](https://github.com/icinga/icinga2/issues/5315) (Documentation): Docs: Replace nagios-plugins by monitoring-plugins for Debian/Ubuntu
* [#5314](https://github.com/icinga/icinga2/issues/5314) (Documentation): Document Common name \(CN\) in client setup
* [#5310](https://github.com/icinga/icinga2/issues/5310) (Packages): RPM: Disable SELinux policy hardlink
* [#5306](https://github.com/icinga/icinga2/issues/5306) (Documentation, Packages): Remove CentOS 5 from 'Getting started' docs
* [#5304](https://github.com/icinga/icinga2/issues/5304) (Documentation, Packages): Update INSTALL.md for RPM builds
* [#5303](https://github.com/icinga/icinga2/issues/5303) (Packages): RPM: Fix builds on Amazon Linux
* [#5299](https://github.com/icinga/icinga2/issues/5299) (Notifications): Ensure that "mail from" works on RHEL/CentOS
* [#5291](https://github.com/icinga/icinga2/issues/5291) (Documentation): Update docs for RHEL/CentOS 5 EOL
* [#5286](https://github.com/icinga/icinga2/issues/5286) (Configuration): Fix verbose mode in notifications scripts
* [#5285](https://github.com/icinga/icinga2/issues/5285) (Documentation): Fix sysstat installation in troubleshooting docs
* [#5275](https://github.com/icinga/icinga2/issues/5275) (Documentation): Add troubleshooting hints for cgroup fork errors
* [#5265](https://github.com/icinga/icinga2/issues/5265): Move PerfdataValue\(\) class into base library
* [#5251](https://github.com/icinga/icinga2/issues/5251) (Tests): Update Travis CI environment to trusty
* [#5238](https://github.com/icinga/icinga2/issues/5238) (DB IDO): Remove deprecated "DbCat1 | DbCat2" notation for DB IDO categories
* [#5237](https://github.com/icinga/icinga2/issues/5237) (Documentation): Docs: Add a note for Windows debuglog to the troubleshooting chapter
* [#5229](https://github.com/icinga/icinga2/issues/5229) (Installation): CMake: require a GCC version according to INSTALL.md
* [#5227](https://github.com/icinga/icinga2/issues/5227) (Documentation, ITL): feature/itl-vmware-esx-storage-path-standbyok
* [#5226](https://github.com/icinga/icinga2/issues/5226) (Packages): RPM spec: don't enable features after an upgrade
* [#5225](https://github.com/icinga/icinga2/issues/5225) (DB IDO): Don't call mysql\_error\(\) after a failure of mysql\_init\(\)
* [#5218](https://github.com/icinga/icinga2/issues/5218) (Packages): icinga2.spec: Allow selecting g++ compiler on older SUSE release builds
* [#5216](https://github.com/icinga/icinga2/issues/5216): Remove "... is is ..." in CONTRIBUTING.md
* [#5206](https://github.com/icinga/icinga2/issues/5206) (Documentation): Typo in Getting Started Guide
* [#5203](https://github.com/icinga/icinga2/issues/5203) (Documentation): Fix typo in Getting Started chapter
* [#5189](https://github.com/icinga/icinga2/issues/5189) (Documentation, Packages): RPM packaging updates
* [#5188](https://github.com/icinga/icinga2/issues/5188) (Documentation, Packages): Boost \>= 1.48 required
* [#5184](https://github.com/icinga/icinga2/issues/5184) (Documentation): Doc/appendix: fix malformed markdown links
* [#5181](https://github.com/icinga/icinga2/issues/5181) (Documentation): List SELinux packages required for building RPMs
* [#5178](https://github.com/icinga/icinga2/issues/5178) (Documentation, Windows): Documentation vague on "update-windows" check plugin
* [#5177](https://github.com/icinga/icinga2/issues/5177) (Packages): Issues Packing icinga 2.6.3 tar.gz to RPM
* [#5175](https://github.com/icinga/icinga2/issues/5175) (Documentation): Add a note about flapping problems to the docs
* [#5174](https://github.com/icinga/icinga2/issues/5174) (Documentation): Add missing object type to Apply Rules doc example
* [#5173](https://github.com/icinga/icinga2/issues/5173) (Documentation): Object type missing from ping Service example in docs
* [#5166](https://github.com/icinga/icinga2/issues/5166) (API, Documentation): Set zone attribute to no\_user\_modify for API POST requests
* [#5165](https://github.com/icinga/icinga2/issues/5165) (Documentation): Syntax error In Dependencies chapter
* [#5161](https://github.com/icinga/icinga2/issues/5161) (Documentation): ITL documentation - disk-windows usage note with % thresholds
* [#5157](https://github.com/icinga/icinga2/issues/5157) (Documentation): "Three Levels with master, Satellites, and Clients" chapter is not clear about client config
* [#5156](https://github.com/icinga/icinga2/issues/5156) (Documentation): Add CONTRIBUTING.md
* [#5155](https://github.com/icinga/icinga2/issues/5155) (Documentation): 3.5. Apply Rules topic in the docs needs work.
* [#5153](https://github.com/icinga/icinga2/issues/5153) (Packages): Changed dependency of selinux subpackage
* [#5151](https://github.com/icinga/icinga2/issues/5151) (Documentation): Replace http:// links with https:// links where a secure website exists
* [#5150](https://github.com/icinga/icinga2/issues/5150) (Documentation): Invalid links in documentation
* [#5149](https://github.com/icinga/icinga2/issues/5149) (Documentation): Update documentation, change http:// links to https:// links where a website exists
* [#5127](https://github.com/icinga/icinga2/issues/5127) (Installation): Improve systemd service file
* [#5111](https://github.com/icinga/icinga2/issues/5111) (Documentation): Fix duration attribute requirement for schedule-downtime API action
* [#5104](https://github.com/icinga/icinga2/issues/5104) (Documentation): Correct link to nscp documentation
* [#5102](https://github.com/icinga/icinga2/issues/5102) (Compat, Configuration, Packages): Deprecate the icinga2-classicui-config package
* [#5100](https://github.com/icinga/icinga2/issues/5100) (Packages, Windows): Update Chocolatey package to match current guidelines
* [#5097](https://github.com/icinga/icinga2/issues/5097) (Documentation): The last example for typeof\(\) is missing the result
* [#5094](https://github.com/icinga/icinga2/issues/5094) (Cluster, Configuration): Log message "Object cannot be deleted because it was not created using the API"
* [#5087](https://github.com/icinga/icinga2/issues/5087) (Configuration): Function metadata should show available arguments
* [#5046](https://github.com/icinga/icinga2/issues/5046) (ITL): Add querytype to dns check
* [#5042](https://github.com/icinga/icinga2/issues/5042) (DB IDO): Add link to upgrade documentation to log message
* [#4987](https://github.com/icinga/icinga2/issues/4987) (ITL): Review `dummy` entry in ITL
* [#124](https://github.com/icinga/icinga2/issues/124) (ITL): FreeBSD's /dev/fd can either be inside devfs, or be of type fdescfs.

## 2.6.3 (2017-03-29)

### Enhancement

* [#4954](https://github.com/icinga/icinga2/issues/4954) (Documentation): Add an example for /v1/actions/process-check-result which uses filter/type
* [#3133](https://github.com/icinga/icinga2/issues/3133) (Documentation): Add practical examples for apply expressions

### Bug

* [#5080](https://github.com/icinga/icinga2/issues/5080) (DB IDO): Missing index use can cause icinga\_downtimehistory queries to hang indefinitely
* [#4989](https://github.com/icinga/icinga2/issues/4989) (Check Execution): Icinga daemon runs with nice 5 after reload
* [#4930](https://github.com/icinga/icinga2/issues/4930) (Cluster): Change "Discarding 'config update object'" log messages to notice log level
* [#4603](https://github.com/icinga/icinga2/issues/4603) (DB IDO): With too many comments, Icinga reload process won't finish reconnecting to Database

### Support

* [#5057](https://github.com/icinga/icinga2/issues/5057) (Documentation): Update Security section in the Distributed Monitoring chapter
* [#5055](https://github.com/icinga/icinga2/issues/5055) (Documentation, ITL): mysql\_socket attribute missing in the documentation for the mysql CheckCommand
* [#5035](https://github.com/icinga/icinga2/issues/5035) (Documentation): Docs: Typo in Distributed Monitoring chapter
* [#5030](https://github.com/icinga/icinga2/issues/5030) (Documentation): Advanced topics: Mention the API and explain stick acks, fixed/flexible downtimes
* [#5029](https://github.com/icinga/icinga2/issues/5029) (Documentation): Advanced topics: Wrong acknowledgement notification filter
* [#4996](https://github.com/icinga/icinga2/issues/4996) (Documentation): documentation: mixed up host names in 6-distributed-monitoring.md
* [#4980](https://github.com/icinga/icinga2/issues/4980) (Documentation): Add OpenBSD and AlpineLinux package repositories to the documentation
* [#4955](https://github.com/icinga/icinga2/issues/4955) (Documentation, ITL): Review CheckCommand documentation including external URLs

## 2.6.2 (2017-02-13)

### Bug

* [#4952](https://github.com/icinga/icinga2/issues/4952) (API, CLI): Icinga crashes while trying to remove configuration files for objects which no longer exist

## 2.6.1 (2017-01-31)

### Notes

This release addresses a number of bugs we have identified in version 2.6.0.

The documentation changes reflect our recent move to GitHub.

### Enhancement

* [#4923](https://github.com/icinga/icinga2/issues/4923): Migration to Github
* [#4916](https://github.com/icinga/icinga2/issues/4916) (Documentation): Add travis-ci build status logo to README.md
* [#4869](https://github.com/icinga/icinga2/issues/4869) (Documentation): Update RELEASE.md
* [#4868](https://github.com/icinga/icinga2/issues/4868) (Documentation): Add more build details to INSTALL.md
* [#4813](https://github.com/icinga/icinga2/issues/4813): Include argument name for log message about incorrect set\_if values

### Bug

* [#4950](https://github.com/icinga/icinga2/issues/4950): IDO schema update is not compatible to MySQL 5.7
* [#4917](https://github.com/icinga/icinga2/issues/4917) (Documentation): Incorrect license file mentioned in README.md
* [#4908](https://github.com/icinga/icinga2/issues/4908) (Documentation): Move domain to icinga.com
* [#4885](https://github.com/icinga/icinga2/issues/4885) (Documentation): SLES 12 SP2 libboost\_thread package requires libboost\_chrono
* [#4882](https://github.com/icinga/icinga2/issues/4882): Crash - Error: parse error: premature EOF
* [#4877](https://github.com/icinga/icinga2/issues/4877) (DB IDO): IDO MySQL schema not working on MySQL 5.7
* [#4874](https://github.com/icinga/icinga2/issues/4874) (DB IDO): IDO: Timestamps in PostgreSQL may still have a time zone offset
* [#4870](https://github.com/icinga/icinga2/issues/4870) (Packages): SLES11 SP4 dependency on Postgresql \>= 8.4
* [#4867](https://github.com/icinga/icinga2/issues/4867): SIGPIPE shutdown on config reload
* [#4803](https://github.com/icinga/icinga2/issues/4803) (Documentation): Update Repositories in Docs

### Support

* [#4944](https://github.com/icinga/icinga2/issues/4944) (Documentation): doc/6-distributed-monitoring.md: Fix typo
* [#4934](https://github.com/icinga/icinga2/issues/4934) (Documentation): Update contribution section for GitHub

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

* [#4851](https://github.com/icinga/icinga2/issues/4851) (Documentation): Update README.md and correct project URLs
* [#4846](https://github.com/icinga/icinga2/issues/4846) (Documentation): Add a note for boolean values in the disk CheckCommand section
* [#4845](https://github.com/icinga/icinga2/issues/4845) (Documentation): Troubleshooting: Add examples for fetching the executed command line
* [#4842](https://github.com/icinga/icinga2/issues/4842) (ITL): Add tempdir attribute to postgres CheckCommand
* [#4840](https://github.com/icinga/icinga2/issues/4840) (Documentation): Update Windows screenshots in the client documentation
* [#4838](https://github.com/icinga/icinga2/issues/4838) (Documentation): Add example for concurrent\_checks in CheckerComponent object type
* [#4837](https://github.com/icinga/icinga2/issues/4837) (ITL): Add sudo option to mailq CheckCommand
* [#4836](https://github.com/icinga/icinga2/issues/4836) (ITL): Add verbose parameter to http CheckCommand
* [#4835](https://github.com/icinga/icinga2/issues/4835) (ITL): Add timeout option to mysql\_health CheckCommand
* [#4821](https://github.com/icinga/icinga2/issues/4821) (Documentation): Add a note about removing "conf.d" on the client for "top down command endpoint" setups
* [#4809](https://github.com/icinga/icinga2/issues/4809) (Documentation): Update API and Library Reference chapters
* [#4804](https://github.com/icinga/icinga2/issues/4804) (Documentation): Add a note about default template import to the CheckCommand object
* [#4798](https://github.com/icinga/icinga2/issues/4798) (Cluster): Deprecate cluster/client mode "bottom up" w/ repository.d and node update-config
* [#4796](https://github.com/icinga/icinga2/issues/4796) (Installation): Sort Changelog by category
* [#4793](https://github.com/icinga/icinga2/issues/4793) (Documentation): Docs: ITL plugins contrib order
* [#4792](https://github.com/icinga/icinga2/issues/4792) (Tests): Add unit test for notification state/type filter checks
* [#4787](https://github.com/icinga/icinga2/issues/4787) (Documentation): Doc: Swap packages.icinga.org w/ DebMon
* [#4780](https://github.com/icinga/icinga2/issues/4780) (Documentation): Add a note about pinning checks w/ command\_endpoint
* [#4770](https://github.com/icinga/icinga2/issues/4770) (API): Allow to evaluate macros through the API
* [#4724](https://github.com/icinga/icinga2/issues/4724) (Packages): Update .mailmap for icinga.com
* [#4713](https://github.com/icinga/icinga2/issues/4713) (Cluster): Check whether nodes are synchronizing the API log before putting them into UNKNOWN
* [#4708](https://github.com/icinga/icinga2/issues/4708) (Documentation): Add more Timeperiod examples in the documentation
* [#4706](https://github.com/icinga/icinga2/issues/4706) (Documentation): Add an example of multi-parents configuration for the Migration chapter
* [#4684](https://github.com/icinga/icinga2/issues/4684) (ITL): Add a radius CheckCommand for the radius check provide by nagios-plugins
* [#4681](https://github.com/icinga/icinga2/issues/4681) (ITL): Add CheckCommand definition for check\_logstash
* [#4672](https://github.com/icinga/icinga2/issues/4672) (ITL): Add timeout option to oracle\_health CheckCommand
* [#4671](https://github.com/icinga/icinga2/issues/4671) (Packages): Windows Installer should include NSClient++ 0.5.0
* [#4651](https://github.com/icinga/icinga2/issues/4651) (Plugins): Review windows plugins performance output
* [#4636](https://github.com/icinga/icinga2/issues/4636) (Documentation): Add development docs for writing a core dump file
* [#4631](https://github.com/icinga/icinga2/issues/4631) (Configuration): Suppress compiler warnings for auto-generated code
* [#4622](https://github.com/icinga/icinga2/issues/4622) (Cluster): Improve log message for ignored config updates
* [#4608](https://github.com/icinga/icinga2/issues/4608) (ITL): Add CheckCommand definition for check\_iostats
* [#4607](https://github.com/icinga/icinga2/issues/4607) (Packages): Improve support for building the chocolatey package
* [#4596](https://github.com/icinga/icinga2/issues/4596) (Documentation): Update service monitoring and distributed docs
* [#4590](https://github.com/icinga/icinga2/issues/4590): Make sure that libmethods is automatically loaded even when not using the ITL
* [#4588](https://github.com/icinga/icinga2/issues/4588) (Installation): Use raw string literals in mkembedconfig
* [#4587](https://github.com/icinga/icinga2/issues/4587) (Configuration): Implement support for default templates
* [#4584](https://github.com/icinga/icinga2/issues/4584) (Documentation): Add missing reference to libmethods for the default ITL command templates
* [#4580](https://github.com/icinga/icinga2/issues/4580) (API): Provide location information for objects and templates in the API
* [#4578](https://github.com/icinga/icinga2/issues/4578) (Installation): Improve detection for the -flto compiler flag
* [#4576](https://github.com/icinga/icinga2/issues/4576): Use lambda functions for INITIALIZE\_ONCE
* [#4575](https://github.com/icinga/icinga2/issues/4575): Use 'auto' for iterator declarations
* [#4571](https://github.com/icinga/icinga2/issues/4571): Implement an rvalue constructor for the String and Value classes
* [#4570](https://github.com/icinga/icinga2/issues/4570) (Configuration): Implement a command-line argument for "icinga2 console" to allow specifying a script file
* [#4569](https://github.com/icinga/icinga2/issues/4569) (Installation): Set versions for all internal libraries
* [#4563](https://github.com/icinga/icinga2/issues/4563) (Configuration): Remove unused method: ApplyRule::DiscardRules
* [#4559](https://github.com/icinga/icinga2/issues/4559): Replace BOOST\_FOREACH with range-based for loops
* [#4558](https://github.com/icinga/icinga2/issues/4558) (Installation): Update cmake config to require a compiler that supports C++11
* [#4557](https://github.com/icinga/icinga2/issues/4557): Add -fvisibility=hidden to the default compiler flags
* [#4551](https://github.com/icinga/icinga2/issues/4551) (Tests): Implement unit tests for state changes
* [#4543](https://github.com/icinga/icinga2/issues/4543) (ITL): ITL - check\_vmware\_esx - specify a datacenter/vsphere server for esx/host checks
* [#4537](https://github.com/icinga/icinga2/issues/4537): Implement an environment variable to keep Icinga from closing FDs on startup
* [#4536](https://github.com/icinga/icinga2/issues/4536): Avoid unnecessary string copies
* [#4535](https://github.com/icinga/icinga2/issues/4535): Remove deprecated functions
* [#4492](https://github.com/icinga/icinga2/issues/4492) (Documentation): Add information about function 'range'
* [#4324](https://github.com/icinga/icinga2/issues/4324) (ITL): Add CheckCommand definition for check\_glusterfs
* [#3684](https://github.com/icinga/icinga2/issues/3684) (Configuration): Command line option for config syntax validation
* [#2968](https://github.com/icinga/icinga2/issues/2968): Better message for apply errors
* [#2943](https://github.com/icinga/icinga2/issues/2943) (Installation): Make the user account configurable for the Windows service

### Bug

* [#4862](https://github.com/icinga/icinga2/issues/4862) (Documentation): "2.1.4. Installation Paths" should contain systemd paths
* [#4861](https://github.com/icinga/icinga2/issues/4861) (Documentation): Update "2.1.3. Enabled Features during Installation" - outdated "feature list"
* [#4859](https://github.com/icinga/icinga2/issues/4859) (Documentation): Update package instructions for Fedora
* [#4831](https://github.com/icinga/icinga2/issues/4831) (CLI): Wrong help string for node setup cli command argument --master\_host
* [#4829](https://github.com/icinga/icinga2/issues/4829) (Documentation): Missing API headers for X-HTTP-Method-Override
* [#4828](https://github.com/icinga/icinga2/issues/4828) (API): Crash in CreateObjectHandler \(regression from \#11684
* [#4827](https://github.com/icinga/icinga2/issues/4827) (Documentation): Fix example in PNP template docs
* [#4802](https://github.com/icinga/icinga2/issues/4802): Icinga tries to delete Downtime objects that were statically configured
* [#4801](https://github.com/icinga/icinga2/issues/4801): Sending a HUP signal to the child process for execution actually kills it
* [#4800](https://github.com/icinga/icinga2/issues/4800) (Documentation): Docs: Typo in "CLI commands" chapter
* [#4791](https://github.com/icinga/icinga2/issues/4791) (DB IDO): PostgreSQL: Don't use timestamp with timezone for UNIX timestamp columns
* [#4789](https://github.com/icinga/icinga2/issues/4789) (Notifications): Recovery notifications sent for Not-Problem notification type if notified before
* [#4775](https://github.com/icinga/icinga2/issues/4775) (Cluster): Crash w/ SendNotifications cluster handler and check result with empty perfdata
* [#4771](https://github.com/icinga/icinga2/issues/4771): Config validation crashes when using command\_endpoint without also having an ApiListener object
* [#4752](https://github.com/icinga/icinga2/issues/4752) (Graphite): Performance data writer for Graphite : Values without fraction limited to 2147483647 \(7FFFFFFF\)
* [#4740](https://github.com/icinga/icinga2/issues/4740): SIGALRM handling may be affected by recent commit
* [#4736](https://github.com/icinga/icinga2/issues/4736) (Documentation): Docs: wrong heading level for commands.conf and groups.conf
* [#4726](https://github.com/icinga/icinga2/issues/4726) (Notifications): Flapping notifications sent for soft state changes
* [#4717](https://github.com/icinga/icinga2/issues/4717) (API): Icinga crashes while deleting a config file which doesn't exist anymore
* [#4714](https://github.com/icinga/icinga2/issues/4714) (ITL): Default values for check\_swap are incorrect
* [#4710](https://github.com/icinga/icinga2/issues/4710) (ITL): snmp\_miblist variable to feed the -m option of check\_snmp is missing in the snmpv3 CheckCommand object
* [#4705](https://github.com/icinga/icinga2/issues/4705) (Documentation): Typo in the documentation
* [#4699](https://github.com/icinga/icinga2/issues/4699) (Documentation): Fix some spelling mistakes
* [#4678](https://github.com/icinga/icinga2/issues/4678) (Configuration): Configuration validation fails when setting tls\_protocolmin to TLSv1.2
* [#4677](https://github.com/icinga/icinga2/issues/4677) (ITL): Problem passing arguments to nscp-local CheckCommand objects
* [#4674](https://github.com/icinga/icinga2/issues/4674) (CLI): Parse error: "premature EOF" when running "icinga2 node update-config"
* [#4667](https://github.com/icinga/icinga2/issues/4667) (Documentation): Add documentation for logrotation for the mainlog feature
* [#4665](https://github.com/icinga/icinga2/issues/4665): Crash in ClusterEvents::SendNotificationsAPIHandler
* [#4653](https://github.com/icinga/icinga2/issues/4653) (Documentation): Corrections for distributed monitoring chapter
* [#4646](https://github.com/icinga/icinga2/issues/4646) (Notifications): Forced custom notification is setting "force\_next\_notification": true permanently
* [#4644](https://github.com/icinga/icinga2/issues/4644) (API): Crash in HttpRequest::Parse while processing HTTP request
* [#4641](https://github.com/icinga/icinga2/issues/4641) (Documentation): Docs: Migrating Notification example tells about filters instead of types
* [#4639](https://github.com/icinga/icinga2/issues/4639) (Documentation): GDB example in the documentation isn't working
* [#4630](https://github.com/icinga/icinga2/issues/4630) (Configuration): Validation does not highlight the correct attribute
* [#4629](https://github.com/icinga/icinga2/issues/4629) (CLI): broken: icinga2 --version
* [#4620](https://github.com/icinga/icinga2/issues/4620) (API): Invalid API filter error messages
* [#4619](https://github.com/icinga/icinga2/issues/4619) (CLI): Cli: boost::bad\_get on icinga::String::String\(icinga::Value&&\) 
* [#4618](https://github.com/icinga/icinga2/issues/4618) (ITL): Hangman easter egg is broken
* [#4616](https://github.com/icinga/icinga2/issues/4616): Build fails with Visual Studio 2015
* [#4612](https://github.com/icinga/icinga2/issues/4612) (Tests): Unit tests randomly crash after the tests have completed
* [#4606](https://github.com/icinga/icinga2/issues/4606): Remove unused last\_in\_downtime field
* [#4602](https://github.com/icinga/icinga2/issues/4602) (CLI): Last option highlighted as the wrong one, even when it is not the culprit
* [#4601](https://github.com/icinga/icinga2/issues/4601) (Documentation): Typo in distributed monitoring docs
* [#4599](https://github.com/icinga/icinga2/issues/4599): Unexpected state changes with max\_check\_attempts = 2
* [#4597](https://github.com/icinga/icinga2/issues/4597) (ITL): Default disk plugin check should not check inodes
* [#4595](https://github.com/icinga/icinga2/issues/4595) (ITL): Manubulon: Add missing procurve memory flag
* [#4589](https://github.com/icinga/icinga2/issues/4589) (Documentation): Fix help output for update-links.py
* [#4585](https://github.com/icinga/icinga2/issues/4585) (ITL): Fix code style violations in the ITL
* [#4583](https://github.com/icinga/icinga2/issues/4583) (Configuration): Debug hints for dictionary expressions are nested incorrectly
* [#4582](https://github.com/icinga/icinga2/issues/4582) (ITL): Incorrect help text for check\_swap
* [#4574](https://github.com/icinga/icinga2/issues/4574) (Notifications): Don't send Flapping\* notifications when downtime is active
* [#4573](https://github.com/icinga/icinga2/issues/4573) (DB IDO): Getting error during schema update 
* [#4572](https://github.com/icinga/icinga2/issues/4572) (Configuration): Config validation shouldnt allow 'endpoints = \[ "" \]'
* [#4566](https://github.com/icinga/icinga2/issues/4566) (Notifications): Fixed downtimes scheduled for a future date trigger DOWNTIMESTART notifications
* [#4564](https://github.com/icinga/icinga2/issues/4564): Add missing initializer for WorkQueue::m\_NextTaskID
* [#4556](https://github.com/icinga/icinga2/issues/4556) (Installation): logrotate file is not properly generated when the logrotate binary resides in /usr/bin
* [#4555](https://github.com/icinga/icinga2/issues/4555): Fix compiler warnings
* [#4541](https://github.com/icinga/icinga2/issues/4541) (DB IDO): Don't link against libmysqlclient\_r
* [#4538](https://github.com/icinga/icinga2/issues/4538): Don't update TimePeriod ranges for inactive objects
* [#4423](https://github.com/icinga/icinga2/issues/4423) (Performance Data): InfluxdbWriter does not write state other than 0
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
* [#4513](https://github.com/icinga/icinga2/issues/4513) (Documentation): Development docs: Add own section for gdb backtrace from a running process
* [#4506](https://github.com/icinga/icinga2/issues/4506) (ITL): Add interfacetable CheckCommand options --trafficwithpkt and --snmp-maxmsgsize
* [#4498](https://github.com/icinga/icinga2/issues/4498): Remove unnecessary Dictionary::Contains calls
* [#4493](https://github.com/icinga/icinga2/issues/4493) (Cluster): Improve performance for Endpoint config validation
* [#4491](https://github.com/icinga/icinga2/issues/4491): Improve performance for type lookups
* [#4487](https://github.com/icinga/icinga2/issues/4487) (DB IDO): Incremental updates for the IDO database
* [#4486](https://github.com/icinga/icinga2/issues/4486) (DB IDO): Remove unused code from the IDO classes
* [#4485](https://github.com/icinga/icinga2/issues/4485) (API): Add API action for generating a PKI ticket
* [#4479](https://github.com/icinga/icinga2/issues/4479) (Configuration): Implement comparison operators for the Array class
* [#4477](https://github.com/icinga/icinga2/issues/4477) (ITL): Add perfsyntax parameter to nscp-local-counter CheckCommand
* [#4468](https://github.com/icinga/icinga2/issues/4468) (Documentation): Add URL and short description for Monitoring Plugins inside the ITL documentation
* [#4467](https://github.com/icinga/icinga2/issues/4467): Implement the System\#sleep function
* [#4465](https://github.com/icinga/icinga2/issues/4465) (Configuration): Implement support for namespaces
* [#4464](https://github.com/icinga/icinga2/issues/4464) (CLI): Implement support for inspecting variables with LLDB/GDB
* [#4457](https://github.com/icinga/icinga2/issues/4457): Implement support for marking functions as deprecated
* [#4456](https://github.com/icinga/icinga2/issues/4456) (ITL): Add custom variables for all check\_swap arguments
* [#4454](https://github.com/icinga/icinga2/issues/4454): Include compiler name/version and build host name in --version
* [#4453](https://github.com/icinga/icinga2/issues/4453) (Documentation): Rewrite Client and Cluster chapter and; add service monitoring chapter
* [#4451](https://github.com/icinga/icinga2/issues/4451) (Configuration): Move internal script functions into the 'Internal' namespace
* [#4449](https://github.com/icinga/icinga2/issues/4449): Improve logging for the WorkQueue class
* [#4445](https://github.com/icinga/icinga2/issues/4445): Rename/Remove experimental script functions
* [#4443](https://github.com/icinga/icinga2/issues/4443): Implement process\_check\_result script method for the Checkable class
* [#4442](https://github.com/icinga/icinga2/issues/4442) (API): Support for determining the Icinga 2 version via the API
* [#4437](https://github.com/icinga/icinga2/issues/4437) (ITL): Add command definition for check\_mysql\_query
* [#4431](https://github.com/icinga/icinga2/issues/4431) (Notifications): Add the notification type into the log message
* [#4424](https://github.com/icinga/icinga2/issues/4424) (Cluster): Enhance TLS handshake error messages with connection information
* [#4416](https://github.com/icinga/icinga2/issues/4416) (ITL): Add check command definition for check\_graphite
* [#4415](https://github.com/icinga/icinga2/issues/4415) (API): Remove obsolete debug log message
* [#4410](https://github.com/icinga/icinga2/issues/4410) (Configuration): Add map/reduce and filter functionality for the Array class
* [#4403](https://github.com/icinga/icinga2/issues/4403) (CLI): Add history for icinga2 console
* [#4398](https://github.com/icinga/icinga2/issues/4398) (Cluster): Log a warning if there are more than 2 zone endpoint members
* [#4397](https://github.com/icinga/icinga2/issues/4397) (ITL): A lot of missing parameters for \(latest\) mysql\_health
* [#4393](https://github.com/icinga/icinga2/issues/4393) (Cluster): Include IP address and port in the "New connection" log message
* [#4388](https://github.com/icinga/icinga2/issues/4388) (Configuration): Implement the \_\_ptr script function
* [#4386](https://github.com/icinga/icinga2/issues/4386) (Cluster): Improve error messages for failed certificate validation
* [#4381](https://github.com/icinga/icinga2/issues/4381) (Cluster): Improve log message for connecting nodes without configured Endpoint object
* [#4379](https://github.com/icinga/icinga2/issues/4379) (ITL): Add support for "-A" command line switch to CheckCommand "snmp-process" 
* [#4352](https://github.com/icinga/icinga2/issues/4352) (Cluster): Enhance client disconnect message for "No data received on new API connection."
* [#4348](https://github.com/icinga/icinga2/issues/4348) (DB IDO): Do not populate logentries table by default
* [#4332](https://github.com/icinga/icinga2/issues/4332) (ITL): Add check command definition for db2\_health
* [#4325](https://github.com/icinga/icinga2/issues/4325) (API): API: Add missing downtime\_depth attribute
* [#4314](https://github.com/icinga/icinga2/issues/4314) (DB IDO): Change Ido\*Connection 'categories' attribute to an array
* [#4305](https://github.com/icinga/icinga2/issues/4305) (ITL): Add check command definitions for kdc and rbl
* [#4297](https://github.com/icinga/icinga2/issues/4297) (ITL): add check command for plugin check\_apache\_status
* [#4295](https://github.com/icinga/icinga2/issues/4295) (DB IDO): Enhance IDO check with schema version info
* [#4294](https://github.com/icinga/icinga2/issues/4294) (DB IDO): Update DB IDO schema version to 1.14.1
* [#4290](https://github.com/icinga/icinga2/issues/4290) (API): Implement support for getting a list of global variables from the API
* [#4281](https://github.com/icinga/icinga2/issues/4281) (API): Support for enumerating available templates via the API
* [#4276](https://github.com/icinga/icinga2/issues/4276) (ITL): Adding option to access ifName for manubulon snmp-interface check command
* [#4268](https://github.com/icinga/icinga2/issues/4268) (Performance Data): InfluxDB Metadata
* [#4251](https://github.com/icinga/icinga2/issues/4251) (Tests): Add debugging mode for Utility::GetTime
* [#4250](https://github.com/icinga/icinga2/issues/4250) (ITL): Add CIM port parameter for esxi\_hardware CheckCommand
* [#4236](https://github.com/icinga/icinga2/issues/4236) (Documentation): Use HTTPS for debmon.org links in the documentation
* [#4206](https://github.com/icinga/icinga2/issues/4206) (Cluster): Add lag threshold for cluster-zone check
* [#4190](https://github.com/icinga/icinga2/issues/4190) (Packages): Windows Installer: Remove dependency on KB2999226 package
* [#4178](https://github.com/icinga/icinga2/issues/4178) (API): Improve logging for HTTP API requests
* [#4154](https://github.com/icinga/icinga2/issues/4154) (Configuration): Remove the \(unused\) 'inherits' keyword
* [#4135](https://github.com/icinga/icinga2/issues/4135) (Installation): Add script for automatically cherry-picking commits for minor versions
* [#4129](https://github.com/icinga/icinga2/issues/4129) (Configuration): Improve performance for field accesses
* [#4124](https://github.com/icinga/icinga2/issues/4124) (Documentation): Documentation review
* [#4061](https://github.com/icinga/icinga2/issues/4061) (Configuration): Allow strings in state/type filters
* [#4048](https://github.com/icinga/icinga2/issues/4048): Cleanup downtimes created by ScheduleDowntime
* [#4046](https://github.com/icinga/icinga2/issues/4046) (Configuration): Config parser should not log names of included files by default
* [#4023](https://github.com/icinga/icinga2/issues/4023) (ITL): Add "retries" option to check\_snmp command
* [#3999](https://github.com/icinga/icinga2/issues/3999) (API): ApiListener: Make minimum TLS version configurable
* [#3997](https://github.com/icinga/icinga2/issues/3997) (API): ApiListener: Force server's preferred cipher
* [#3911](https://github.com/icinga/icinga2/issues/3911) (Graphite): Add acknowledgement type to Graphite, InfluxDB, OpenTSDB metadata
* [#3888](https://github.com/icinga/icinga2/issues/3888) (API): Implement SSL cipher configuration support for the API feature
* [#3829](https://github.com/icinga/icinga2/issues/3829) (Packages): Provide packages for icinga-studio on Fedora
* [#3763](https://github.com/icinga/icinga2/issues/3763): Add name attribute for WorkQueue class
* [#3711](https://github.com/icinga/icinga2/issues/3711) (ITL): icinga2.conf: Include plugins-contrib, manubulon, windows-plugins, nscp by default
* [#3708](https://github.com/icinga/icinga2/issues/3708) (Packages): Firewalld Service definition for Icinga
* [#3683](https://github.com/icinga/icinga2/issues/3683) (ITL): Add IPv4/IPv6 support to the rest of the monitoring-plugins
* [#3612](https://github.com/icinga/icinga2/issues/3612) (Documentation): Update SELinux documentation
* [#3562](https://github.com/icinga/icinga2/issues/3562) (Performance Data): Add InfluxDbWriter feature
* [#3400](https://github.com/icinga/icinga2/issues/3400): Remove the deprecated IcingaStatusWriter feature
* [#3237](https://github.com/icinga/icinga2/issues/3237) (Performance Data): Gelf module: expose 'perfdata' fields for 'CHECK\_RESULT' events
* [#3224](https://github.com/icinga/icinga2/issues/3224) (Configuration): Implement support for formatting date/time
* [#3178](https://github.com/icinga/icinga2/issues/3178) (DB IDO): Add SSL support for the IdoMysqlConnection feature
* [#3012](https://github.com/icinga/icinga2/issues/3012) (ITL): Extend CheckCommand definitions for nscp-local
* [#2970](https://github.com/icinga/icinga2/issues/2970) (Performance Data): Add timestamp support for GelfWriter
* [#2606](https://github.com/icinga/icinga2/issues/2606) (Packages): Package for syntax highlighting
* [#2040](https://github.com/icinga/icinga2/issues/2040): Exclude option for TimePeriod definitions

### Bug

* [#4534](https://github.com/icinga/icinga2/issues/4534) (CLI): Icinga2 segault on startup
* [#4526](https://github.com/icinga/icinga2/issues/4526) (Packages): Revert dependency on firewalld on RHEL
* [#4524](https://github.com/icinga/icinga2/issues/4524) (API): API Remote crash via Google Chrome
* [#4521](https://github.com/icinga/icinga2/issues/4521) (Documentation): Typo in Notification object documentation
* [#4520](https://github.com/icinga/icinga2/issues/4520) (Configuration): Memory leak when using closures
* [#4518](https://github.com/icinga/icinga2/issues/4518) (ITL): ITL uses unsupported arguments for check\_swap on Debian wheezy/Ubuntu trusty
* [#4517](https://github.com/icinga/icinga2/issues/4517) (Documentation): Documentation is missing for the API permissions that are new in 2.5.0
* [#4512](https://github.com/icinga/icinga2/issues/4512) (Cluster): Incorrect certificate validation error message
* [#4511](https://github.com/icinga/icinga2/issues/4511): ClrCheck is null on \*nix
* [#4510](https://github.com/icinga/icinga2/issues/4510) (Documentation): Docs: API example uses wrong attribute name
* [#4505](https://github.com/icinga/icinga2/issues/4505) (CLI): Cannot set ownership for user 'icinga' group 'icinga' on file '/var/lib/icinga2/ca/serial.txt'.
* [#4504](https://github.com/icinga/icinga2/issues/4504) (API): API: events for DowntimeTriggered does not provide needed information
* [#4502](https://github.com/icinga/icinga2/issues/4502) (DB IDO): IDO query fails due to key contraint violation for the icinga\_customvariablestatus table
* [#4501](https://github.com/icinga/icinga2/issues/4501) (Cluster): DB IDO started before daemonizing \(no systemd\)
* [#4500](https://github.com/icinga/icinga2/issues/4500) (DB IDO): Query for customvariablestatus incorrectly updates the host's/service's insert ID
* [#4499](https://github.com/icinga/icinga2/issues/4499) (DB IDO): Insert fails for the icinga\_scheduleddowntime table due to duplicate key
* [#4497](https://github.com/icinga/icinga2/issues/4497): Fix incorrect detection of the 'Concurrency' variable
* [#4496](https://github.com/icinga/icinga2/issues/4496) (API): API: action schedule-downtime requires a duration also when fixed is true
* [#4495](https://github.com/icinga/icinga2/issues/4495): Use hash-based serial numbers for new certificates
* [#4494](https://github.com/icinga/icinga2/issues/4494) (Installation): Remove unused functions from icinga-installer
* [#4490](https://github.com/icinga/icinga2/issues/4490) (Cluster): ClusterEvents::NotificationSentAllUsersAPIHandler\(\) does not set notified\_users
* [#4489](https://github.com/icinga/icinga2/issues/4489) (Documentation): Missing documentation for "legacy-timeperiod" template
* [#4488](https://github.com/icinga/icinga2/issues/4488): Replace GetType\(\)-\>GetName\(\) calls with GetReflectionType\(\)-\>GetName\(\)
* [#4484](https://github.com/icinga/icinga2/issues/4484) (Cluster): Only allow sending command\_endpoint checks to directly connected child zones
* [#4483](https://github.com/icinga/icinga2/issues/4483) (DB IDO): ido CheckCommand returns returns "Could not connect to database server" when HA enabled
* [#4481](https://github.com/icinga/icinga2/issues/4481) (DB IDO): Fix the "ido" check command for use with command\_endpoint
* [#4478](https://github.com/icinga/icinga2/issues/4478): CompatUtility::GetCheckableNotificationStateFilter is returning an incorrect value
* [#4476](https://github.com/icinga/icinga2/issues/4476) (DB IDO): Importing mysql schema fails
* [#4475](https://github.com/icinga/icinga2/issues/4475) (CLI): pki sign-csr does not log where it is writing the certificate file
* [#4472](https://github.com/icinga/icinga2/issues/4472) (DB IDO): IDO marks objects as inactive on shutdown
* [#4471](https://github.com/icinga/icinga2/issues/4471) (DB IDO): IDO does duplicate config updates
* [#4470](https://github.com/icinga/icinga2/issues/4470) (Documentation): The description for the http\_certificate attribute doesn't have the right default value
* [#4466](https://github.com/icinga/icinga2/issues/4466) (Configuration): 'use' keyword cannot be used with templates
* [#4462](https://github.com/icinga/icinga2/issues/4462) (Notifications): Add log message if notifications are forced \(i.e. filters are not checked\)
* [#4461](https://github.com/icinga/icinga2/issues/4461) (Notifications): Notification resent, even if interval = 0
* [#4460](https://github.com/icinga/icinga2/issues/4460) (DB IDO): Fixed downtime start does not update actual\_start\_time
* [#4458](https://github.com/icinga/icinga2/issues/4458): Flexible downtimes should be removed after trigger\_time+duration
* [#4455](https://github.com/icinga/icinga2/issues/4455): Disallow casting "" to an Object
* [#4452](https://github.com/icinga/icinga2/issues/4452) (Packages): Error compiling on windows due to changes in apilistener around minimum tls version
* [#4447](https://github.com/icinga/icinga2/issues/4447): Handle I/O errors while writing the Icinga state file more gracefully
* [#4446](https://github.com/icinga/icinga2/issues/4446) (Notifications): Incorrect downtime notification events
* [#4444](https://github.com/icinga/icinga2/issues/4444): Fix building Icinga with -fvisibility=hidden
* [#4439](https://github.com/icinga/icinga2/issues/4439) (Configuration): Icinga doesn't delete temporary icinga2.debug file when config validation fails
* [#4434](https://github.com/icinga/icinga2/issues/4434) (Notifications): Notification sent too fast when one master fails
* [#4432](https://github.com/icinga/icinga2/issues/4432) (Packages): Windows build broken since ref 11292
* [#4430](https://github.com/icinga/icinga2/issues/4430) (Cluster): Remove obsolete README files in tools/syntax
* [#4427](https://github.com/icinga/icinga2/issues/4427) (Notifications): Missing notification for recovery during downtime
* [#4425](https://github.com/icinga/icinga2/issues/4425) (DB IDO): Change the way outdated comments/downtimes are deleted on restart
* [#4421](https://github.com/icinga/icinga2/issues/4421) (ITL): -q option for check\_ntp\_time is wrong
* [#4420](https://github.com/icinga/icinga2/issues/4420) (Notifications): Multiple notifications when master fails
* [#4419](https://github.com/icinga/icinga2/issues/4419) (Documentation): Incorrect API permission name for /v1/status in the documentation
* [#4418](https://github.com/icinga/icinga2/issues/4418) (DB IDO): icinga2 IDO reload performance significant slower with latest snapshot release
* [#4417](https://github.com/icinga/icinga2/issues/4417) (Notifications): Notification interval mistimed
* [#4413](https://github.com/icinga/icinga2/issues/4413) (DB IDO): icinga2 empties custom variables, host-, servcie- and contactgroup members at the end of IDO database reconnection
* [#4412](https://github.com/icinga/icinga2/issues/4412) (Notifications): Reminder notifications ignore HA mode
* [#4405](https://github.com/icinga/icinga2/issues/4405) (DB IDO): Deprecation warning should include object type and name
* [#4404](https://github.com/icinga/icinga2/issues/4404) (Installation): Increase default systemd timeout
* [#4401](https://github.com/icinga/icinga2/issues/4401) (Performance Data): Incorrect escaping / formatting of perfdata to InfluxDB
* [#4399](https://github.com/icinga/icinga2/issues/4399): Icinga stats min\_execution\_time and max\_execution\_time are invalid
* [#4396](https://github.com/icinga/icinga2/issues/4396) (Documentation): Missing explanation for three level clusters with CSR auto-signing
* [#4395](https://github.com/icinga/icinga2/issues/4395) (Documentation): Incorrect documentation about apply rules in zones.d directories
* [#4394](https://github.com/icinga/icinga2/issues/4394): icinga check reports "-1" for minimum latency and execution time and only uptime has a number but 0
* [#4391](https://github.com/icinga/icinga2/issues/4391) (DB IDO): Do not clear {host,service,contact}group\_members tables on restart
* [#4387](https://github.com/icinga/icinga2/issues/4387) (Documentation): Improve author information about check\_yum
* [#4384](https://github.com/icinga/icinga2/issues/4384) (API): Fix URL encoding for '&'
* [#4380](https://github.com/icinga/icinga2/issues/4380) (Cluster): Increase cluster reconnect interval
* [#4378](https://github.com/icinga/icinga2/issues/4378) (Notifications): Optimize two ObjectLocks into one in Notification::BeginExecuteNotification method
* [#4376](https://github.com/icinga/icinga2/issues/4376) (Cluster): CheckerComponent sometimes fails to schedule checks in time
* [#4375](https://github.com/icinga/icinga2/issues/4375) (Cluster): Duplicate messages for command\_endpoint w/ master and satellite
* [#4372](https://github.com/icinga/icinga2/issues/4372) (API): state\_filters\_real shouldn't be visible in the API
* [#4371](https://github.com/icinga/icinga2/issues/4371) (Notifications): notification.notification\_number runtime attribute returning 0 \(instead of 1\) in first notification e-mail
* [#4370](https://github.com/icinga/icinga2/issues/4370): Test the change with HARD OK transitions
* [#4363](https://github.com/icinga/icinga2/issues/4363) (DB IDO): IDO module starts threads before daemonize
* [#4361](https://github.com/icinga/icinga2/issues/4361) (Documentation): pkg-config is not listed as a build requirement in INSTALL.md
* [#4359](https://github.com/icinga/icinga2/issues/4359) (ITL): ITL: check\_iftraffic64.pl default values, wrong postfix value in CheckCommand
* [#4356](https://github.com/icinga/icinga2/issues/4356) (DB IDO): DB IDO query queue does not clean up with v2.4.10-520-g124c80b
* [#4349](https://github.com/icinga/icinga2/issues/4349) (DB IDO): Add missing index on state history for DB IDO cleanup
* [#4345](https://github.com/icinga/icinga2/issues/4345): Ensure to clear the SSL error queue before calling SSL\_{read,write,do\_handshake}
* [#4344](https://github.com/icinga/icinga2/issues/4344) (Packages): Build fails with Visual Studio 2013
* [#4343](https://github.com/icinga/icinga2/issues/4343) (Configuration): include\_recursive should gracefully handle inaccessible files
* [#4341](https://github.com/icinga/icinga2/issues/4341) (API): Icinga incorrectly disconnects all endpoints if one has a wrong certificate
* [#4340](https://github.com/icinga/icinga2/issues/4340) (DB IDO): deadlock in ido reconnect
* [#4337](https://github.com/icinga/icinga2/issues/4337) (Documentation): Add a note to the docs that API POST updates to custom attributes/groups won't trigger re-evaluation
* [#4333](https://github.com/icinga/icinga2/issues/4333) (Documentation): Documentation: Setting up Plugins section is broken
* [#4329](https://github.com/icinga/icinga2/issues/4329) (Performance Data): Key Escapes in InfluxDB Writer Don't Work
* [#4328](https://github.com/icinga/icinga2/issues/4328) (Documentation): Typo in Manubulon CheckCommand documentation
* [#4327](https://github.com/icinga/icinga2/issues/4327) (Packages): Icinga fails to build with OpenSSL 1.1.0
* [#4318](https://github.com/icinga/icinga2/issues/4318) (Documentation): Migration docs still show unsupported CHANGE\_\*MODATTR external commands
* [#4313](https://github.com/icinga/icinga2/issues/4313) (Configuration): Icinga crashes when using include\_recursive in an object definition
* [#4309](https://github.com/icinga/icinga2/issues/4309) (Configuration): ConfigWriter::EmitScope incorrectly quotes dictionary keys
* [#4306](https://github.com/icinga/icinga2/issues/4306) (Documentation): Add a note about creating Zone/Endpoint objects with the API
* [#4300](https://github.com/icinga/icinga2/issues/4300) (DB IDO): Comment/Downtime delete queries are slow
* [#4299](https://github.com/icinga/icinga2/issues/4299) (Documentation): Incorrect URL for API examples in the documentation
* [#4293](https://github.com/icinga/icinga2/issues/4293) (DB IDO): Overflow in current\_notification\_number column in DB IDO MySQL
* [#4287](https://github.com/icinga/icinga2/issues/4287) (DB IDO): Program status table is not updated in IDO after starting icinga
* [#4283](https://github.com/icinga/icinga2/issues/4283) (Cluster): Icinga 2 satellite crashes
* [#4278](https://github.com/icinga/icinga2/issues/4278) (DB IDO): SOFT state changes with the same state are not logged
* [#4275](https://github.com/icinga/icinga2/issues/4275) (API): Trying to delete an object protected by a permissions filter, ends up deleting all objects that match the filter instead
* [#4274](https://github.com/icinga/icinga2/issues/4274) (Notifications): Duplicate notifications
* [#4265](https://github.com/icinga/icinga2/issues/4265) (Documentation): Improve "Endpoint" documentation
* [#4264](https://github.com/icinga/icinga2/issues/4264) (Performance Data): InfluxWriter doesnt sanitize the data before sending
* [#4263](https://github.com/icinga/icinga2/issues/4263) (Documentation): Fix systemd client command formatting
* [#4259](https://github.com/icinga/icinga2/issues/4259): Flapping Notifications dependent on state change
* [#4258](https://github.com/icinga/icinga2/issues/4258): last SOFT state should be hard \(max\_check\_attempts\)
* [#4257](https://github.com/icinga/icinga2/issues/4257) (Configuration): Incorrect custom variable name in the hosts.conf example config
* [#4255](https://github.com/icinga/icinga2/issues/4255) (Configuration): Config validation should not delete comments/downtimes w/o reference
* [#4254](https://github.com/icinga/icinga2/issues/4254) (ITL): Add "fuse.gvfsd-fuse" to the list of excluded file systems for check\_disk
* [#4244](https://github.com/icinga/icinga2/issues/4244): SOFT OK-state after returning from a soft state
* [#4239](https://github.com/icinga/icinga2/issues/4239) (Notifications): Downtime notifications do not pass author and comment
* [#4238](https://github.com/icinga/icinga2/issues/4238) (Documentation): Missing quotes for API action URL
* [#4234](https://github.com/icinga/icinga2/issues/4234) (Tests): Boost tests are missing a dependency on libmethods
* [#4232](https://github.com/icinga/icinga2/issues/4232): Problems with check scheduling for HARD state changes \(standalone/command\_endpoint\)
* [#4231](https://github.com/icinga/icinga2/issues/4231) (DB IDO): Volatile check results for OK-\>OK transitions are logged into DB IDO statehistory
* [#4230](https://github.com/icinga/icinga2/issues/4230) (Installation): Windows: Error with repository handler \(missing /var/lib/icinga2/api/repository path\)
* [#4217](https://github.com/icinga/icinga2/issues/4217) (Documentation): node setup: Add a note for --endpoint syntax for client-master connection
* [#4211](https://github.com/icinga/icinga2/issues/4211) (Packages): Incorrect filter in pick.py
* [#4187](https://github.com/icinga/icinga2/issues/4187): Icinga 2 client gets killed during network scans
* [#4171](https://github.com/icinga/icinga2/issues/4171) (DB IDO): Outdated downtime/comments not removed from IDO database \(restart\)
* [#4148](https://github.com/icinga/icinga2/issues/4148) (Packages): RPM update starts disabled icinga2 service
* [#4147](https://github.com/icinga/icinga2/issues/4147) (Packages): Reload permission error with SELinux
* [#4134](https://github.com/icinga/icinga2/issues/4134) (Configuration): Don't allow flow control keywords outside of other flow control constructs
* [#4121](https://github.com/icinga/icinga2/issues/4121) (Notifications): notification interval = 0 not honoured in HA clusters
* [#4106](https://github.com/icinga/icinga2/issues/4106) (Notifications): last\_problem\_notification should be synced in HA cluster
* [#4077](https://github.com/icinga/icinga2/issues/4077): Numbers are not properly formatted in runtime macro strings
* [#4002](https://github.com/icinga/icinga2/issues/4002): Don't violate POSIX by ensuring that the argument to usleep\(3\) is less than 1000000 
* [#3954](https://github.com/icinga/icinga2/issues/3954) (Cluster): High load when pinning command endpoint on HA cluster
* [#3949](https://github.com/icinga/icinga2/issues/3949) (DB IDO): IDO: entry\_time of all comments is set to the date and time when Icinga 2 was restarted
* [#3902](https://github.com/icinga/icinga2/issues/3902): Hang in TlsStream::Handshake
* [#3820](https://github.com/icinga/icinga2/issues/3820) (Configuration): High CPU usage with self-referenced parent zone config
* [#3805](https://github.com/icinga/icinga2/issues/3805) (Performance Data): GELF multi-line output
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

## 2.4.9 (2016-05-19)

### Notes

This release fixes a number of issues introduced in 2.4.8.

### Bug

* [#4225](https://github.com/icinga/icinga2/issues/4225) (Compat): Command Pipe thread 100% CPU Usage
* [#4224](https://github.com/icinga/icinga2/issues/4224): Checks are not executed anymore on command
* [#4222](https://github.com/icinga/icinga2/issues/4222) (Configuration): Segfault when trying to start 2.4.8
* [#4221](https://github.com/icinga/icinga2/issues/4221) (Performance Data): Error: Function call 'rename' for file '/var/spool/icinga2/tmp/service-perfdata' failed with error code 2, 'No such file or directory'

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

* [#4205](https://github.com/icinga/icinga2/issues/4205) (Documentation): Add the category to the generated changelog
* [#4203](https://github.com/icinga/icinga2/issues/4203) (Cluster): Only activate HARunOnce objects once there's a cluster connection
* [#4198](https://github.com/icinga/icinga2/issues/4198): Move CalculateExecutionTime and CalculateLatency into the CheckResult class
* [#4196](https://github.com/icinga/icinga2/issues/4196) (Cluster): Remove unused cluster commands
* [#4184](https://github.com/icinga/icinga2/issues/4184) (ITL): 'disk' CheckCommand: Exclude 'cgroup' and 'tracefs' by default
* [#4149](https://github.com/icinga/icinga2/issues/4149) (CLI): Implement SNI support for the CLI commands
* [#4103](https://github.com/icinga/icinga2/issues/4103): Add support for subjectAltName in SSL certificates
* [#3919](https://github.com/icinga/icinga2/issues/3919) (Configuration): Internal check for config problems
* [#3634](https://github.com/icinga/icinga2/issues/3634) (ITL): Provide icingacli in the ITL
* [#3321](https://github.com/icinga/icinga2/issues/3321): "icinga" check should have state WARNING when the last reload failed
* [#2993](https://github.com/icinga/icinga2/issues/2993) (Performance Data): PerfdataWriter: Better failure handling for file renames across file systems
* [#2896](https://github.com/icinga/icinga2/issues/2896) (Cluster): Alert config reload failures with the icinga check 
* [#2468](https://github.com/icinga/icinga2/issues/2468): Maximum concurrent service checks

### Bug

* [#4219](https://github.com/icinga/icinga2/issues/4219) (DB IDO): Postgresql warnings on startup
* [#4212](https://github.com/icinga/icinga2/issues/4212): assertion failed: GetResumeCalled\(\)
* [#4210](https://github.com/icinga/icinga2/issues/4210) (API): Incorrect variable names for joined fields in filters
* [#4204](https://github.com/icinga/icinga2/issues/4204) (DB IDO): Ensure that program status updates are immediately updated in DB IDO
* [#4202](https://github.com/icinga/icinga2/issues/4202) (API): API: Missing error handling for invalid JSON request body
* [#4193](https://github.com/icinga/icinga2/issues/4193) (Documentation): Missing documentation for event commands w/ execution bridge
* [#4182](https://github.com/icinga/icinga2/issues/4182): Crash in UnameHelper
* [#4180](https://github.com/icinga/icinga2/issues/4180): Expired downtimes are not removed
* [#4170](https://github.com/icinga/icinga2/issues/4170) (API): Icinga Crash with the workflow Create\_Host-\> Downtime for the Host -\>  Delete Downtime -\> Remove Host
* [#4146](https://github.com/icinga/icinga2/issues/4146) (Packages): Update chocolatey packages and RELEASE.md
* [#4145](https://github.com/icinga/icinga2/issues/4145) (Configuration): Wrong log severity causes segfault
* [#4144](https://github.com/icinga/icinga2/issues/4144) (Documentation): Incorrect chapter headings for Object\#to\_string and Object\#type
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

## 2.4.7 (2016-04-21)

### Notes

* Bugfixes

### Bug

* [#4142](https://github.com/icinga/icinga2/issues/4142) (DB IDO): Crash in IdoMysqlConnection::ExecuteMultipleQueries

## 2.4.6 (2016-04-20)

### Notes

* Bugfixes

### Enhancement

* [#4141](https://github.com/icinga/icinga2/issues/4141) (Documentation): Update RELEASE.md

### Bug

* [#4140](https://github.com/icinga/icinga2/issues/4140) (DB IDO): Failed assertion in IdoPgsqlConnection::FieldToEscapedString
* [#4139](https://github.com/icinga/icinga2/issues/4139) (Packages): Icinga 2 fails to build on Ubuntu Xenial
* [#4136](https://github.com/icinga/icinga2/issues/4136) (Documentation): Docs: Zone attribute 'endpoints' is an array

## 2.4.5 (2016-04-20)

### Notes

* Windows Installer changed from NSIS to MSI
* New configuration attribute for hosts and services: check_timeout (overrides the CheckCommand's timeout when set)
* ITL updates
* Lots of bugfixes

### Enhancement

* [#4119](https://github.com/icinga/icinga2/issues/4119) (Installation): Update chocolatey uninstall script for the MSI package
* [#4117](https://github.com/icinga/icinga2/issues/4117) (Installation): Make sure to update the agent wizard banner
* [#4073](https://github.com/icinga/icinga2/issues/4073) (Installation): Install 64-bit version of NSClient++ on 64-bit versions of Windows
* [#4072](https://github.com/icinga/icinga2/issues/4072) (Installation): Update NSClient++ to version 0.4.4.19
* [#4064](https://github.com/icinga/icinga2/issues/4064) (Packages): Build 64-bit packages for Windows
* [#4055](https://github.com/icinga/icinga2/issues/4055) (Documentation): Add silent install / reference to NSClient++ to documentation
* [#4039](https://github.com/icinga/icinga2/issues/4039) (Documentation): Update .mailmap for Markus Frosch
* [#3953](https://github.com/icinga/icinga2/issues/3953) (ITL): Add --units, --rate and --rate-multiplier support for the snmpv3 check command
* [#3903](https://github.com/icinga/icinga2/issues/3903) (ITL): Add --method parameter for check\_{oracle,mysql,mssql}\_health CheckCommands
* [#3145](https://github.com/icinga/icinga2/issues/3145) (Documentation): Add Windows setup wizard screenshots
* [#3023](https://github.com/icinga/icinga2/issues/3023) (Configuration): Implement support for overriding check command timeout

### Bug

* [#4131](https://github.com/icinga/icinga2/issues/4131) (Configuration): Vim Syntax Highlighting does not work with assign where
* [#4127](https://github.com/icinga/icinga2/issues/4127) (Installation): Windows installer does not copy "features-enabled" on upgrade
* [#4122](https://github.com/icinga/icinga2/issues/4122) (Documentation): Remove instance\_name from Ido\*Connection example
* [#4118](https://github.com/icinga/icinga2/issues/4118) (Installation): icinga2-installer.exe doesn't wait until NSIS uninstall.exe exits
* [#4116](https://github.com/icinga/icinga2/issues/4116) (API): icinga2 crashes when a command\_endpoint is set, but the api feature is not active
* [#4114](https://github.com/icinga/icinga2/issues/4114): Compiler warning in NotifyActive
* [#4113](https://github.com/icinga/icinga2/issues/4113) (Installation): Package fails to build on \*NIX
* [#4109](https://github.com/icinga/icinga2/issues/4109) (API): Navigation attributes are missing in /v1/objects/\<type\>
* [#4108](https://github.com/icinga/icinga2/issues/4108) (Documentation): Incorrect link in the documentation
* [#4104](https://github.com/icinga/icinga2/issues/4104) (Configuration): Segfault during config validation if host exists, service does not exist any longer and downtime expires
* [#4099](https://github.com/icinga/icinga2/issues/4099) (Installation): make install overwrites configuration files
* [#4095](https://github.com/icinga/icinga2/issues/4095): DowntimesExpireTimerHandler crashes Icinga2 with \<unknown function\>
* [#4089](https://github.com/icinga/icinga2/issues/4089): Make the socket event engine configurable
* [#4080](https://github.com/icinga/icinga2/issues/4080) (Documentation): Update documentation URL for Icinga Web 2
* [#4078](https://github.com/icinga/icinga2/issues/4078) (Configuration): Overwriting global type variables causes crash in ConfigItem::Commit\(\)
* [#4076](https://github.com/icinga/icinga2/issues/4076) (API): API User gets wrongly authenticated \(client\_cn and no password\)
* [#4074](https://github.com/icinga/icinga2/issues/4074) (Installation): FatalError\(\) returns when called before Application.Run
* [#4069](https://github.com/icinga/icinga2/issues/4069) (Installation): Error compiling icinga2 targeted for x64 on Windows
* [#4066](https://github.com/icinga/icinga2/issues/4066): ConfigSync broken from 2.4.3. to 2.4.4 under Windows
* [#4058](https://github.com/icinga/icinga2/issues/4058) (Documentation): Docs: Cluster manual SSL generation formatting is broken
* [#4057](https://github.com/icinga/icinga2/issues/4057) (Documentation): Update the CentOS installation documentation
* [#4056](https://github.com/icinga/icinga2/issues/4056) (CLI): Remove semi-colons in the auto-generated configs
* [#4053](https://github.com/icinga/icinga2/issues/4053) (Installation): Icinga 2 Windows Agent does not honor install path during upgrade
* [#4052](https://github.com/icinga/icinga2/issues/4052) (API): Config validation for Notification objects should check whether the state filters are valid
* [#4043](https://github.com/icinga/icinga2/issues/4043) (Documentation): Docs: Remove the migration script chapter
* [#4041](https://github.com/icinga/icinga2/issues/4041) (Documentation): Explain how to use functions for wildcard matches for arrays and/or dictionaries in assign where expressions
* [#4035](https://github.com/icinga/icinga2/issues/4035) (DB IDO): IDO: historical contact notifications table column notification\_id is off-by-one
* [#4032](https://github.com/icinga/icinga2/issues/4032) (Packages): Remove dependency for .NET 3.5 from the chocolatey package
* [#4031](https://github.com/icinga/icinga2/issues/4031): Downtimes are not always activated/expired on restart
* [#4016](https://github.com/icinga/icinga2/issues/4016): Symlink subfolders not followed/considered for config files
* [#4014](https://github.com/icinga/icinga2/issues/4014): Use retry\_interval instead of check\_interval for first OK -\> NOT-OK state change
* [#3988](https://github.com/icinga/icinga2/issues/3988) (Packages): Incorrect base URL in the icinga-rpm-release packages for Fedora
* [#3973](https://github.com/icinga/icinga2/issues/3973) (Cluster): Downtimes and Comments are not synced to child zones
* [#3970](https://github.com/icinga/icinga2/issues/3970) (API): Socket Exceptions \(Operation not permitted\) while reading from API
* [#3907](https://github.com/icinga/icinga2/issues/3907) (Configuration): Too many assign where filters cause stack overflow
* [#3780](https://github.com/icinga/icinga2/issues/3780) (DB IDO): DB IDO: downtime is not in effect after restart
* [#3658](https://github.com/icinga/icinga2/issues/3658) (Packages): Add application manifest for the Windows agent wizard
* [#2998](https://github.com/icinga/icinga2/issues/2998) (Installation): logrotate fails since the "su" directive was removed

## 2.4.4 (2016-03-16)

### Notes

* Bugfixes

### Enhancement

* [#3958](https://github.com/icinga/icinga2/issues/3958) (ITL): Add "query" option to check\_postgres command.
* [#3484](https://github.com/icinga/icinga2/issues/3484) (ITL): ITL: Allow to enforce specific SSL versions using the http check command

### Bug

* [#4036](https://github.com/icinga/icinga2/issues/4036) (CLI): Add the executed cli command to the Windows wizard error messages
* [#4033](https://github.com/icinga/icinga2/issues/4033) (Documentation): Update development docs to use 'thread apply all bt full'
* [#4027](https://github.com/icinga/icinga2/issues/4027) (Packages): Chocolatey package is missing uninstall function
* [#4019](https://github.com/icinga/icinga2/issues/4019) (Configuration): Segmentation fault during 'icinga2 daemon -C'
* [#4018](https://github.com/icinga/icinga2/issues/4018) (Documentation): Docs: Add API examples for creating services and check commands
* [#4017](https://github.com/icinga/icinga2/issues/4017) (CLI): 'icinga2 feature list' fails when all features are disabled
* [#4011](https://github.com/icinga/icinga2/issues/4011) (Packages): Update build requirements for SLES 11 SP4
* [#4009](https://github.com/icinga/icinga2/issues/4009) (Documentation): Typo in API docs
* [#4008](https://github.com/icinga/icinga2/issues/4008) (Configuration): Windows wizard error "too many arguments"
* [#4006](https://github.com/icinga/icinga2/issues/4006): Volatile transitions from HARD NOT-OK-\>NOT-OK do not trigger notifications
* [#3996](https://github.com/icinga/icinga2/issues/3996): epoll\_ctl might cause oops on Ubuntu trusty
* [#3990](https://github.com/icinga/icinga2/issues/3990): Services status updated multiple times within check\_interval even though no retry was triggered
* [#3987](https://github.com/icinga/icinga2/issues/3987): Incorrect check interval when passive check results are used
* [#3985](https://github.com/icinga/icinga2/issues/3985): Active checks are executed even though passive results are submitted
* [#3981](https://github.com/icinga/icinga2/issues/3981): DEL\_DOWNTIME\_BY\_HOST\_NAME does not accept optional arguments
* [#3961](https://github.com/icinga/icinga2/issues/3961) (CLI): Wrong log message for trusted cert in node setup command
* [#3960](https://github.com/icinga/icinga2/issues/3960) (Installation): CMake does not find MySQL libraries on Windows
* [#3939](https://github.com/icinga/icinga2/issues/3939) (CLI): Common name in node wizard isn't case sensitive
* [#3908](https://github.com/icinga/icinga2/issues/3908) (ITL): ITL: Missing documentation for nwc\_health "mode" parameter
* [#3845](https://github.com/icinga/icinga2/issues/3845) (Documentation): Explain how to join hosts/services for /v1/objects/comments
* [#3755](https://github.com/icinga/icinga2/issues/3755) (Documentation): http check's URI is really just Path
* [#3745](https://github.com/icinga/icinga2/issues/3745) (API): Status code 200 even if an object could not be deleted.
* [#3742](https://github.com/icinga/icinga2/issues/3742) (DB IDO): DB IDO: User notification type filters are incorrect
* [#3442](https://github.com/icinga/icinga2/issues/3442) (API): MkDirP not working on Windows
* [#3439](https://github.com/icinga/icinga2/issues/3439) (Notifications): Host notification type is PROBLEM but should be RECOVERY
* [#3303](https://github.com/icinga/icinga2/issues/3303) (Notifications): Problem notifications while Flapping is active
* [#3153](https://github.com/icinga/icinga2/issues/3153) (Notifications): Flapping notifications are sent for hosts/services which are in a downtime

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

* [#3927](https://github.com/icinga/icinga2/issues/3927) (ITL): Checkcommand Disk : Option Freespace-ignore-reserved
* [#3878](https://github.com/icinga/icinga2/issues/3878) (Configuration): Add String\#trim
* [#3857](https://github.com/icinga/icinga2/issues/3857) (Cluster): Support TLSv1.1 and TLSv1.2 for the cluster transport encryption
* [#3836](https://github.com/icinga/icinga2/issues/3836) (Documentation): Troubleshooting: Explain how to fetch the executed command 
* [#3826](https://github.com/icinga/icinga2/issues/3826) (Documentation): Add example how to use custom functions in attributes
* [#3810](https://github.com/icinga/icinga2/issues/3810) (Plugins): Add Timeout parameter to snmpv3 check
* [#3793](https://github.com/icinga/icinga2/issues/3793) (Documentation): "setting up check plugins" section should be enhanced with package manager examples
* [#3785](https://github.com/icinga/icinga2/issues/3785) (DB IDO): Log DB IDO query queue stats
* [#3784](https://github.com/icinga/icinga2/issues/3784) (DB IDO): DB IDO: Add a log message when the connection handling is completed
* [#3760](https://github.com/icinga/icinga2/issues/3760) (Configuration): Raise a config error for "Checkable" objects in global zones
* [#3754](https://github.com/icinga/icinga2/issues/3754) (Plugins): Add "-x" parameter in command definition for disk-windows CheckCommand
* [#3747](https://github.com/icinga/icinga2/issues/3747) (ITL): Add check\_iostat to ITL
* [#3679](https://github.com/icinga/icinga2/issues/3679) (Installation): Add CMake flag for disabling the unit tests

### Bug

* [#3957](https://github.com/icinga/icinga2/issues/3957) (CLI): "node setup" tries to chown\(\) files before they're created
* [#3947](https://github.com/icinga/icinga2/issues/3947): CentOS 5 doesn't support epoll\_create1
* [#3946](https://github.com/icinga/icinga2/issues/3946) (Documentation): Documentation: Unescaped pipe character in tables
* [#3922](https://github.com/icinga/icinga2/issues/3922) (Configuration): YYYY-MM-DD time specs are parsed incorrectly
* [#3915](https://github.com/icinga/icinga2/issues/3915) (API): Connections are not cleaned up properly
* [#3913](https://github.com/icinga/icinga2/issues/3913) (Cluster): Cluster WQ thread dies after fork\(\)
* [#3910](https://github.com/icinga/icinga2/issues/3910): Clean up unused variables a bit
* [#3905](https://github.com/icinga/icinga2/issues/3905) (DB IDO): Problem with hostgroup\_members table cleanup
* [#3900](https://github.com/icinga/icinga2/issues/3900) (Packages): Windows build fails on InterlockedIncrement type
* [#3898](https://github.com/icinga/icinga2/issues/3898) (API): API queries on non-existant objects cause exception
* [#3897](https://github.com/icinga/icinga2/issues/3897) (Configuration): Crash in ConfigItem::RunWithActivationContext
* [#3896](https://github.com/icinga/icinga2/issues/3896) (Cluster): Ensure that config sync updates are always sent on reconnect
* [#3893](https://github.com/icinga/icinga2/issues/3893) (Documentation): Outdated link to icingaweb2-module-nagvis
* [#3892](https://github.com/icinga/icinga2/issues/3892) (Documentation): Partially missing escaping in doc/7-icinga-template-library.md
* [#3889](https://github.com/icinga/icinga2/issues/3889) (DB IDO): Deleting an object via API does not disable it in DB IDO
* [#3871](https://github.com/icinga/icinga2/issues/3871) (Cluster): Master reloads with agents generate false alarms
* [#3870](https://github.com/icinga/icinga2/issues/3870) (DB IDO): next\_check noise in the IDO
* [#3866](https://github.com/icinga/icinga2/issues/3866) (Cluster): Check event duplication with parallel connections involved
* [#3863](https://github.com/icinga/icinga2/issues/3863) (Cluster): Segfault in ApiListener::ConfigUpdateObjectAPIHandler
* [#3861](https://github.com/icinga/icinga2/issues/3861) (Documentation): Incorrect IdoPgSqlConnection Example in Documentation
* [#3859](https://github.com/icinga/icinga2/issues/3859): Stream buffer size is 512 bytes, could be raised
* [#3858](https://github.com/icinga/icinga2/issues/3858) (CLI): Escaped sequences not properly generated with 'node update-config'
* [#3850](https://github.com/icinga/icinga2/issues/3850) (Documentation): Incorrect name in AUTHORS
* [#3848](https://github.com/icinga/icinga2/issues/3848) (Configuration): Mistake in mongodb command definition \(mongodb\_replicaset\)
* [#3843](https://github.com/icinga/icinga2/issues/3843): Modified attributes do not work for the IcingaApplication object w/ external commands
* [#3838](https://github.com/icinga/icinga2/issues/3838) (Installation): Race condition when using systemd unit file
* [#3835](https://github.com/icinga/icinga2/issues/3835) (Cluster): high load and memory consumption on icinga2 agent v2.4.1
* [#3833](https://github.com/icinga/icinga2/issues/3833) (Documentation): Better explaination for array values in "disk" CheckCommand docs
* [#3832](https://github.com/icinga/icinga2/issues/3832) (Installation): Compiler warnings in lib/remote/base64.cpp
* [#3827](https://github.com/icinga/icinga2/issues/3827) (Configuration): Icinga state file corruption with temporary file creation
* [#3818](https://github.com/icinga/icinga2/issues/3818) (Installation): Logrotate on systemd distros should use systemctl not service
* [#3817](https://github.com/icinga/icinga2/issues/3817) (Cluster): Cluster config sync: Ensure that /var/lib/icinga2/api/zones/\* exists
* [#3816](https://github.com/icinga/icinga2/issues/3816) (Cluster): Exception stack trace on icinga2 client when the master reloads the configuration
* [#3812](https://github.com/icinga/icinga2/issues/3812) (API): API actions: Decide whether fixed: false is the right default
* [#3808](https://github.com/icinga/icinga2/issues/3808) (Documentation): Typos in the "troubleshooting" section of the documentation
* [#3798](https://github.com/icinga/icinga2/issues/3798) (DB IDO): is\_active in IDO is only re-enabled on "every second" restart
* [#3797](https://github.com/icinga/icinga2/issues/3797): Remove superfluous \#ifdef
* [#3794](https://github.com/icinga/icinga2/issues/3794) (DB IDO): Icinga2 crashes in IDO when removing a comment
* [#3787](https://github.com/icinga/icinga2/issues/3787) (CLI): "repository add" cli command writes invalid "type" attribute
* [#3786](https://github.com/icinga/icinga2/issues/3786) (DB IDO): Evaluate if CanExecuteQuery/FieldToEscapedString lead to exceptions on !m\_Connected
* [#3783](https://github.com/icinga/icinga2/issues/3783) (DB IDO): Implement support for re-ordering groups of IDO queries
* [#3781](https://github.com/icinga/icinga2/issues/3781) (Documentation): Formatting problem in "Advanced Filter" chapter
* [#3775](https://github.com/icinga/icinga2/issues/3775) (Configuration): Config validation doesn't fail when templates are used as object names
* [#3774](https://github.com/icinga/icinga2/issues/3774) (DB IDO): IDO breaks when writing to icinga\_programstatus with latest snapshots
* [#3773](https://github.com/icinga/icinga2/issues/3773) (Configuration): Relative path in include\_zones does not work
* [#3771](https://github.com/icinga/icinga2/issues/3771) (Installation): Build error with older CMake versions on VERSION\_LESS compare
* [#3770](https://github.com/icinga/icinga2/issues/3770) (Documentation): Missing documentation for API packages zones.d config sync 
* [#3769](https://github.com/icinga/icinga2/issues/3769) (Packages): Windows build fails with latest git master
* [#3766](https://github.com/icinga/icinga2/issues/3766) (API): Cluster config sync ignores zones.d from API packages
* [#3765](https://github.com/icinga/icinga2/issues/3765): Use NodeName in null and random checks
* [#3764](https://github.com/icinga/icinga2/issues/3764) (DB IDO): Failed IDO query for icinga\_downtimehistory
* [#3759](https://github.com/icinga/icinga2/issues/3759) (Documentation): Missing SUSE repository for monitoring plugins documentation
* [#3752](https://github.com/icinga/icinga2/issues/3752): Incorrect information in --version on Linux
* [#3749](https://github.com/icinga/icinga2/issues/3749) (ITL): The hpasm check command is using the PluginDir constant
* [#3748](https://github.com/icinga/icinga2/issues/3748) (Documentation): Wrong postgresql-setup initdb command for RHEL7
* [#3746](https://github.com/icinga/icinga2/issues/3746) (Packages): chcon partial context error in safe-reload prevents reload 
* [#3741](https://github.com/icinga/icinga2/issues/3741) (DB IDO): Avoid duplicate config and status updates on startup
* [#3735](https://github.com/icinga/icinga2/issues/3735) (Configuration): Disallow lambda expressions where side-effect-free expressions are not allowed
* [#3730](https://github.com/icinga/icinga2/issues/3730): Missing path in mkdir\(\) exceptions
* [#3729](https://github.com/icinga/icinga2/issues/3729) (ITL): ITL check command possibly mistyped variable names
* [#3728](https://github.com/icinga/icinga2/issues/3728) (DB IDO): build of icinga2 with gcc 4.4.7 segfaulting with ido
* [#3723](https://github.com/icinga/icinga2/issues/3723) (Installation): Crash on startup with incorrect directory permissions
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
* [#3550](https://github.com/icinga/icinga2/issues/3550) (Documentation): A PgSQL DB for the IDO can't be created w/ UTF8
* [#3549](https://github.com/icinga/icinga2/issues/3549) (Documentation): Incorrect SQL command for creating the user of the PostgreSQL DB for the IDO
* [#3540](https://github.com/icinga/icinga2/issues/3540) (Livestatus): Livestatus log query - filter "class" yields empty results
* [#3440](https://github.com/icinga/icinga2/issues/3440): Icinga2 reload timeout results in killing old and new process because of systemd
* [#2866](https://github.com/icinga/icinga2/issues/2866) (DB IDO): DB IDO: notification\_id for contact notifications is out of range
* [#2746](https://github.com/icinga/icinga2/issues/2746) (DB IDO): Add priority queue for disconnect/programstatus update events 
* [#2009](https://github.com/icinga/icinga2/issues/2009): Re-checks scheduling w/ retry\_interval

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
* [#3691](https://github.com/icinga/icinga2/issues/3691) (ITL): Add running\_kernel\_use\_sudo option for the running\_kernel check
* [#3657](https://github.com/icinga/icinga2/issues/3657) (ITL): Add by\_ssh\_options argument for the check\_by\_ssh plugin

### Bug

* [#3710](https://github.com/icinga/icinga2/issues/3710) (CLI): Remove --master\_zone from --help because it is currently not implemented
* [#3701](https://github.com/icinga/icinga2/issues/3701) (Documentation): Incorrect path for icinga2 binary in development documentation
* [#3699](https://github.com/icinga/icinga2/issues/3699) (Installation): Windows setup wizard crashes when InstallDir registry key is not set
* [#3690](https://github.com/icinga/icinga2/issues/3690) (Documentation): Fix typos in the documentation
* [#3689](https://github.com/icinga/icinga2/issues/3689) (CLI): CLI command 'repository add' doesn't work
* [#3685](https://github.com/icinga/icinga2/issues/3685) (CLI): node wizard checks for /var/lib/icinga2/ca directory but not the files
* [#3682](https://github.com/icinga/icinga2/issues/3682) (ITL): Indentation in command-plugins.conf
* [#3680](https://github.com/icinga/icinga2/issues/3680) (Installation): Incorrect redirect for stderr in /usr/lib/icinga2/prepare-dirs
* [#3674](https://github.com/icinga/icinga2/issues/3674): lib/base/process.cpp SIGSEGV on Debian squeeze / RHEL 6
* [#3673](https://github.com/icinga/icinga2/issues/3673) (Documentation): Documentation for schedule-downtime is missing required paremeters
* [#3671](https://github.com/icinga/icinga2/issues/3671) (API): Icinga 2 crashes when ScheduledDowntime objects are used
* [#3670](https://github.com/icinga/icinga2/issues/3670) (CLI): API setup command incorrectly overwrites existing certificates
* [#3665](https://github.com/icinga/icinga2/issues/3665) (CLI): "node wizard" does not ask user to verify SSL certificate
* [#3656](https://github.com/icinga/icinga2/issues/3656) (Packages): Build fails on SLES 11 SP3 with GCC 4.8
* [#3594](https://github.com/icinga/icinga2/issues/3594) (Documentation): Documentation example in "Access Object Attributes at Runtime" doesn't work correctly
* [#3391](https://github.com/icinga/icinga2/issues/3391) (Documentation): Incorrect web inject URL in documentation

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

* [#3663](https://github.com/icinga/icinga2/issues/3663) (Documentation): Update wxWidgets documentation for Icinga Studio
* [#3642](https://github.com/icinga/icinga2/issues/3642): Release 2.4.0
* [#3640](https://github.com/icinga/icinga2/issues/3640) (Documentation): Explain DELETE for config stages/packages
* [#3631](https://github.com/icinga/icinga2/issues/3631) (Documentation): Documentation for the script debugger
* [#3630](https://github.com/icinga/icinga2/issues/3630) (Documentation): Explain variable names for joined objects in filter expressions
* [#3629](https://github.com/icinga/icinga2/issues/3629) (Documentation): Documentation for /v1/console
* [#3628](https://github.com/icinga/icinga2/issues/3628) (Documentation): Mention wxWidget \(optional\) requirement in INSTALL.md
* [#3624](https://github.com/icinga/icinga2/issues/3624) (API): Enhance programmatic examples for the API docs
* [#3611](https://github.com/icinga/icinga2/issues/3611) (API): Change object query result set
* [#3609](https://github.com/icinga/icinga2/issues/3609) (API): Change 'api setup' into a manual step while configuring the API
* [#3608](https://github.com/icinga/icinga2/issues/3608) (CLI): Icinga 2 script debugger
* [#3591](https://github.com/icinga/icinga2/issues/3591) (CLI): Change output format for 'icinga2 console'
* [#3584](https://github.com/icinga/icinga2/issues/3584) (ITL): Add ipv4/ipv6 only to tcp and http CheckCommand
* [#3582](https://github.com/icinga/icinga2/issues/3582) (ITL): Add check command mysql
* [#3580](https://github.com/icinga/icinga2/issues/3580): Change GetLastStateUp/Down to host attributes
* [#3578](https://github.com/icinga/icinga2/issues/3578) (ITL): Add check command negate
* [#3576](https://github.com/icinga/icinga2/issues/3576) (Plugins): Missing parameters for check jmx4perl
* [#3563](https://github.com/icinga/icinga2/issues/3563) (Documentation): Documentation: Reorganize Livestatus and alternative frontends
* [#3561](https://github.com/icinga/icinga2/issues/3561) (CLI): Use ZoneName variable for parent\_zone in node update-config
* [#3537](https://github.com/icinga/icinga2/issues/3537) (CLI): Rewrite man page
* [#3531](https://github.com/icinga/icinga2/issues/3531) (DB IDO): Add the name for comments/downtimes next to legacy\_id to DB IDO
* [#3525](https://github.com/icinga/icinga2/issues/3525) (ITL): Ability to set port on SNMP Checks
* [#3516](https://github.com/icinga/icinga2/issues/3516) (Documentation): Add documentation for apply+for in the language reference chapter
* [#3515](https://github.com/icinga/icinga2/issues/3515): Remove api.cpp, api.hpp 
* [#3508](https://github.com/icinga/icinga2/issues/3508) (Cluster): Add getter for endpoint 'connected' attribute
* [#3507](https://github.com/icinga/icinga2/issues/3507) (API): Hide internal attributes
* [#3506](https://github.com/icinga/icinga2/issues/3506) (API): Original attributes list in IDO
* [#3503](https://github.com/icinga/icinga2/issues/3503) (API): Log a warning message on unauthorized http request
* [#3502](https://github.com/icinga/icinga2/issues/3502) (API): Use the API for "icinga2 console"
* [#3500](https://github.com/icinga/icinga2/issues/3500) (Documentation): Add 'support' tracker to changelog.py
* [#3498](https://github.com/icinga/icinga2/issues/3498) (DB IDO): DB IDO should provide its connected state via /v1/status
* [#3490](https://github.com/icinga/icinga2/issues/3490) (ITL): Add check command nginx\_status
* [#3488](https://github.com/icinga/icinga2/issues/3488) (API): Document that modified attributes require accept\_config for cluster/clients
* [#3469](https://github.com/icinga/icinga2/issues/3469) (Configuration): Pretty-print arrays and dictionaries when converting them to strings
* [#3463](https://github.com/icinga/icinga2/issues/3463) (API): Change object version to timestamps for diff updates on config sync
* [#3452](https://github.com/icinga/icinga2/issues/3452) (Configuration): Provide keywords to retrieve the current file name at parse time
* [#3435](https://github.com/icinga/icinga2/issues/3435) (API): Move /v1/\<type\> to /v1/objects/\<type\>
* [#3432](https://github.com/icinga/icinga2/issues/3432) (API): Rename statusqueryhandler to objectqueryhandler
* [#3426](https://github.com/icinga/icinga2/issues/3426) (Documentation): Add documentation for api-users.conf and app.conf
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
* [#3281](https://github.com/icinga/icinga2/issues/3281) (Documentation): Document Object\#clone
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
* [#3063](https://github.com/icinga/icinga2/issues/3063) (Installation): "-Wno-deprecated-register" compiler option breaks builds on SLES 11
* [#3025](https://github.com/icinga/icinga2/issues/3025) (DB IDO): DB IDO/Livestatus: Add zone object table w/ endpoint members
* [#2964](https://github.com/icinga/icinga2/issues/2964) (ITL): Move 'running\_kernel' check command to plugins-contrib 'operating system' section
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
* [#2784](https://github.com/icinga/icinga2/issues/2784) (ITL): Move the base command templates into libmethods
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
* [#3662](https://github.com/icinga/icinga2/issues/3662) (Packages): Download URL for NSClient++ is incorrect
* [#3649](https://github.com/icinga/icinga2/issues/3649) (DB IDO): Group memberships are not updated for runtime created objects
* [#3648](https://github.com/icinga/icinga2/issues/3648) (API): API overwrites \(and then deletes\) config file when trying to create an object that already exists
* [#3647](https://github.com/icinga/icinga2/issues/3647) (API): Don't allow users to set state attributes via PUT
* [#3645](https://github.com/icinga/icinga2/issues/3645): Deadlock in MacroProcessor::EvaluateFunction
* [#3638](https://github.com/icinga/icinga2/issues/3638) (Documentation): Documentation for /v1/types
* [#3635](https://github.com/icinga/icinga2/issues/3635): modify\_attribute: object cannot be cloned
* [#3633](https://github.com/icinga/icinga2/issues/3633) (API): Detailed error message is missing when object creation via API fails
* [#3632](https://github.com/icinga/icinga2/issues/3632) (API): API call doesn't fail when trying to use a template that doesn't exist
* [#3626](https://github.com/icinga/icinga2/issues/3626) (Documentation): Icinga 2 API Docs
* [#3625](https://github.com/icinga/icinga2/issues/3625): Improve location information for errors in API filters
* [#3622](https://github.com/icinga/icinga2/issues/3622) (API): /v1/console should only use a single permission
* [#3621](https://github.com/icinga/icinga2/issues/3621) (Documentation): Documentation should not reference real host names
* [#3620](https://github.com/icinga/icinga2/issues/3620) (API): 'remove-comment' action does not support filters
* [#3619](https://github.com/icinga/icinga2/issues/3619) (CLI): 'api setup' should create a user even when api feature is already enabled
* [#3618](https://github.com/icinga/icinga2/issues/3618) (CLI): Autocompletion doesn't work in the debugger
* [#3617](https://github.com/icinga/icinga2/issues/3617) (API): There's a variable called 'string' in filter expressions
* [#3615](https://github.com/icinga/icinga2/issues/3615) (Packages): Update OpenSSL for the Windows builds
* [#3614](https://github.com/icinga/icinga2/issues/3614) (Installation): Don't try to use --gc-sections on Solaris
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
* [#3547](https://github.com/icinga/icinga2/issues/3547) (Documentation): Incorrect attribute name in the documentation
* [#3546](https://github.com/icinga/icinga2/issues/3546) (Cluster): Improve error handling during log replay
* [#3536](https://github.com/icinga/icinga2/issues/3536) (CLI): Improve --help output for the --log-level option
* [#3535](https://github.com/icinga/icinga2/issues/3535) (CLI): "Command options" is empty when executing icinga2 without any argument.
* [#3534](https://github.com/icinga/icinga2/issues/3534) (DB IDO): Custom variables aren't removed from the IDO database
* [#3532](https://github.com/icinga/icinga2/issues/3532) (ITL): 'dig\_lookup' custom attribute for the 'dig' check command isn't optional
* [#3524](https://github.com/icinga/icinga2/issues/3524) (DB IDO): Changing a group's attributes causes duplicate rows in the icinga\_\*group\_members table
* [#3522](https://github.com/icinga/icinga2/issues/3522) (Packages): 'which' isn't available in a minimal CentOS container
* [#3517](https://github.com/icinga/icinga2/issues/3517): OpenBSD: hang during ConfigItem::ActivateItems\(\) in daemon startup
* [#3514](https://github.com/icinga/icinga2/issues/3514) (CLI): Misleading wording in generated zones.conf
* [#3511](https://github.com/icinga/icinga2/issues/3511) (Documentation): Escaping $ not documented
* [#3501](https://github.com/icinga/icinga2/issues/3501) (API): restore\_attribute does not work in clusters
* [#3489](https://github.com/icinga/icinga2/issues/3489) (API): Ensure that modified attributes work with clients with local config and no zone attribute
* [#3485](https://github.com/icinga/icinga2/issues/3485) (API): Icinga2 API performance regression
* [#3482](https://github.com/icinga/icinga2/issues/3482) (API): Version updates are not working properly
* [#3477](https://github.com/icinga/icinga2/issues/3477) (Documentation): Remove duplicated text in section "Apply Notifications to Hosts and Services"
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
* [#2893](https://github.com/icinga/icinga2/issues/2893) (Installation): icinga demo module can not be built
* [#2884](https://github.com/icinga/icinga2/issues/2884) (DB IDO): PostgreSQL schema sets default timestamps w/o time zone
* [#2879](https://github.com/icinga/icinga2/issues/2879): Compiler warnings with latest HEAD 5ac5f98
* [#2870](https://github.com/icinga/icinga2/issues/2870) (DB IDO): pgsql driver does not have latest mysql changes synced
* [#2863](https://github.com/icinga/icinga2/issues/2863) (Configuration): Crash in VMOps::FunctionCall
* [#2858](https://github.com/icinga/icinga2/issues/2858) (Packages): Specify pidfile for status\_of\_proc in the init script
* [#2850](https://github.com/icinga/icinga2/issues/2850) (Configuration): Validation fails even though field is not required 
* [#2824](https://github.com/icinga/icinga2/issues/2824) (DB IDO): Failed assertion in IdoMysqlConnection::FieldToEscapedString  
* [#2808](https://github.com/icinga/icinga2/issues/2808) (Configuration): Make default notifications include users from host.vars.notification.mail.users
* [#2803](https://github.com/icinga/icinga2/issues/2803): Don't allow users to instantiate the StreamLogger class
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
* [#3479](https://github.com/icinga/icinga2/issues/3479) (Documentation): Improve timeperiod documentation
* [#3437](https://github.com/icinga/icinga2/issues/3437) (ITL): Add timeout argument for pop, spop, imap, simap commands
* [#3436](https://github.com/icinga/icinga2/issues/3436) (Documentation): Clarify on cluster/client naming convention and add troubleshooting section
* [#3430](https://github.com/icinga/icinga2/issues/3430) (Documentation): Find a better description for cluster communication requirements
* [#3421](https://github.com/icinga/icinga2/issues/3421): Implement the Array\#reverse and String\#reverse methods
* [#3408](https://github.com/icinga/icinga2/issues/3408) (Documentation): Improve documentation for check\_memory
* [#3407](https://github.com/icinga/icinga2/issues/3407) (ITL): Make check\_disk.exe CheckCommand Config more verbose
* [#3406](https://github.com/icinga/icinga2/issues/3406) (Documentation): Update graphing section in the docs
* [#3402](https://github.com/icinga/icinga2/issues/3402) (Documentation): Update debug docs for core dumps and full backtraces
* [#3399](https://github.com/icinga/icinga2/issues/3399) (ITL): expand check command dig
* [#3394](https://github.com/icinga/icinga2/issues/3394) (ITL): Add ipv4/ipv6 only to nrpe CheckCommand
* [#3385](https://github.com/icinga/icinga2/issues/3385) (ITL): Add check command pgsql
* [#3382](https://github.com/icinga/icinga2/issues/3382) (ITL): Add check command squid
* [#3351](https://github.com/icinga/icinga2/issues/3351) (Documentation): Command Execution Bridge: Use of same endpoint names in examples for a better understanding
* [#3327](https://github.com/icinga/icinga2/issues/3327): Implement a way for users to resolve commands+arguments in the same way Icinga does
* [#3326](https://github.com/icinga/icinga2/issues/3326): escape\_shell\_arg\(\) method
* [#3235](https://github.com/icinga/icinga2/issues/3235) (ITL): check\_command for plugin check\_hpasm
* [#3214](https://github.com/icinga/icinga2/issues/3214) (ITL): add check command for check\_nwc\_health
* [#3092](https://github.com/icinga/icinga2/issues/3092) (Documentation): Add FreeBSD setup to getting started
* [#2969](https://github.com/icinga/icinga2/issues/2969) (Performance Data): Add timestamp support for OpenTsdbWriter

### Bug

* [#3492](https://github.com/icinga/icinga2/issues/3492) (Cluster): Wrong connection log message for global zones
* [#3491](https://github.com/icinga/icinga2/issues/3491): cidr\_match\(\) doesn't properly validate IP addresses
* [#3487](https://github.com/icinga/icinga2/issues/3487) (Cluster): ApiListener::SyncRelayMessage doesn't send message to all zone members
* [#3478](https://github.com/icinga/icinga2/issues/3478) (Documentation): Broken table layout in chapter 20
* [#3476](https://github.com/icinga/icinga2/issues/3476) (Compat): Missing Start call for base class in CheckResultReader
* [#3475](https://github.com/icinga/icinga2/issues/3475) (Compat): Checkresultreader is unable to process host checks
* [#3466](https://github.com/icinga/icinga2/issues/3466): "Not after" value overflows in X509 certificates on RHEL5
* [#3464](https://github.com/icinga/icinga2/issues/3464) (Cluster): Don't log messages we've already relayed to all relevant zones
* [#3460](https://github.com/icinga/icinga2/issues/3460) (Performance Data): Performance Data Labels including '=' will not be displayed correct
* [#3454](https://github.com/icinga/icinga2/issues/3454): Percent character whitespace on Windows
* [#3449](https://github.com/icinga/icinga2/issues/3449) (Cluster): Don't throw an exception when replaying the current replay log file
* [#3446](https://github.com/icinga/icinga2/issues/3446): Deadlock in TlsStream::Close
* [#3428](https://github.com/icinga/icinga2/issues/3428) (Configuration): config checker reports wrong error on apply for rules
* [#3427](https://github.com/icinga/icinga2/issues/3427) (Configuration): Config parser problem with parenthesis and newlines 
* [#3423](https://github.com/icinga/icinga2/issues/3423) (Configuration): Remove unnecessary MakeLiteral calls in SetExpression::DoEvaluate
* [#3417](https://github.com/icinga/icinga2/issues/3417) (Configuration): null + null should not be ""
* [#3416](https://github.com/icinga/icinga2/issues/3416) (API): Problem with customvariable table update/insert queries
* [#3409](https://github.com/icinga/icinga2/issues/3409) (Documentation): Windows Check Update -\> Access denied
* [#3379](https://github.com/icinga/icinga2/issues/3379) (Installation): Rather use unique SID when granting rights for folders in NSIS on Windows Client
* [#3373](https://github.com/icinga/icinga2/issues/3373) (Livestatus): Improve error message for socket errors in Livestatus
* [#3324](https://github.com/icinga/icinga2/issues/3324) (Cluster): Deadlock in WorkQueue::Enqueue
* [#3204](https://github.com/icinga/icinga2/issues/3204) (Configuration): String methods cannot be invoked on an empty string
* [#3045](https://github.com/icinga/icinga2/issues/3045) (Packages): icinga2 ido mysql misspelled database username
* [#3038](https://github.com/icinga/icinga2/issues/3038) (Livestatus): sending multiple Livestatus commands rejects all except the first
* [#2568](https://github.com/icinga/icinga2/issues/2568) (Cluster): check cluster-zone returns wrong log lag

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

* [#3320](https://github.com/icinga/icinga2/issues/3320) (ITL): Add new arguments openvmtools for Open VM Tools
* [#3313](https://github.com/icinga/icinga2/issues/3313) (ITL): add check command nscp-local-counter
* [#3254](https://github.com/icinga/icinga2/issues/3254) (Livestatus): Use an empty dictionary for the 'this' scope when executing commands with Livestatus
* [#3253](https://github.com/icinga/icinga2/issues/3253): Implement the Dictionary\#keys method
* [#3219](https://github.com/icinga/icinga2/issues/3219) (ITL): snmpv3 CheckCommand section improved
* [#3213](https://github.com/icinga/icinga2/issues/3213) (ITL): add check command for check\_mailq
* [#3208](https://github.com/icinga/icinga2/issues/3208) (ITL): Add check\_jmx4perl to ITL
* [#3206](https://github.com/icinga/icinga2/issues/3206): Implement Dictionary\#get and Array\#get
* [#3186](https://github.com/icinga/icinga2/issues/3186) (ITL): check\_command for plugin check\_clamd
* [#3170](https://github.com/icinga/icinga2/issues/3170) (Configuration): Adding "-r" parameter to the check\_load command for dividing the load averages by the number of CPUs.
* [#3166](https://github.com/icinga/icinga2/issues/3166) (Documentation): Update gdb pretty printer docs w/ Python 3
* [#3164](https://github.com/icinga/icinga2/issues/3164) (ITL): Add check\_redis to ITL
* [#3162](https://github.com/icinga/icinga2/issues/3162) (ITL): Add check\_yum to ITL
* [#3111](https://github.com/icinga/icinga2/issues/3111) (ITL): CheckCommand for check\_interfaces

### Bug

* [#3319](https://github.com/icinga/icinga2/issues/3319) (Documentation): Duplicate severity type in the documentation for SyslogLogger
* [#3312](https://github.com/icinga/icinga2/issues/3312) (ITL): fix check command nscp-local
* [#3308](https://github.com/icinga/icinga2/issues/3308) (Documentation): Fix global Zone example to  "Global Configuration Zone for Templates"
* [#3305](https://github.com/icinga/icinga2/issues/3305) (Configuration): Icinga2 - too many open files - Exception
* [#3299](https://github.com/icinga/icinga2/issues/3299): Utility::Glob on Windows doesn't support wildcards in all but the last path component
* [#3298](https://github.com/icinga/icinga2/issues/3298) (Packages): Don't re-download NSCP for every build
* [#3292](https://github.com/icinga/icinga2/issues/3292): Serial number field is not properly initialized for CA certificates
* [#3279](https://github.com/icinga/icinga2/issues/3279) (DB IDO): Add missing category for IDO query
* [#3266](https://github.com/icinga/icinga2/issues/3266) (Plugins): Default disk checks on Windows fail because check\_disk doesn't support -K
* [#3265](https://github.com/icinga/icinga2/issues/3265) (ITL): check\_command interfaces option match\_aliases has to be boolean
* [#3262](https://github.com/icinga/icinga2/issues/3262) (Documentation): typo in docs
* [#3260](https://github.com/icinga/icinga2/issues/3260): First SOFT state is recognized as second SOFT state
* [#3255](https://github.com/icinga/icinga2/issues/3255) (Cluster): Warning about invalid API function icinga::Hello
* [#3241](https://github.com/icinga/icinga2/issues/3241): Agent freezes when the check returns massive output
* [#3239](https://github.com/icinga/icinga2/issues/3239) (Packages): missing check\_perfmon.exe 
* [#3222](https://github.com/icinga/icinga2/issues/3222) (Configuration): Dict initializer incorrectly re-initializes field that is set to an empty string
* [#3216](https://github.com/icinga/icinga2/issues/3216) (Tests): Build fix for Boost 1.59
* [#3211](https://github.com/icinga/icinga2/issues/3211) (Configuration): Operator + is inconsistent when used with empty and non-empty strings
* [#3200](https://github.com/icinga/icinga2/issues/3200) (CLI): icinga2 node wizard don't take zone\_name input
* [#3199](https://github.com/icinga/icinga2/issues/3199): Trying to set a field for a non-object instance fails
* [#3196](https://github.com/icinga/icinga2/issues/3196) (Cluster): Add log for missing EventCommand for command\_endpoints
* [#3194](https://github.com/icinga/icinga2/issues/3194): Set correct X509 version for certificates
* [#3149](https://github.com/icinga/icinga2/issues/3149) (CLI): missing config warning on empty port in endpoints
* [#3010](https://github.com/icinga/icinga2/issues/3010) (Cluster): cluster check w/ immediate parent and child zone endpoints
* [#2867](https://github.com/icinga/icinga2/issues/2867): Missing DEL\_DOWNTIME\_BY\_HOST\_NAME command required by Classic UI 1.x
* [#2352](https://github.com/icinga/icinga2/issues/2352) (Cluster): Reload does not work on Windows

## 2.3.8 (2015-07-21)

### Notes

* Bugfixes

### Bug

* [#3161](https://github.com/icinga/icinga2/issues/3161) (ITL): checkcommand disk does not check free inode - check\_disk
* [#3160](https://github.com/icinga/icinga2/issues/3160) (Performance Data): Escaping does not work for OpenTSDB perfdata plugin
* [#3152](https://github.com/icinga/icinga2/issues/3152) (ITL): Wrong parameter for CheckCommand "ping-common-windows"
* [#3151](https://github.com/icinga/icinga2/issues/3151) (DB IDO): DB IDO: Do not update endpointstatus table on config updates
* [#3120](https://github.com/icinga/icinga2/issues/3120) (Configuration): Don't allow "ignore where" for groups when there's no "assign where"

## 2.3.7 (2015-07-15)

### Notes

* Bugfixes

### Enhancement

* [#3142](https://github.com/icinga/icinga2/issues/3142) (Documentation): Enhance troubleshooting ssl errors & cluster replay log

### Bug

* [#3148](https://github.com/icinga/icinga2/issues/3148): Missing lock in ScriptUtils::Union
* [#3147](https://github.com/icinga/icinga2/issues/3147): Assertion failed in icinga::ScriptUtils::Intersection
* [#3136](https://github.com/icinga/icinga2/issues/3136) (DB IDO): DB IDO: endpoint\* tables are cleared on reload causing constraint violations
* [#3135](https://github.com/icinga/icinga2/issues/3135) (Documentation): Wrong formatting in DB IDO extensions docs
* [#3134](https://github.com/icinga/icinga2/issues/3134): Incorrect return value for the macro\(\) function
* [#3114](https://github.com/icinga/icinga2/issues/3114) (Configuration): Config parser ignores "ignore" in template definition
* [#3061](https://github.com/icinga/icinga2/issues/3061) (Cluster): Selective cluster reconnecting breaks client communication

## 2.3.6 (2015-07-08)

### Notes

* Require openssl1 on sles11sp3 from Security Module repository
  * Bug in SLES 11's OpenSSL version 0.9.8j preventing verification of generated certificates.
  * Re-create these certificates with 2.3.6 linking against openssl1 (cli command or CSR auto-signing).
* ITL: Add ldap, ntp_peer, mongodb and elasticsearch CheckCommand definitions
* Bugfixes

### Enhancement

* [#3132](https://github.com/icinga/icinga2/issues/3132) (ITL): new options for smtp CheckCommand
* [#3125](https://github.com/icinga/icinga2/issues/3125) (ITL): Add new options for ntp\_time CheckCommand
* [#3123](https://github.com/icinga/icinga2/issues/3123) (Packages): Require gcc47-c++ on sles11 from SLES software development kit repository
* [#3110](https://github.com/icinga/icinga2/issues/3110) (ITL): Add ntp\_peer CheckCommand
* [#3085](https://github.com/icinga/icinga2/issues/3085) (Documentation): Merge documentation fixes from GitHub
* [#3081](https://github.com/icinga/icinga2/issues/3081) (Installation): changelog.py: Allow to define project, make custom\_fields and changes optional
* [#3073](https://github.com/icinga/icinga2/issues/3073) (Installation): Enhance changelog.py with wordpress blogpost output
* [#3066](https://github.com/icinga/icinga2/issues/3066) (ITL): snmpv3 CheckCommand: Add possibility to set securityLevel
* [#3064](https://github.com/icinga/icinga2/issues/3064) (ITL): Add elasticsearch checkcommand to itl
* [#2975](https://github.com/icinga/icinga2/issues/2975) (ITL): Add "mongodb" CheckCommand definition
* [#2963](https://github.com/icinga/icinga2/issues/2963) (ITL): Add "ldap" CheckCommand for "check\_ldap" plugin
* [#2651](https://github.com/icinga/icinga2/issues/2651) (Packages): Add Icinga 2 to Chocolatey Windows Repository
* [#1793](https://github.com/icinga/icinga2/issues/1793) (Documentation): add pagerduty notification documentation

### Bug

* [#3126](https://github.com/icinga/icinga2/issues/3126) (Documentation): Update getting started for Debian Jessie
* [#3122](https://github.com/icinga/icinga2/issues/3122) (Packages): mysql-devel is not available in sles11sp3
* [#3118](https://github.com/icinga/icinga2/issues/3118) (Cluster): Generated certificates cannot be verified w/ openssl 0.9.8j on SLES 11
* [#3108](https://github.com/icinga/icinga2/issues/3108) (Documentation): wrong default port documentated for nrpe
* [#3103](https://github.com/icinga/icinga2/issues/3103) (ITL): itl/plugins-contrib.d/\*.conf should point to PluginContribDir
* [#3099](https://github.com/icinga/icinga2/issues/3099) (Documentation): Missing openssl verify in cluster troubleshooting docs
* [#3098](https://github.com/icinga/icinga2/issues/3098) (Cluster): Add log message for discarded cluster events \(e.g. from unauthenticated clients\)
* [#3097](https://github.com/icinga/icinga2/issues/3097): Fix stability issues in the TlsStream/Stream classes
* [#3096](https://github.com/icinga/icinga2/issues/3096) (Documentation): Documentation for checks in an HA zone is wrong
* [#3091](https://github.com/icinga/icinga2/issues/3091) (ITL): Incorrect check\_ping.exe parameter in the ITL
* [#3088](https://github.com/icinga/icinga2/issues/3088) (Cluster): Windows client w/ command\_endpoint broken with $nscp\_path$ and NscpPath detection
* [#3086](https://github.com/icinga/icinga2/issues/3086) (Documentation): Wrong file reference in README.md
* [#3084](https://github.com/icinga/icinga2/issues/3084) (CLI): node setup: indent accept\_config and accept\_commands
* [#3074](https://github.com/icinga/icinga2/issues/3074) (Notifications): Functions can't be specified as command arguments
* [#3031](https://github.com/icinga/icinga2/issues/3031) (ITL): Missing 'snmp\_is\_cisco' in Manubulon snmp-memory command definition
* [#3002](https://github.com/icinga/icinga2/issues/3002) (ITL): Incorrect variable name in the ITL
* [#2979](https://github.com/icinga/icinga2/issues/2979) (CLI): port empty when using icinga2 node wizard

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

* [#3072](https://github.com/icinga/icinga2/issues/3072) (Documentation): Documentation: Move configuration before advanced topics
* [#3069](https://github.com/icinga/icinga2/issues/3069) (Documentation): Enhance cluster docs with HA command\_endpoints
* [#3068](https://github.com/icinga/icinga2/issues/3068) (Documentation): Enhance cluster/client troubleshooting
* [#3049](https://github.com/icinga/icinga2/issues/3049) (Documentation): Update documentation for escape sequences
* [#3036](https://github.com/icinga/icinga2/issues/3036) (Documentation): Explain string concatenation in objects by real-world example
* [#3035](https://github.com/icinga/icinga2/issues/3035) (Documentation): Use a more simple example for passing command parameters
* [#3033](https://github.com/icinga/icinga2/issues/3033) (Documentation): Add local variable scope for \*Command to documentation \(host, service, etc\)
* [#3032](https://github.com/icinga/icinga2/issues/3032) (Documentation): Add typeof in 'assign/ignore where' expression as example
* [#3030](https://github.com/icinga/icinga2/issues/3030) (Documentation): Add examples for function usage in "set\_if" and "command" attributes
* [#3024](https://github.com/icinga/icinga2/issues/3024) (Documentation): Best practices: cluster config sync
* [#3019](https://github.com/icinga/icinga2/issues/3019) (ITL): Add 'iftraffic' to plugins-contrib check command definitions
* [#3017](https://github.com/icinga/icinga2/issues/3017) (Documentation): Update service apply for documentation
* [#3011](https://github.com/icinga/icinga2/issues/3011) (Installation): Add support for installing NSClient++ in the Icinga 2 Windows wizard
* [#3009](https://github.com/icinga/icinga2/issues/3009) (Configuration): Add the --load-all and --log options for nscp-local
* [#3008](https://github.com/icinga/icinga2/issues/3008) (Configuration): Include \<nscp\> by default on Windows
* [#3005](https://github.com/icinga/icinga2/issues/3005) (Installation): Determine NSClient++ installation path using MsiGetComponentPath
* [#3003](https://github.com/icinga/icinga2/issues/3003) (ITL): Add 'disk\_smb' Plugin CheckCommand definition
* [#2994](https://github.com/icinga/icinga2/issues/2994) (Installation): Bundle NSClient++ in Windows Installer
* [#2971](https://github.com/icinga/icinga2/issues/2971) (Performance Data): Add timestamp support for PerfdataWriter
* [#2966](https://github.com/icinga/icinga2/issues/2966) (Documentation): Include Windows support details in the documentation
* [#2965](https://github.com/icinga/icinga2/issues/2965) (Documentation): ITL Documentation: Add a link for passing custom attributes as command parameters
* [#2956](https://github.com/icinga/icinga2/issues/2956) (ITL): Add arguments to "tcp" CheckCommand
* [#2955](https://github.com/icinga/icinga2/issues/2955) (ITL): Add arguments to "ftp" CheckCommand
* [#2954](https://github.com/icinga/icinga2/issues/2954) (ITL): Add arguments to "dns" CheckCommand
* [#2949](https://github.com/icinga/icinga2/issues/2949) (ITL): Add 'check\_drivesize' as nscp-local check command
* [#2938](https://github.com/icinga/icinga2/issues/2938) (ITL): Add SHOWALL to NSCP Checkcommand
* [#2817](https://github.com/icinga/icinga2/issues/2817) (Configuration): Add CheckCommand objects for Windows plugins
* [#2794](https://github.com/icinga/icinga2/issues/2794) (Plugins): Add check\_perfmon plugin for Windows
* [#2451](https://github.com/icinga/icinga2/issues/2451) (Installation): Extend Windows installer with an update mode
* [#2279](https://github.com/icinga/icinga2/issues/2279) (Documentation): Add documentation and CheckCommands for the windows plugins

### Bug

* [#3062](https://github.com/icinga/icinga2/issues/3062) (Documentation): Documentation: Update the link to register a new Icinga account
* [#3059](https://github.com/icinga/icinga2/issues/3059) (Documentation): Documentation: Typo
* [#3057](https://github.com/icinga/icinga2/issues/3057) (Documentation): Documentation: Extend Custom Attributes with the boolean type
* [#3056](https://github.com/icinga/icinga2/issues/3056) (Documentation): Wrong service table attributes in Livestatus documentation
* [#3055](https://github.com/icinga/icinga2/issues/3055) (Documentation): Documentation: Typo
* [#3051](https://github.com/icinga/icinga2/issues/3051) (Plugins): plugins-contrib.d/databases.conf: wrong argument for mssql\_health
* [#3043](https://github.com/icinga/icinga2/issues/3043) (Compat): Multiline vars are broken in objects.cache output
* [#3039](https://github.com/icinga/icinga2/issues/3039) (Compat): Multi line output not correctly handled from compat channels
* [#3016](https://github.com/icinga/icinga2/issues/3016) (Installation): Wrong permission etc on windows
* [#3015](https://github.com/icinga/icinga2/issues/3015) (Documentation): Typo in Configuration Best Practice
* [#3007](https://github.com/icinga/icinga2/issues/3007) (Configuration): Disk and 'icinga' services are missing in the default Windows config
* [#3006](https://github.com/icinga/icinga2/issues/3006) (Configuration): Some checks in the default Windows configuration fail
* [#3004](https://github.com/icinga/icinga2/issues/3004) (Installation): --scm-installs fails when the service is already installed
* [#2986](https://github.com/icinga/icinga2/issues/2986) (DB IDO): Missing custom attributes in backends if name is equal to object attribute
* [#2973](https://github.com/icinga/icinga2/issues/2973) (Packages): SPEC: Give group write permissions for perfdata dir
* [#2959](https://github.com/icinga/icinga2/issues/2959) (ITL): 'disk': wrong order of threshold command arguments
* [#2952](https://github.com/icinga/icinga2/issues/2952) (DB IDO): Incorrect type and state filter mapping for User objects in DB IDO
* [#2951](https://github.com/icinga/icinga2/issues/2951) (DB IDO): Downtimes are always "fixed"
* [#2950](https://github.com/icinga/icinga2/issues/2950) (Documentation): Missing "\)" in last Apply Rules example
* [#2945](https://github.com/icinga/icinga2/issues/2945) (DB IDO): Possible DB deadlock
* [#2940](https://github.com/icinga/icinga2/issues/2940) (Configuration): node update-config reports critical and warning
* [#2935](https://github.com/icinga/icinga2/issues/2935) (Configuration): WIN: syslog is not an enable-able feature in windows
* [#2894](https://github.com/icinga/icinga2/issues/2894) (DB IDO): Wrong timestamps w/ historical data replay in DB IDO
* [#2880](https://github.com/icinga/icinga2/issues/2880) (ITL): Including \<nscp\> on Linux fails with unregistered function
* [#2839](https://github.com/icinga/icinga2/issues/2839) (CLI): Node wont connect properly to master if host is is not set for Endpoint on new installs
* [#2836](https://github.com/icinga/icinga2/issues/2836): Icinga2 --version: Error showing Distribution
* [#2819](https://github.com/icinga/icinga2/issues/2819) (Configuration): Syntax Highlighting: host.address vs host.add 

## 2.3.4 (2015-04-20)

### Notes

* ITL: Check commands for various databases
* Improve validation messages for time periods
* Update max_check_attempts in generic-{host,service} templates
* Update logrotate configuration
* Bugfixes

### Enhancement

* [#2843](https://github.com/icinga/icinga2/issues/2843) (Documentation): Add explanatory note for Icinga2 client documentation
* [#2841](https://github.com/icinga/icinga2/issues/2841): Improve timeperiod validation error messages
* [#2791](https://github.com/icinga/icinga2/issues/2791) (Cluster): Agent Wizard: add options for API defaults
* [#2770](https://github.com/icinga/icinga2/issues/2770) (ITL): Add database plugins to ITL

### Bug

* [#2903](https://github.com/icinga/icinga2/issues/2903) (Configuration): custom attributes with recursive macro function calls causing sigabrt
* [#2902](https://github.com/icinga/icinga2/issues/2902) (Documentation): Documentation: set\_if usage with boolean values and functions
* [#2898](https://github.com/icinga/icinga2/issues/2898) (CLI): troubleshoot truncates crash reports
* [#2891](https://github.com/icinga/icinga2/issues/2891) (ITL): web.conf is not in the RPM package
* [#2890](https://github.com/icinga/icinga2/issues/2890) (ITL): check\_disk order of command arguments 
* [#2888](https://github.com/icinga/icinga2/issues/2888) (Installation): Vim syntax: Match groups before host/service/user objects
* [#2886](https://github.com/icinga/icinga2/issues/2886): Acknowledging problems w/ expire time does not add the expiry information to the related comment for IDO and compat
* [#2883](https://github.com/icinga/icinga2/issues/2883) (Notifications): Multiple log messages w/ "Attempting to send notifications for notification object"
* [#2882](https://github.com/icinga/icinga2/issues/2882) (DB IDO): scheduled\_downtime\_depth column is not reset when a downtime ends or when a downtime is being removed
* [#2881](https://github.com/icinga/icinga2/issues/2881) (DB IDO): Downtimes which have been triggered are not properly recorded in the database
* [#2878](https://github.com/icinga/icinga2/issues/2878) (DB IDO): Don't update scheduleddowntime table w/ trigger\_time column when only adding a downtime
* [#2876](https://github.com/icinga/icinga2/issues/2876) (Documentation): Typo in graphite feature enable documentation
* [#2868](https://github.com/icinga/icinga2/issues/2868) (Documentation): Fix a typo
* [#2855](https://github.com/icinga/icinga2/issues/2855): Fix complexity class for Dictionary::Get
* [#2853](https://github.com/icinga/icinga2/issues/2853) (CLI): Node wizard should only accept 'y', 'n', 'Y' and 'N' as answers for boolean questions  
* [#2852](https://github.com/icinga/icinga2/issues/2852) (Installation): Windows Build: Flex detection
* [#2842](https://github.com/icinga/icinga2/issues/2842) (Configuration): Default max\_check\_attempts should be lower for hosts than for services
* [#2840](https://github.com/icinga/icinga2/issues/2840) (Configuration): Validation errors for time ranges which span the DST transition
* [#2837](https://github.com/icinga/icinga2/issues/2837) (Documentation): Fix a minor markdown error
* [#2834](https://github.com/icinga/icinga2/issues/2834) (ITL): Add arguments to the UPS check
* [#2832](https://github.com/icinga/icinga2/issues/2832) (Documentation): Reword documentation of check\_address
* [#2827](https://github.com/icinga/icinga2/issues/2827) (Configuration): logrotate does not work
* [#2801](https://github.com/icinga/icinga2/issues/2801) (Cluster): command\_endpoint check\_results are not replicated to other endpoints in the same zone
* [#2793](https://github.com/icinga/icinga2/issues/2793) (Packages): logrotate doesn't work on Ubuntu

## 2.3.3 (2015-03-26)

### Notes

* New function: parse_performance_data
* Include more details in --version
* Improve documentation
* Bugfixes

### Enhancement

* [#2806](https://github.com/icinga/icinga2/issues/2806) (Documentation): Move release info in INSTALL.md into a separate file
* [#2799](https://github.com/icinga/icinga2/issues/2799) (ITL): Add "random" CheckCommand for test and demo purposes
* [#2771](https://github.com/icinga/icinga2/issues/2771): Include more details in --version
* [#2756](https://github.com/icinga/icinga2/issues/2756) (Documentation): Add "access objects at runtime" examples to advanced section
* [#2743](https://github.com/icinga/icinga2/issues/2743): New function: parse\_performance\_data
* [#2738](https://github.com/icinga/icinga2/issues/2738) (Documentation): Update documentation for "apply for" rules
* [#2737](https://github.com/icinga/icinga2/issues/2737) (Notifications): Show state/type filter names in notice/debug log

### Bug

* [#2828](https://github.com/icinga/icinga2/issues/2828): Array in command arguments doesn't work
* [#2825](https://github.com/icinga/icinga2/issues/2825) (Documentation): Fix incorrect perfdata templates in the documentation 
* [#2823](https://github.com/icinga/icinga2/issues/2823) (ITL): wrong 'dns\_lookup' custom attribute default in command-plugins.conf 
* [#2818](https://github.com/icinga/icinga2/issues/2818) (Configuration): Local variables in "apply for" are overridden
* [#2816](https://github.com/icinga/icinga2/issues/2816) (CLI): Segmentation fault when executing "icinga2 pki new-cert"
* [#2812](https://github.com/icinga/icinga2/issues/2812) (Configuration): Return doesn't work inside loops
* [#2807](https://github.com/icinga/icinga2/issues/2807) (Configuration): Figure out why command validators are not triggered 
* [#2779](https://github.com/icinga/icinga2/issues/2779) (Documentation): Correct HA documentation
* [#2778](https://github.com/icinga/icinga2/issues/2778) (Configuration): object Notification + apply Service fails with error "...refers to service which doesn't exist"
* [#2777](https://github.com/icinga/icinga2/issues/2777) (Documentation): Typo and invalid example in the runtime macro documentation
* [#2776](https://github.com/icinga/icinga2/issues/2776) (Documentation): Remove prompt to create a TicketSalt from the wizard
* [#2775](https://github.com/icinga/icinga2/issues/2775) (Documentation): Explain processing logic/order of apply rules with for loops
* [#2774](https://github.com/icinga/icinga2/issues/2774) (Documentation): Revamp migration documentation
* [#2773](https://github.com/icinga/icinga2/issues/2773) (Documentation): Typo in doc library-reference
* [#2772](https://github.com/icinga/icinga2/issues/2772) (Plugins): Plugin "check\_http" is missing in Windows environments
* [#2768](https://github.com/icinga/icinga2/issues/2768) (Configuration): Add missing keywords in the syntax highlighting files
* [#2765](https://github.com/icinga/icinga2/issues/2765) (Documentation): Fix a typo in the documentation of ICINGA2\_WITH\_MYSQL and ICINGA2\_WITH\_PGSQL
* [#2762](https://github.com/icinga/icinga2/issues/2762) (Installation): Flex version check does not reject unsupported versions
* [#2761](https://github.com/icinga/icinga2/issues/2761) (Installation): Build warnings with CMake 3.1.3
* [#2760](https://github.com/icinga/icinga2/issues/2760): Don't ignore extraneous arguments for functions
* [#2753](https://github.com/icinga/icinga2/issues/2753) (DB IDO): Don't update custom vars for each status update
* [#2752](https://github.com/icinga/icinga2/issues/2752): startup.log broken when the DB schema needs an update
* [#2749](https://github.com/icinga/icinga2/issues/2749) (Configuration): Missing config validator for command arguments 'set\_if'
* [#2718](https://github.com/icinga/icinga2/issues/2718) (Configuration): Update syntax highlighting for 2.3 features
* [#2557](https://github.com/icinga/icinga2/issues/2557) (Configuration): Improve error message for invalid field access
* [#2548](https://github.com/icinga/icinga2/issues/2548) (Configuration): Fix VIM syntax highlighting for comments
* [#2501](https://github.com/icinga/icinga2/issues/2501) (Documentation): Re-order the object types in alphabetical order

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

* [#2742](https://github.com/icinga/icinga2/issues/2742) (Packages): Debian packages do not create /var/log/icinga2/crash
* [#2739](https://github.com/icinga/icinga2/issues/2739): Crash in Dependency::Stop
* [#2736](https://github.com/icinga/icinga2/issues/2736): Fix formatting for the GDB stacktrace
* [#2735](https://github.com/icinga/icinga2/issues/2735): Make sure that the /var/log/icinga2/crash directory exists
* [#2732](https://github.com/icinga/icinga2/issues/2732) (Documentation): Update documentation for DB IDO HA Run-Once
* [#2731](https://github.com/icinga/icinga2/issues/2731) (Configuration): Config validation fail because of unexpected new-line
* [#2728](https://github.com/icinga/icinga2/issues/2728) (Documentation): Fix check\_disk default thresholds and document the change of unit
* [#2727](https://github.com/icinga/icinga2/issues/2727) (Cluster): Api heartbeat message response time problem
* [#2716](https://github.com/icinga/icinga2/issues/2716) (CLI): Missing program name in 'icinga2 --version'
* [#2672](https://github.com/icinga/icinga2/issues/2672): Kill signal sent only to check process, not whole process group
* [#2483](https://github.com/icinga/icinga2/issues/2483) (ITL): Fix check\_disk thresholds: make sure partitions are the last arguments

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

* [#2711](https://github.com/icinga/icinga2/issues/2711) (Documentation): Document closures \('use'\)
* [#2705](https://github.com/icinga/icinga2/issues/2705) (ITL): Add check commands for NSClient++
* [#2704](https://github.com/icinga/icinga2/issues/2704): Support the SNI TLS extension
* [#2702](https://github.com/icinga/icinga2/issues/2702): Add validator for time ranges in ScheduledDowntime objects
* [#2701](https://github.com/icinga/icinga2/issues/2701): Remove macro argument for IMPL\_TYPE\_LOOKUP
* [#2696](https://github.com/icinga/icinga2/issues/2696): Include GDB backtrace in crash reports
* [#2678](https://github.com/icinga/icinga2/issues/2678) (Configuration): Add support for else-if
* [#2663](https://github.com/icinga/icinga2/issues/2663) (Livestatus): Change Livestatus query log level to 'notice'
* [#2662](https://github.com/icinga/icinga2/issues/2662) (Documentation): Update Remote Client/Distributed Monitoring Documentation
* [#2657](https://github.com/icinga/icinga2/issues/2657) (Cluster): Show slave lag for the cluster-zone check
* [#2652](https://github.com/icinga/icinga2/issues/2652) (ITL): Rename PluginsContribDir to PluginContribDir
* [#2649](https://github.com/icinga/icinga2/issues/2649) (ITL): Snmp CheckCommand misses various options
* [#2635](https://github.com/icinga/icinga2/issues/2635) (Configuration): introduce time dependent variable values
* [#2634](https://github.com/icinga/icinga2/issues/2634) (Cluster): Add the ability to use a CA certificate as a way of verifying hosts for CSR autosigning
* [#2614](https://github.com/icinga/icinga2/issues/2614) (ITL): add webinject checkcommand
* [#2610](https://github.com/icinga/icinga2/issues/2610) (ITL): Add ITL check command for check\_ipmi\_sensor
* [#2609](https://github.com/icinga/icinga2/issues/2609): udp check command is missing arguments.
* [#2604](https://github.com/icinga/icinga2/issues/2604) (CLI): Backup certificate files in 'node setup'
* [#2601](https://github.com/icinga/icinga2/issues/2601) (Configuration): Implement continue/break keywords
* [#2600](https://github.com/icinga/icinga2/issues/2600) (Configuration): Implement support for Json.encode and Json.decode
* [#2595](https://github.com/icinga/icinga2/issues/2595) (Documentation): Add documentation for cli command 'console'
* [#2591](https://github.com/icinga/icinga2/issues/2591) (Performance Data): Add timestamp support for Graphite
* [#2588](https://github.com/icinga/icinga2/issues/2588) (Configuration): Add path information for objects in object list
* [#2578](https://github.com/icinga/icinga2/issues/2578) (Configuration): Implement Array\#join
* [#2573](https://github.com/icinga/icinga2/issues/2573) (ITL): Extend disk checkcommand
* [#2555](https://github.com/icinga/icinga2/issues/2555) (Documentation): The Zone::global attribute is not documented
* [#2553](https://github.com/icinga/icinga2/issues/2553) (Configuration): Implement validator support for function objects
* [#2552](https://github.com/icinga/icinga2/issues/2552) (Configuration): Make operators &&, || behave like in JavaScript
* [#2546](https://github.com/icinga/icinga2/issues/2546): Add macros $host.check\_source$ and $service.check\_source$
* [#2544](https://github.com/icinga/icinga2/issues/2544) (Configuration): Implement the while keyword
* [#2541](https://github.com/icinga/icinga2/issues/2541) (ITL): The check "hostalive" is not working with ipv6
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
* [#2399](https://github.com/icinga/icinga2/issues/2399) (Documentation): Allow name changed from inside the object
* [#2396](https://github.com/icinga/icinga2/issues/2396) (Configuration): Evaluate usage of function\(\)
* [#2391](https://github.com/icinga/icinga2/issues/2391): Improve output of ToString for type objects
* [#2390](https://github.com/icinga/icinga2/issues/2390): Register type objects as global variables
* [#2387](https://github.com/icinga/icinga2/issues/2387) (Documentation): Documentation enhancement for snmp traps and passive checks.
* [#2374](https://github.com/icinga/icinga2/issues/2374) (Packages): Move the config file for the ido-\*sql features into the icinga2-ido-\* packages
* [#2367](https://github.com/icinga/icinga2/issues/2367) (Configuration): The lexer shouldn't accept escapes for characters which don't have to be escaped
* [#2365](https://github.com/icinga/icinga2/issues/2365) (DB IDO): Implement socket\_path attribute for the IdoMysqlConnection class
* [#2355](https://github.com/icinga/icinga2/issues/2355) (Configuration): Implement official support for user-defined functions and the "for" keyword
* [#2351](https://github.com/icinga/icinga2/issues/2351) (Plugins): Windows agent is missing the standard plugin check\_ping
* [#2348](https://github.com/icinga/icinga2/issues/2348) (Plugins): Plugin Check Commands: Add icmp
* [#2324](https://github.com/icinga/icinga2/issues/2324) (Configuration): Implement the "if" and "else" keywords
* [#2323](https://github.com/icinga/icinga2/issues/2323) (Configuration): Figure out whether Number + String should implicitly convert the Number argument to a string
* [#2322](https://github.com/icinga/icinga2/issues/2322) (Configuration): Make the config parser thread-safe
* [#2321](https://github.com/icinga/icinga2/issues/2321) (Documentation): Document operator precedence
* [#2318](https://github.com/icinga/icinga2/issues/2318) (Configuration): Implement the % operator
* [#2312](https://github.com/icinga/icinga2/issues/2312): Move the cast functions into libbase
* [#2310](https://github.com/icinga/icinga2/issues/2310) (Configuration): Implement unit tests for the config parser
* [#2304](https://github.com/icinga/icinga2/issues/2304): Implement an option to disable building the Demo component
* [#2303](https://github.com/icinga/icinga2/issues/2303): Implement an option to disable building the Livestatus module
* [#2302](https://github.com/icinga/icinga2/issues/2302) (Installation): Don't build db\_ido when both MySQL and PostgreSQL aren't enabled
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
* [#2012](https://github.com/icinga/icinga2/issues/2012) (ITL): ITL: ESXi-Hardware
* [#2011](https://github.com/icinga/icinga2/issues/2011) (ITL): ITL: Check\_Mem.pl
* [#2000](https://github.com/icinga/icinga2/issues/2000) (Performance Data): Add OpenTSDB Writer
* [#1984](https://github.com/icinga/icinga2/issues/1984) (ITL): ITL: Interfacetable
* [#1959](https://github.com/icinga/icinga2/issues/1959) (Configuration): extended Manubulon SNMP Check Plugin Command 
* [#1890](https://github.com/icinga/icinga2/issues/1890) (DB IDO): IDO should fill program\_end\_time on a clean shutdown
* [#1866](https://github.com/icinga/icinga2/issues/1866) (Notifications): Disable flapping detection by default
* [#1860](https://github.com/icinga/icinga2/issues/1860) (Documentation): Add some more PNP details
* [#1859](https://github.com/icinga/icinga2/issues/1859): Run CheckCommands with C locale \(workaround for comma vs dot and plugin api bug\)
* [#1783](https://github.com/icinga/icinga2/issues/1783) (Plugins): Plugin Check Commands: add check\_vmware\_esx
* [#1733](https://github.com/icinga/icinga2/issues/1733) (Configuration): Disallow side-effect-free r-value expressions in expression lists
* [#1507](https://github.com/icinga/icinga2/issues/1507): Don't spawn threads for network connections
* [#404](https://github.com/icinga/icinga2/issues/404) (CLI): Add troubleshooting collect cli command

### Bug

* [#2709](https://github.com/icinga/icinga2/issues/2709) (Documentation): Fix a typo in documentation
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
* [#2661](https://github.com/icinga/icinga2/issues/2661) (ITL): ITL: The procs check command uses spaces instead of tabs
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
* [#2616](https://github.com/icinga/icinga2/issues/2616) (Installation): Build fails on OpenBSD
* [#2615](https://github.com/icinga/icinga2/issues/2615): Make the arguments for the stats functions const-ref
* [#2613](https://github.com/icinga/icinga2/issues/2613) (DB IDO): DB IDO: Duplicate entry icinga\_scheduleddowntime
* [#2608](https://github.com/icinga/icinga2/issues/2608) (Plugins): Ignore the -X option for check\_disk on Windows
* [#2605](https://github.com/icinga/icinga2/issues/2605): Compiler warnings
* [#2602](https://github.com/icinga/icinga2/issues/2602) (Packages): Icinga2 config reset after package update \(centos6.6\)
* [#2599](https://github.com/icinga/icinga2/issues/2599) (Cluster): Agent writes CR CR LF in synchronized config files
* [#2598](https://github.com/icinga/icinga2/issues/2598): Added downtimes must be triggered immediately if checkable is Not-OK
* [#2597](https://github.com/icinga/icinga2/issues/2597) (Cluster): Config sync authoritative file never created
* [#2596](https://github.com/icinga/icinga2/issues/2596) (Compat): StatusDataWriter: Wrong host notification filters \(broken fix in \#8192\)
* [#2593](https://github.com/icinga/icinga2/issues/2593) (Compat): last\_hard\_state missing in StatusDataWriter
* [#2589](https://github.com/icinga/icinga2/issues/2589) (Configuration): Stacktrace on Endpoint not belonging to a zone or multiple zones
* [#2586](https://github.com/icinga/icinga2/issues/2586): Icinga2 master doesn't change check-status when "accept\_commands = true" is not set at client node
* [#2579](https://github.com/icinga/icinga2/issues/2579) (Configuration): Apply rule '' for host does not match anywhere!
* [#2575](https://github.com/icinga/icinga2/issues/2575) (Documentation): Remote Clients: Add manual setup cli commands
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
* [#2511](https://github.com/icinga/icinga2/issues/2511) (Packages): '../features-available/checker.conf' does not exist \[Windows\]
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
* [#2198](https://github.com/icinga/icinga2/issues/2198) (Documentation): Variable expansion is single quoted.
* [#2194](https://github.com/icinga/icinga2/issues/2194) (Configuration): validate configured legacy timeperiod ranges
* [#2174](https://github.com/icinga/icinga2/issues/2174) (Configuration): Update validators for CustomVarObject
* [#2020](https://github.com/icinga/icinga2/issues/2020) (Configuration): Invalid macro results in exception
* [#1899](https://github.com/icinga/icinga2/issues/1899): Scheduled start time will be ignored if the host or service is already in a problem state
* [#1530](https://github.com/icinga/icinga2/issues/1530): Remove name and return value for stats functions

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
* [#2532](https://github.com/icinga/icinga2/issues/2532) (ITL): check\_ssmtp command does NOT support mail\_from
* [#2521](https://github.com/icinga/icinga2/issues/2521) (Documentation): Typos in readme file for windows plugins
* [#2520](https://github.com/icinga/icinga2/issues/2520) (Documentation): inconsistent URL http\(s\)://www.icinga.org
* [#2517](https://github.com/icinga/icinga2/issues/2517) (Packages): Fix YAJL detection on Debian squeeze
* [#2512](https://github.com/icinga/icinga2/issues/2512) (Documentation): Update Icinga Web 2 uri to /icingaweb2
* [#2508](https://github.com/icinga/icinga2/issues/2508) (Compat): Feature statusdata shows wrong host notification options
* [#2481](https://github.com/icinga/icinga2/issues/2481) (CLI): Satellite doesn't use manually supplied 'local zone name'
* [#2464](https://github.com/icinga/icinga2/issues/2464): vfork\(\) hangs on OS X
* [#2462](https://github.com/icinga/icinga2/issues/2462) (Packages): Icinga 2.2.2 build fails on SLES11SP3 because of changed boost dependency
* [#2256](https://github.com/icinga/icinga2/issues/2256) (Notifications): kUn-Bashify mail-{host,service}-notification.sh
* [#2242](https://github.com/icinga/icinga2/issues/2242): livestatus / nsca / etc submits are ignored during reload
* [#1893](https://github.com/icinga/icinga2/issues/1893): Configured recurring downtimes not applied on saturdays

## 2.2.3 (2015-01-12)

### Notes

* Bugfixes

### Bug

* [#2499](https://github.com/icinga/icinga2/issues/2499) (CLI): Segfault on update-config old empty config
* [#2498](https://github.com/icinga/icinga2/issues/2498) (CLI): icinga2 node update config shows hex instead of human readable names
* [#2496](https://github.com/icinga/icinga2/issues/2496): Icinga 2.2.2 segfaults on FreeBSD
* [#2490](https://github.com/icinga/icinga2/issues/2490) (Documentation): Typo in example of StatusDataWriter
* [#2477](https://github.com/icinga/icinga2/issues/2477): DB IDO query queue limit reached on reload
* [#2473](https://github.com/icinga/icinga2/issues/2473) (CLI): check\_interval must be greater than 0 error on update-config
* [#2471](https://github.com/icinga/icinga2/issues/2471) (Cluster): Arguments without values are not used on plugin exec
* [#2470](https://github.com/icinga/icinga2/issues/2470) (Plugins): Windows plugin check\_service.exe can't find service NTDS
* [#2460](https://github.com/icinga/icinga2/issues/2460) (Packages): Icinga 2.2.2 doesn't build on i586 SUSE distributions
* [#2459](https://github.com/icinga/icinga2/issues/2459) (CLI): Incorrect ticket shouldn't cause "node wizard" to terminate
* [#2420](https://github.com/icinga/icinga2/issues/2420) (Notifications): Volatile checks trigger invalid notifications on OK-\>OK state changes

## 2.2.2 (2014-12-18)

### Notes

* Bugfixes

### Bug

* [#2446](https://github.com/icinga/icinga2/issues/2446) (Compat): StatusDataWriter: Wrong export of event\_handler\_enabled
* [#2444](https://github.com/icinga/icinga2/issues/2444) (CLI): Remove usage info from --version
* [#2430](https://github.com/icinga/icinga2/issues/2430) (ITL): No option to specify timeout to check\_snmp and snmp manubulon commands
* [#2422](https://github.com/icinga/icinga2/issues/2422) (Documentation): Setting a dictionary key to null does not cause the key/value to be removed
* [#2417](https://github.com/icinga/icinga2/issues/2417) (Tests): Unit tests fail on FreeBSD
* [#2416](https://github.com/icinga/icinga2/issues/2416) (DB IDO): DB IDO: Missing last\_hard\_state column update in {host,service}status tables
* [#2412](https://github.com/icinga/icinga2/issues/2412) (Documentation): Update host examples in Dependencies for Network Reachability documentation
* [#2411](https://github.com/icinga/icinga2/issues/2411): exception during config check
* [#2409](https://github.com/icinga/icinga2/issues/2409) (Documentation): Wrong command in documentation for installing Icinga 2 pretty printers.
* [#2404](https://github.com/icinga/icinga2/issues/2404) (Documentation): Livestatus: Replace unixcat with nc -U 
* [#2394](https://github.com/icinga/icinga2/issues/2394): typeof does not work for numbers
* [#2381](https://github.com/icinga/icinga2/issues/2381): SIGABRT while evaluating apply rules
* [#2380](https://github.com/icinga/icinga2/issues/2380) (Configuration): typeof\(\) seems to return null for arrays and dictionaries
* [#2376](https://github.com/icinga/icinga2/issues/2376) (Configuration): Apache 2.2 fails with new apache conf
* [#2371](https://github.com/icinga/icinga2/issues/2371) (Configuration): Test Classic UI config file with Apache 2.4
* [#2370](https://github.com/icinga/icinga2/issues/2370) (Cluster): update\_config not updating configuration
* [#2369](https://github.com/icinga/icinga2/issues/2369) (Packages): SUSE packages %set\_permissions post statement wasn't moved to common
* [#2368](https://github.com/icinga/icinga2/issues/2368) (Packages): /usr/lib/icinga2 is not owned by a package
* [#2360](https://github.com/icinga/icinga2/issues/2360): CLI `icinga2 node update-config` doesn't sync configs from remote clients as expected
* [#2354](https://github.com/icinga/icinga2/issues/2354) (DB IDO): Improve error reporting when libmysqlclient or libpq are missing
* [#2350](https://github.com/icinga/icinga2/issues/2350) (Cluster): Segfault on issuing node update-config
* [#2341](https://github.com/icinga/icinga2/issues/2341) (Cluster): execute checks locally if command\_endpoint == local endpoint
* [#2292](https://github.com/icinga/icinga2/issues/2292) (Tests): The unit tests still crash sometimes
* [#2283](https://github.com/icinga/icinga2/issues/2283) (Cluster): Cluster heartbeats need to be more aggressive
* [#2266](https://github.com/icinga/icinga2/issues/2266) (CLI): "node wizard" shouldn't crash when SaveCert fails
* [#2255](https://github.com/icinga/icinga2/issues/2255) (DB IDO): If a parent host goes down, the child host isn't marked as unrechable in the db ido
* [#2216](https://github.com/icinga/icinga2/issues/2216) (Cluster): Repository does not support services which have a slash in their name
* [#2202](https://github.com/icinga/icinga2/issues/2202) (Configuration): CPU usage at 100% when check\_interval = 0 in host object definition 
* [#2180](https://github.com/icinga/icinga2/issues/2180) (Documentation): Documentation: Add note on default notification interval in getting started notifications.conf
* [#2154](https://github.com/icinga/icinga2/issues/2154) (Cluster): update-config fails to create hosts
* [#2148](https://github.com/icinga/icinga2/issues/2148) (Compat): Feature `compatlog' should flush output buffer on every new line
* [#2021](https://github.com/icinga/icinga2/issues/2021): double macros in command arguments seems to lead to exception
* [#2016](https://github.com/icinga/icinga2/issues/2016) (Notifications): Docs: Better explaination of dependency state filters
* [#1947](https://github.com/icinga/icinga2/issues/1947) (Livestatus): Missing host downtimes/comments in Livestatus
* [#1942](https://github.com/icinga/icinga2/issues/1942) (Packages): icinga2 init-script doesn't validate configuration on reload action

## 2.2.1 (2014-12-01)

### Notes

* Support arrays in [command argument macros](#command-passing-parameters) #6709
    * Allows to define multiple parameters for [nrpe -a](#plugin-check-command-nrpe), [nscp -l](#plugin-check-command-nscp), [disk -p](#plugin-check-command-disk), [dns -a](#plugin-check-command-dns).
* Bugfixes

### Enhancement

* [#2366](https://github.com/icinga/icinga2/issues/2366): Release 2.2.1
* [#2343](https://github.com/icinga/icinga2/issues/2343) (Documentation): Document how arrays in macros work
* [#2285](https://github.com/icinga/icinga2/issues/2285) (ITL): Increase default timeout for NRPE checks
* [#2277](https://github.com/icinga/icinga2/issues/2277) (Configuration): The classicui Apache conf doesn't support Apache 2.4
* [#2117](https://github.com/icinga/icinga2/issues/2117) (Packages): Update spec file to use yajl-devel
* [#1790](https://github.com/icinga/icinga2/issues/1790): Support for arrays in macros

### Bug

* [#2363](https://github.com/icinga/icinga2/issues/2363) (Packages): Fix Apache config in the Debian package
* [#2359](https://github.com/icinga/icinga2/issues/2359) (Packages): Wrong permission in run directory after restart
* [#2344](https://github.com/icinga/icinga2/issues/2344) (Documentation): Documentation: Explain how unresolved macros are handled
* [#2340](https://github.com/icinga/icinga2/issues/2340) (CLI): Segfault in CA handling
* [#2336](https://github.com/icinga/icinga2/issues/2336) (Documentation): Wrong information in section "Linux Client Setup Wizard for Remote Monitoring"
* [#2328](https://github.com/icinga/icinga2/issues/2328) (Cluster): Verify if master radio box is disabled in the Windows wizard
* [#2311](https://github.com/icinga/icinga2/issues/2311) (Configuration): !in operator returns incorrect result
* [#2301](https://github.com/icinga/icinga2/issues/2301) (Packages): Move the icinga2-prepare-dirs script elsewhere
* [#2293](https://github.com/icinga/icinga2/issues/2293) (Configuration): Objects created with node update-config can't be seen in Classic UI
* [#2288](https://github.com/icinga/icinga2/issues/2288) (Cluster): Incorrect error message for localhost
* [#2282](https://github.com/icinga/icinga2/issues/2282) (Cluster): Icinga2 node add failed with unhandled exception
* [#2280](https://github.com/icinga/icinga2/issues/2280) (Packages): Icinga 2.2 misses the build requirement libyajl-devel for SUSE distributions
* [#2278](https://github.com/icinga/icinga2/issues/2278) (Packages): /usr/sbin/icinga-prepare-dirs conflicts in the bin and common package
* [#2276](https://github.com/icinga/icinga2/issues/2276) (Packages): Systemd rpm scripts are run in wrong package
* [#2275](https://github.com/icinga/icinga2/issues/2275) (Documentation): 2.2.0 has out-of-date icinga2 man page
* [#2273](https://github.com/icinga/icinga2/issues/2273): Restart Icinga - Error Restoring program state from file '/var/lib/icinga2/icinga2.state'
* [#2272](https://github.com/icinga/icinga2/issues/2272) (Cluster): Windows wizard is missing --zone argument
* [#2271](https://github.com/icinga/icinga2/issues/2271) (Cluster): Windows wizard uses incorrect CLI command
* [#2267](https://github.com/icinga/icinga2/issues/2267) (Cluster): Built-in commands shouldn't be run on the master instance in remote command execution mode
* [#2251](https://github.com/icinga/icinga2/issues/2251) (Documentation): object and template with the same name generate duplicate object error
* [#2212](https://github.com/icinga/icinga2/issues/2212) (Packages): icinga2 checkconfig should fail if group given for command files does not exist
* [#2207](https://github.com/icinga/icinga2/issues/2207) (Livestatus): livestatus large amount of submitting unix socket command results in broken pipes
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

* [#2253](https://github.com/icinga/icinga2/issues/2253) (Packages): Conditionally enable MySQL and PostgresSQL, add support for FreeBSD and DragonFlyBSD
* [#2236](https://github.com/icinga/icinga2/issues/2236) (Packages): Enable parallel builds for the Debian package
* [#2219](https://github.com/icinga/icinga2/issues/2219): Icinga 2 should use less RAM
* [#2218](https://github.com/icinga/icinga2/issues/2218) (Documentation): Documentation: Update Icinga Web 2 installation
* [#2217](https://github.com/icinga/icinga2/issues/2217) (Performance Data): Add GelfWriter for writing log events to graylog2/logstash
* [#2213](https://github.com/icinga/icinga2/issues/2213): Optimize class layout
* [#2204](https://github.com/icinga/icinga2/issues/2204) (ITL): Plugin Check Commands: disk is missing '-p', 'x' parameter
* [#2203](https://github.com/icinga/icinga2/issues/2203) (Configuration): Revamp sample configuration: add NodeName host, move services into apply rules schema
* [#2189](https://github.com/icinga/icinga2/issues/2189) (Configuration): Refactor AST into multiple classes
* [#2187](https://github.com/icinga/icinga2/issues/2187) (Configuration): Implement support for arbitrarily complex indexers
* [#2184](https://github.com/icinga/icinga2/issues/2184) (Configuration): Generate objects using apply with foreach in arrays or dictionaries \(key =\> value\)
* [#2183](https://github.com/icinga/icinga2/issues/2183) (Configuration): Support dictionaries in custom attributes
* [#2182](https://github.com/icinga/icinga2/issues/2182) (Cluster): Execute remote commands on the agent w/o local objects by passing custom attributes
* [#2179](https://github.com/icinga/icinga2/issues/2179): Implement keys\(\)
* [#2178](https://github.com/icinga/icinga2/issues/2178) (CLI): Cli command Node: Disable notifications feature on client nodes
* [#2175](https://github.com/icinga/icinga2/issues/2175) (Documentation): Documentation for arrays & dictionaries in custom attributes and their usage in apply rules for
* [#2161](https://github.com/icinga/icinga2/issues/2161) (CLI): Cli Command: Rename 'agent' to 'node'
* [#2160](https://github.com/icinga/icinga2/issues/2160) (Documentation): Documentation: Explain how to manage agent config in central repository
* [#2158](https://github.com/icinga/icinga2/issues/2158) (Cluster): Require --zone to be specified for "node setup"
* [#2152](https://github.com/icinga/icinga2/issues/2152) (Cluster): Rename --agent to --zone \(for blacklist/whitelist\)
* [#2150](https://github.com/icinga/icinga2/issues/2150) (Documentation): Documentation: Move troubleshooting after the getting started chapter
* [#2143](https://github.com/icinga/icinga2/issues/2143) (Documentation): Documentation: Revamp getting started with 1 host and multiple \(service\) applies
* [#2140](https://github.com/icinga/icinga2/issues/2140) (CLI): Cli: Use Node Blacklist functionality in 'node update-config'
* [#2138](https://github.com/icinga/icinga2/issues/2138) (CLI): Find a better name for 'repository commit --clear'
* [#2131](https://github.com/icinga/icinga2/issues/2131) (Configuration): Set host/service variable in apply rules
* [#2130](https://github.com/icinga/icinga2/issues/2130) (Documentation): Documentation: Mention 'icinga2 object list' in config validation
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
* [#2041](https://github.com/icinga/icinga2/issues/2041) (Documentation): Documentation: Cli Commands
* [#2031](https://github.com/icinga/icinga2/issues/2031) (Graphite): GraphiteWriter: Add support for customized metric prefix names
* [#2024](https://github.com/icinga/icinga2/issues/2024) (Documentation): Documentation: Add support for locally-scoped variables for host/service in applied Dependency
* [#2013](https://github.com/icinga/icinga2/issues/2013) (Documentation): Documentation: Add host/services variables in apply rules 
* [#2003](https://github.com/icinga/icinga2/issues/2003): macro processor needs an array printer
* [#1999](https://github.com/icinga/icinga2/issues/1999) (CLI): Cli command: Repository
* [#1998](https://github.com/icinga/icinga2/issues/1998) (Documentation): Documentation: Agent/Satellite Setup
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
* [#1972](https://github.com/icinga/icinga2/issues/1972) (Documentation): Document how to use multiple assign/ignore statements with logical "and" & "or"
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
* [#2191](https://github.com/icinga/icinga2/issues/2191) (Documentation): link missing in documentation about livestatus
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
* [#2147](https://github.com/icinga/icinga2/issues/2147) (Packages): Feature `checker' is not enabled when installing Icinga 2 using our lates RPM snapshot packages
* [#2142](https://github.com/icinga/icinga2/issues/2142) (Configuration): Icinga2 fails to start due to configuration errors
* [#2141](https://github.com/icinga/icinga2/issues/2141): Build fails
* [#2137](https://github.com/icinga/icinga2/issues/2137): Utility::GetFQDN doesn't work on OS X
* [#2136](https://github.com/icinga/icinga2/issues/2136) (Packages): Build fails on RHEL 6.6
* [#2134](https://github.com/icinga/icinga2/issues/2134): Hosts/services should not have themselves as parents
* [#2133](https://github.com/icinga/icinga2/issues/2133): OnStateLoaded isn't called for objects which don't have any state
* [#2132](https://github.com/icinga/icinga2/issues/2132) (CLI): cli command 'node setup update-config' overwrites existing constants.conf
* [#2129](https://github.com/icinga/icinga2/issues/2129) (Documentation): Fix typos and other small corrections in documentation
* [#2128](https://github.com/icinga/icinga2/issues/2128) (CLI): Cli: Node Setup/Wizard running as root must chown\(\) generated files to icinga daemon user
* [#2127](https://github.com/icinga/icinga2/issues/2127) (Configuration): can't assign Service to Host in nested HostGroup
* [#2125](https://github.com/icinga/icinga2/issues/2125) (Performance Data): Performance data via API is broken
* [#2123](https://github.com/icinga/icinga2/issues/2123) (Packages): Post-update script \(migrate-hosts\) isn't run on RPM-based distributions
* [#2116](https://github.com/icinga/icinga2/issues/2116) (CLI): Cli command: Repository should validate if object exists before add/remove
* [#2106](https://github.com/icinga/icinga2/issues/2106) (Cluster): When replaying logs the secobj attribute is ignored
* [#2095](https://github.com/icinga/icinga2/issues/2095) (Packages): Unity build fails on RHEL 5
* [#2093](https://github.com/icinga/icinga2/issues/2093) (Documentation): Documentation: 1-about contribute links to non-existing report a bug howto
* [#2091](https://github.com/icinga/icinga2/issues/2091) (CLI): Cli command: pki request throws exception on connection failure
* [#2083](https://github.com/icinga/icinga2/issues/2083): CMake warnings on OS X
* [#2077](https://github.com/icinga/icinga2/issues/2077) (CLI): CLI: Auto-completion with colliding arguments
* [#2070](https://github.com/icinga/icinga2/issues/2070) (DB IDO): CLI / MySQL error during vagrant provisioning
* [#2068](https://github.com/icinga/icinga2/issues/2068) (CLI): pki new-cert doesn't check whether the files were successfully written
* [#2065](https://github.com/icinga/icinga2/issues/2065) (DB IDO): Schema upgrade files are missing in /usr/share/icinga2-ido-{mysql,pgsql} 
* [#2063](https://github.com/icinga/icinga2/issues/2063) (CLI): Cli commands: Integers in arrays are printed incorrectly
* [#2058](https://github.com/icinga/icinga2/issues/2058) (Packages): Debian package root permissions interfere with icinga2 cli commands as icinga user
* [#2057](https://github.com/icinga/icinga2/issues/2057) (CLI): failed en/disable feature should return error
* [#2056](https://github.com/icinga/icinga2/issues/2056) (CLI): Commands are auto-completed when they shouldn't be
* [#2052](https://github.com/icinga/icinga2/issues/2052) (Documentation): Wrong usermod command for external command pipe setup
* [#2051](https://github.com/icinga/icinga2/issues/2051) (Configuration): custom attribute name 'type' causes empty vars dictionary
* [#2048](https://github.com/icinga/icinga2/issues/2048) (Compat): Fix reading perfdata in compat/checkresultreader
* [#2042](https://github.com/icinga/icinga2/issues/2042) (Plugins): Setting snmp\_v2 can cause snmp-manubulon-command derived checks to fail
* [#2038](https://github.com/icinga/icinga2/issues/2038) (Configuration): snmp-load checkcommand has a wrong "-T" param value
* [#2037](https://github.com/icinga/icinga2/issues/2037) (Documentation): Documentation: Wrong check command for snmp-int\(erface\)
* [#2034](https://github.com/icinga/icinga2/issues/2034) (Configuration): Importing a CheckCommand in a NotificationCommand results in an exception without stacktrace.
* [#2033](https://github.com/icinga/icinga2/issues/2033) (Documentation): Docs: Default command timeout is 60s not 5m
* [#2029](https://github.com/icinga/icinga2/issues/2029) (Configuration): Error messages for invalid imports missing
* [#2028](https://github.com/icinga/icinga2/issues/2028) (Documentation): Icinga2 docs: link supported operators from sections about apply rules
* [#2026](https://github.com/icinga/icinga2/issues/2026) (Configuration): config parser crashes on unknown attribute in assign
* [#2017](https://github.com/icinga/icinga2/issues/2017) (ITL): ITL: check\_procs and check\_http are missing arguments
* [#2007](https://github.com/icinga/icinga2/issues/2007) (Packages): SLES \(Suse Linux Enterprise Server\) 11 SP3 package dependency failure
* [#2006](https://github.com/icinga/icinga2/issues/2006) (Configuration): snmp-load checkcommand has wrong threshold syntax
* [#2005](https://github.com/icinga/icinga2/issues/2005) (Performance Data): icinga2 returns exponentail perfdata format with check\_nt
* [#2004](https://github.com/icinga/icinga2/issues/2004) (Performance Data): Icinga2 changes perfdata order and removes maximum
* [#2001](https://github.com/icinga/icinga2/issues/2001) (Notifications): default value for "disable\_notifications" in service dependencies is set to "false"
* [#1950](https://github.com/icinga/icinga2/issues/1950) (Configuration): Typo for "HTTP Checks" match in groups.conf
* [#1720](https://github.com/icinga/icinga2/issues/1720) (Notifications): delaying notifications with times.begin should postpone first notification into that window

## 2.1.1 (2014-09-16)

### Enhancement

* [#1962](https://github.com/icinga/icinga2/issues/1962) (Documentation): Extend documentation for icinga-web on Debian systems
* [#1949](https://github.com/icinga/icinga2/issues/1949) (Documentation): Explain event commands and their integration by a real life example \(httpd restart via ssh\)
* [#1939](https://github.com/icinga/icinga2/issues/1939) (Packages): Enable unity build for RPM/Debian packages
* [#1938](https://github.com/icinga/icinga2/issues/1938): Unity builds: Detect whether \_\_COUNTER\_\_ is available
* [#1937](https://github.com/icinga/icinga2/issues/1937) (Packages): Figure out a better way to set the version for snapshot builds
* [#1933](https://github.com/icinga/icinga2/issues/1933): Implement support for unity builds
* [#1932](https://github.com/icinga/icinga2/issues/1932): Ensure that namespaces for INITIALIZE\_ONCE and REGISTER\_TYPE are truly unique
* [#1931](https://github.com/icinga/icinga2/issues/1931): Add include guards for mkclass files
* [#1927](https://github.com/icinga/icinga2/issues/1927) (Documentation): Document how to use @ to escape keywords
* [#1797](https://github.com/icinga/icinga2/issues/1797): Change log message for checking/sending notifications

### Bug

* [#1985](https://github.com/icinga/icinga2/issues/1985) (Documentation): clarify on db ido upgrades
* [#1975](https://github.com/icinga/icinga2/issues/1975): fix memory leak ido\_pgsql
* [#1971](https://github.com/icinga/icinga2/issues/1971) (Livestatus): Livestatus hangs from time to time
* [#1967](https://github.com/icinga/icinga2/issues/1967) (Plugins): fping4 doesn't work correctly with the shipped command-plugins.conf
* [#1966](https://github.com/icinga/icinga2/issues/1966) (Cluster): Segfault using cluster in TlsStream::IsEof
* [#1960](https://github.com/icinga/icinga2/issues/1960) (Packages): GNUInstallDirs.cmake outdated
* [#1958](https://github.com/icinga/icinga2/issues/1958) (Configuration): Manubulon-Plugin conf Filename wrong
* [#1957](https://github.com/icinga/icinga2/issues/1957): Build fails on Haiku
* [#1955](https://github.com/icinga/icinga2/issues/1955) (Cluster): new SSL Errors with too many queued messages
* [#1954](https://github.com/icinga/icinga2/issues/1954): Missing differentiation between service and systemctl
* [#1952](https://github.com/icinga/icinga2/issues/1952) (Performance Data): GraphiteWriter should ignore empty perfdata value
* [#1948](https://github.com/icinga/icinga2/issues/1948): pipe2 returns ENOSYS on GNU Hurd and Debian kfreebsd
* [#1946](https://github.com/icinga/icinga2/issues/1946): Exit code is not initialized for some failed checks
* [#1944](https://github.com/icinga/icinga2/issues/1944) (Packages): service icinga2 status - prints cat error if the service is stopped
* [#1941](https://github.com/icinga/icinga2/issues/1941) (Packages): icinga2 init-script terminates with exit code 0 if $DAEMON is not in place or not executable
* [#1940](https://github.com/icinga/icinga2/issues/1940): icinga2-list-objects complains about Umlauts and stops output
* [#1936](https://github.com/icinga/icinga2/issues/1936) (Packages): Fix rpmlint errors
* [#1935](https://github.com/icinga/icinga2/issues/1935): icinga2-list-objects doesn't work with Python 3
* [#1934](https://github.com/icinga/icinga2/issues/1934) (Configuration): Remove validator for the Script type
* [#1930](https://github.com/icinga/icinga2/issues/1930): "Error parsing performance data" in spite of "enable\_perfdata = false"
* [#1928](https://github.com/icinga/icinga2/issues/1928) (Packages): icinga2.spec: files-attr-not-set for python-icinga2 package
* [#1910](https://github.com/icinga/icinga2/issues/1910) (Cluster): SSL errors with interleaved SSL\_read/write
* [#1862](https://github.com/icinga/icinga2/issues/1862) (Cluster): SSL\_read errors during restart
* [#1849](https://github.com/icinga/icinga2/issues/1849) (Cluster): Too many queued messages
* [#1782](https://github.com/icinga/icinga2/issues/1782): make test fails on openbsd
* [#1522](https://github.com/icinga/icinga2/issues/1522): Link libcJSON against libm

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

* [#1924](https://github.com/icinga/icinga2/issues/1924) (Documentation): add example selinux policy for external command pipe
* [#1915](https://github.com/icinga/icinga2/issues/1915) (Documentation): how to add a new cluster node
* [#1897](https://github.com/icinga/icinga2/issues/1897) (Documentation): Add documentation for icinga2-list-objects
* [#1889](https://github.com/icinga/icinga2/issues/1889) (Documentation): Enhance Graphite Writer description
* [#1888](https://github.com/icinga/icinga2/issues/1888) (Packages): Recommend related packages on SUSE distributions
* [#1887](https://github.com/icinga/icinga2/issues/1887) (Installation): Clean up spec file
* [#1879](https://github.com/icinga/icinga2/issues/1879): Enhance logging for perfdata/graphitewriter
* [#1871](https://github.com/icinga/icinga2/issues/1871) (Configuration): add search path for icinga2.conf
* [#1867](https://github.com/icinga/icinga2/issues/1867) (Documentation): Add systemd options: enable, journal
* [#1865](https://github.com/icinga/icinga2/issues/1865) (Documentation): add section about disabling re-notifications
* [#1864](https://github.com/icinga/icinga2/issues/1864) (Documentation): Add section for reserved keywords
* [#1847](https://github.com/icinga/icinga2/issues/1847) (Documentation): Explain how the order attribute works in commands
* [#1843](https://github.com/icinga/icinga2/issues/1843) (DB IDO): delay ido connect in ha cluster
* [#1810](https://github.com/icinga/icinga2/issues/1810): Change log level for failed commands
* [#1807](https://github.com/icinga/icinga2/issues/1807) (Documentation): Better explanation for HA config cluster
* [#1788](https://github.com/icinga/icinga2/issues/1788): Release 2.1
* [#1787](https://github.com/icinga/icinga2/issues/1787) (Documentation): Documentation for zones and cluster permissions
* [#1786](https://github.com/icinga/icinga2/issues/1786) (Configuration): Information for config objects
* [#1761](https://github.com/icinga/icinga2/issues/1761) (Documentation): Migration: note on check command timeouts
* [#1760](https://github.com/icinga/icinga2/issues/1760) (Plugins): Plugin Check Commands: add manubulon snmp plugins
* [#1548](https://github.com/icinga/icinga2/issues/1548) (Cluster): Log replay sends messages to instances which shouldn't get those messages
* [#1546](https://github.com/icinga/icinga2/issues/1546) (Cluster): Better cluster support for notifications / IDO
* [#1491](https://github.com/icinga/icinga2/issues/1491) (Cluster): Better log messages for cluster changes
* [#977](https://github.com/icinga/icinga2/issues/977) (Cluster): Cluster support for modified attributes

### Bug

* [#1923](https://github.com/icinga/icinga2/issues/1923) (Packages): 64-bit RPMs are not installable
* [#1916](https://github.com/icinga/icinga2/issues/1916): Build fails with Boost 1.56
* [#1913](https://github.com/icinga/icinga2/issues/1913) (Documentation): Keyword "required" used inconsistently for host and service "icon\_image\*" attributes
* [#1905](https://github.com/icinga/icinga2/issues/1905) (Documentation): Update command arguments 'set\_if' and beautify error message
* [#1903](https://github.com/icinga/icinga2/issues/1903) (Cluster): Host and service checks stuck in "pending" when hostname = localhost a parent/satellite setup
* [#1902](https://github.com/icinga/icinga2/issues/1902): Commands are processed multiple times
* [#1896](https://github.com/icinga/icinga2/issues/1896): check file permissions in /var/cache/icinga2
* [#1885](https://github.com/icinga/icinga2/issues/1885) (Packages): enforce /usr/lib as base for the cgi path on SUSE distributions
* [#1884](https://github.com/icinga/icinga2/issues/1884): External command pipe: Too many open files
* [#1883](https://github.com/icinga/icinga2/issues/1883) (Installation): use \_rundir macro for configuring the run directory
* [#1881](https://github.com/icinga/icinga2/issues/1881) (Documentation): clarify on which config tools are available
* [#1873](https://github.com/icinga/icinga2/issues/1873) (Packages): make install does not install the db-schema
* [#1872](https://github.com/icinga/icinga2/issues/1872) (Documentation): Wrong parent in Load Distribution
* [#1868](https://github.com/icinga/icinga2/issues/1868) (Documentation): Wrong object attribute 'enable\_flap\_detection'
* [#1819](https://github.com/icinga/icinga2/issues/1819): ExternalCommandListener fails open pipe: Too many open files

## 2.0.2 (2014-08-07)

### Notes

* DB IDO schema upgrade required (new schema version: 1.11.6)

### Enhancement

* [#1830](https://github.com/icinga/icinga2/issues/1830) (Plugins): Plugin Check Commands: Add timeout option to check\_ssh
* [#1826](https://github.com/icinga/icinga2/issues/1826): Print application paths for --version
* [#1785](https://github.com/icinga/icinga2/issues/1785): Release 2.0.2
* [#1784](https://github.com/icinga/icinga2/issues/1784) (Configuration): Require command to be an array when the arguments attribute is used
* [#1781](https://github.com/icinga/icinga2/issues/1781) (Plugins): Plugin Check Commands: Add expect option to check\_http
* [#1780](https://github.com/icinga/icinga2/issues/1780) (Packages): Rename README to README.md
* [#1763](https://github.com/icinga/icinga2/issues/1763) (Packages): Build packages for el7
* [#1338](https://github.com/icinga/icinga2/issues/1338) (Packages): SUSE packages

### Bug

* [#1861](https://github.com/icinga/icinga2/issues/1861): write startup error messages to error.log
* [#1858](https://github.com/icinga/icinga2/issues/1858): event command execution does not call finish handler
* [#1855](https://github.com/icinga/icinga2/issues/1855): Startup logfile is not flushed to disk
* [#1853](https://github.com/icinga/icinga2/issues/1853) (DB IDO): exit application if ido schema version does not match
* [#1852](https://github.com/icinga/icinga2/issues/1852): Error handler for getaddrinfo must use gai\_strerror
* [#1848](https://github.com/icinga/icinga2/issues/1848): Missing space in error message
* [#1845](https://github.com/icinga/icinga2/issues/1845) (Packages): Remove if\(NOT DEFINED ICINGA2\_SYSCONFIGFILE\) in etc/initsystem/CMakeLists.txt
* [#1842](https://github.com/icinga/icinga2/issues/1842) (Packages): incorrect sysconfig path on sles11
* [#1840](https://github.com/icinga/icinga2/issues/1840): \[Patch\] Fix build issue and crash found on Solaris, potentially other Unix OSes
* [#1839](https://github.com/icinga/icinga2/issues/1839): Icinga 2 crashes during startup
* [#1834](https://github.com/icinga/icinga2/issues/1834) (Cluster): High Availablity does not synchronise the data like expected
* [#1829](https://github.com/icinga/icinga2/issues/1829): Service icinga2 reload command does not cause effect
* [#1828](https://github.com/icinga/icinga2/issues/1828): Fix notification definition if no host\_name / service\_description given
* [#1825](https://github.com/icinga/icinga2/issues/1825) (ITL): The "ssl" check command always sets -D
* [#1821](https://github.com/icinga/icinga2/issues/1821) (ITL): Order doesn't work in check ssh command
* [#1820](https://github.com/icinga/icinga2/issues/1820) (Installation): Repo Error on RHEL 6.5
* [#1816](https://github.com/icinga/icinga2/issues/1816): Config validation without filename argument fails with unhandled exception
* [#1813](https://github.com/icinga/icinga2/issues/1813) (Performance Data): GraphiteWriter: Malformatted integer values
* [#1802](https://github.com/icinga/icinga2/issues/1802) (Documentation): wrong path for the file 'localhost.conf'
* [#1801](https://github.com/icinga/icinga2/issues/1801) (Documentation): Missing documentation about implicit dependency
* [#1800](https://github.com/icinga/icinga2/issues/1800) (Cluster): TLS Connections still unstable in 2.0.1
* [#1796](https://github.com/icinga/icinga2/issues/1796): "order" attribute doesn't seem to work as expected
* [#1792](https://github.com/icinga/icinga2/issues/1792) (Configuration): sample config: add check commands location hint \(itl/plugin check commands\)
* [#1791](https://github.com/icinga/icinga2/issues/1791) (Documentation): icinga Web: wrong path to command pipe
* [#1789](https://github.com/icinga/icinga2/issues/1789) (Documentation): update installation with systemd usage
* [#1779](https://github.com/icinga/icinga2/issues/1779) (Configuration): Remove superfluous quotes and commas in dictionaries
* [#1778](https://github.com/icinga/icinga2/issues/1778): Event Commands are triggered in OK HARD state everytime
* [#1775](https://github.com/icinga/icinga2/issues/1775): additional group rights missing when Icinga started with -u and -g
* [#1774](https://github.com/icinga/icinga2/issues/1774) (Cluster): Missing detailed error messages on ApiListener SSL Errors
* [#1766](https://github.com/icinga/icinga2/issues/1766): RPMLint security warning - missing-call-to-setgroups-before-setuid /usr/sbin/icinga2
* [#1762](https://github.com/icinga/icinga2/issues/1762) (Documentation): clarify on which features are required for classic ui/web/web2
* [#1757](https://github.com/icinga/icinga2/issues/1757) (DB IDO): NULL vs empty string
* [#1754](https://github.com/icinga/icinga2/issues/1754) (Installation): Location of the run directory is hard coded and bound to "local\_state\_dir"
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
* [#1699](https://github.com/icinga/icinga2/issues/1699) (Packages): Classic UI Debian/Ubuntu: apache 2.4 requires 'a2enmod cgi' & apacheutils installed
* [#1684](https://github.com/icinga/icinga2/issues/1684) (Notifications): Notifications not always triggered
* [#1674](https://github.com/icinga/icinga2/issues/1674): ipmi-sensors segfault due to stack size
* [#1666](https://github.com/icinga/icinga2/issues/1666) (DB IDO): objects and their ids are inserted twice

## 2.0.1 (2014-07-10)

### Notes

Bugfix release

### Enhancement

* [#1765](https://github.com/icinga/icinga2/issues/1765) (Documentation): change docs.icinga.org/icinga2/latest to git master
* [#1739](https://github.com/icinga/icinga2/issues/1739) (ITL): Add more options to snmp check
* [#1713](https://github.com/icinga/icinga2/issues/1713) (Configuration): Add port option to check imap/pop/smtp and a new dig
* [#1049](https://github.com/icinga/icinga2/issues/1049) (Livestatus): OutputFormat python

### Bug

* [#1777](https://github.com/icinga/icinga2/issues/1777) (Documentation): event command execution cases are missing
* [#1773](https://github.com/icinga/icinga2/issues/1773) (Notifications): Problem with enable\_notifications and retained state
* [#1772](https://github.com/icinga/icinga2/issues/1772) (Notifications): enable\_notifications = false for users has no effect
* [#1771](https://github.com/icinga/icinga2/issues/1771) (Cluster): Icinga crashes after "Too many queued messages"
* [#1769](https://github.com/icinga/icinga2/issues/1769): Build fails when MySQL is not installed
* [#1767](https://github.com/icinga/icinga2/issues/1767): Increase icinga.cmd Limit
* [#1764](https://github.com/icinga/icinga2/issues/1764) (Installation): ICINGA2\_SYSCONFIGFILE should use full path using CMAKE\_INSTALL\_FULL\_SYSCONFDIR
* [#1753](https://github.com/icinga/icinga2/issues/1753) (Configuration): icinga2-sign-key creates ".crt" and ".key" files when the CA passphrase is invalid
* [#1751](https://github.com/icinga/icinga2/issues/1751) (Configuration): icinga2-build-ca shouldn't prompt for DN
* [#1749](https://github.com/icinga/icinga2/issues/1749): TLS connections are still unstable
* [#1745](https://github.com/icinga/icinga2/issues/1745): Icinga stops updating IDO after a while
* [#1743](https://github.com/icinga/icinga2/issues/1743) (Configuration): Please add --sni option to http check command
* [#1742](https://github.com/icinga/icinga2/issues/1742) (Documentation): Documentation for || and && is missing
* [#1740](https://github.com/icinga/icinga2/issues/1740) (Notifications): Notifications causing segfault from exim
* [#1737](https://github.com/icinga/icinga2/issues/1737) (DB IDO): icinga2-ido-pgsql snapshot package missing dependecy dbconfig-common
* [#1736](https://github.com/icinga/icinga2/issues/1736): Remove line number information from stack traces
* [#1734](https://github.com/icinga/icinga2/issues/1734): Check command result doesn't match
* [#1731](https://github.com/icinga/icinga2/issues/1731): Dependencies should cache their parent and child object
* [#1727](https://github.com/icinga/icinga2/issues/1727): $SERVICEDESC$ isn't getting converted correctly
* [#1724](https://github.com/icinga/icinga2/issues/1724): Improve systemd service definition
* [#1716](https://github.com/icinga/icinga2/issues/1716) (Cluster): Icinga doesn't send SetLogPosition messages when one of the endpoints fails to connect
* [#1712](https://github.com/icinga/icinga2/issues/1712): parsing of double defined command can generate unexpected errors
* [#1709](https://github.com/icinga/icinga2/issues/1709) (Packages): htpasswd should be installed with icinga2-classicui on Ubuntu
* [#1704](https://github.com/icinga/icinga2/issues/1704): Reminder notifications are sent on disabled services 
* [#1702](https://github.com/icinga/icinga2/issues/1702) (Documentation): Array section confusing
* [#1698](https://github.com/icinga/icinga2/issues/1698): icinga2 cannot be built with both systemd and init.d files
* [#1697](https://github.com/icinga/icinga2/issues/1697) (Livestatus): Thruk Panorama View cannot query Host Status
* [#1696](https://github.com/icinga/icinga2/issues/1696) (Packages): Copyright problems
* [#1695](https://github.com/icinga/icinga2/issues/1695): icinga2.state could not be opened
* [#1691](https://github.com/icinga/icinga2/issues/1691): build warnings
* [#1655](https://github.com/icinga/icinga2/issues/1655) (Packages): Debian package icinga2-classicui needs versioned dependency of icinga-cgi\*
* [#1644](https://github.com/icinga/icinga2/issues/1644) (Cluster): base64 on CentOS 5 fails to read certificate bundles
* [#1639](https://github.com/icinga/icinga2/issues/1639) (Cluster): Deadlock in ApiListener::RelayMessage
* [#1609](https://github.com/icinga/icinga2/issues/1609): application fails to start on wrong log file permissions but does not tell about it
* [#1206](https://github.com/icinga/icinga2/issues/1206) (DB IDO): PostgreSQL string escaping

## 2.0.0 (2014-06-16)

### Notes

First official release

### Enhancement

* [#1690](https://github.com/icinga/icinga2/issues/1690) (ITL): improve predefined command-plugins
* [#1689](https://github.com/icinga/icinga2/issues/1689) (Documentation): explain the icinga 2 reload
* [#1686](https://github.com/icinga/icinga2/issues/1686) (Installation): man pages for scripts
* [#1675](https://github.com/icinga/icinga2/issues/1675) (Documentation): add a note on no length restrictions for plugin output / perfdata
* [#1636](https://github.com/icinga/icinga2/issues/1636) (Documentation): Update command definitions to use argument conditions
* [#1600](https://github.com/icinga/icinga2/issues/1600): Prepare 2.0.0 release
* [#1575](https://github.com/icinga/icinga2/issues/1575) (Cluster): Cluster: global zone for all nodes
* [#1572](https://github.com/icinga/icinga2/issues/1572) (Documentation): change docs.icinga.org/icinga2/snapshot to 'latest'
* [#1348](https://github.com/icinga/icinga2/issues/1348): move vagrant box into dedicated demo project
* [#1342](https://github.com/icinga/icinga2/issues/1342) (Installation): Less verbose start output using the initscript
* [#1341](https://github.com/icinga/icinga2/issues/1341): Revamp migration script
* [#1322](https://github.com/icinga/icinga2/issues/1322): Update website for release
* [#1320](https://github.com/icinga/icinga2/issues/1320): Update documentation for 2.0
* [#1319](https://github.com/icinga/icinga2/issues/1319) (Tests): Release tests
* [#1302](https://github.com/icinga/icinga2/issues/1302) (Documentation): Replace Sphinx with Icinga Web 2 Doc Module
* [#788](https://github.com/icinga/icinga2/issues/788) (Packages): add systemd support

### Bug

* [#1694](https://github.com/icinga/icinga2/issues/1694): Separate CMakeLists.txt for etc/initsystem
* [#1685](https://github.com/icinga/icinga2/issues/1685) (Installation): Cleanup installer for 2.0 supported features
* [#1683](https://github.com/icinga/icinga2/issues/1683) (Installation): remove 0.0.x schema upgrade files
* [#1682](https://github.com/icinga/icinga2/issues/1682) (Configuration): logrotate.conf file should rotate log files as icinga user
* [#1681](https://github.com/icinga/icinga2/issues/1681) (Documentation): Add instructions to install debug symbols on debian systems
* [#1680](https://github.com/icinga/icinga2/issues/1680) (Livestatus): Column 'host\_name' does not exist in table 'hosts'
* [#1678](https://github.com/icinga/icinga2/issues/1678) (Livestatus): Nagvis does not work with livestatus \(invalid format\)
* [#1673](https://github.com/icinga/icinga2/issues/1673): OpenSUSE Packages do not enable basic features
* [#1670](https://github.com/icinga/icinga2/issues/1670) (Packages): Ubuntu package Release file lacks 'Suite' line
* [#1669](https://github.com/icinga/icinga2/issues/1669) (Cluster): Segfault with zones without endpoints on config compile
* [#1645](https://github.com/icinga/icinga2/issues/1645) (Packages): Packages are not installable on CentOS 5
* [#1642](https://github.com/icinga/icinga2/issues/1642): Check if host recovery notifications work
* [#1615](https://github.com/icinga/icinga2/issues/1615) (Cluster): Subdirectories in the zone config are not synced
* [#1427](https://github.com/icinga/icinga2/issues/1427): fd-handling in Daemonize incorrect
* [#1312](https://github.com/icinga/icinga2/issues/1312): Permissions error on startup is only logged but not on stderr
* [#907](https://github.com/icinga/icinga2/issues/907) (Packages): icinga2-classicui is not installable on Debian

