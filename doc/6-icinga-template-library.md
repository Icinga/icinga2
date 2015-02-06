# <a id="icinga-template-library"></a> Icinga Template Library

The Icinga Template Library (ITL) implements standard templates and object
definitions for commonly used services.

By default the ITL is included in the `icinga2.conf` configuration file:

    include <itl>

## <a id="itl-generic-templates"></a> Generic Templates

These templates are imported by the provided example configuration.

### <a id="itl-plugin-check-command"></a> plugin-check-command

Command template for check plugins executed by Icinga 2.

The `plugin-check-command` command does not support any vars.

### <a id="itl-plugin-notification-command"></a> plugin-notification-command

Command template for notification scripts executed by Icinga 2.

The `plugin-notification-command` command does not support any vars.

### <a id="itl-plugin-event-command"></a> plugin-event-command

Command template for event handler scripts executed by Icinga 2.

The `plugin-event-command` command does not support any vars.

## <a id="itl-check-commands"></a> Check Commands

These check commands are embedded into Icinga 2 and do not require any external
plugin scripts.

### <a id="itl-icinga"></a> icinga

Check command for the built-in `icinga` check. This check returns performance
data for the current Icinga instance.

The `icinga` check command does not support any vars.

### <a id="itl-icinga-cluster"></a> cluster

Check command for the built-in `cluster` check. This check returns performance
data for the current Icinga instance and connected endpoints.

The `cluster` check command does not support any vars.

### <a id="itl-icinga-cluster-zone"></a> cluster-zone

Check command for the built-in `cluster-zone` check.

Cluster Attributes:

Name         | Description
-------------|---------------
cluster_zone | **Optional.** The zone name. Defaults to "$host.name$".

# <a id="plugin-check-commands"></a> Plugin Check Commands

The Plugin Check Commands provides example configuration for plugin check commands
provided by the Monitoring Plugins project.

By default the Plugin Check Commands are included in the `icinga2.conf` configuration
file:

    include <plugins>

The plugin check commands assume that there's a global constant named `PluginDir`
which contains the path of the plugins from the Monitoring Plugins project.

### <a id="plugin-check-command-apt"></a> apt

Check command for the `check_apt` plugin.

The `apt` check command does not support any vars.


### <a id="plugin-check-command-by-ssh"></a> by_ssh

Check command object for the `check_by_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
by_ssh_address  | **Optional.** The host's address. Defaults to "$address$".
by_ssh_port     | **Optional.** The SSH port. Defaults to 22.
by_ssh_command  | **Optional.** The command that should be executed.
by_ssh_logname  | **Optional.** The SSH username.
by_ssh_identity | **Optional.** The SSH identity.
by_ssh_quiet    | **Optional.** Whether to suppress SSH warnings. Defaults to false.
by_ssh_warn     | **Optional.** The warning threshold.
by_ssh_crit     | **Optional.** The critical threshold.
by_ssh_timeout  | **Optional.** The timeout in seconds.


### <a id="plugin-check-command-dhcp"></a> dhcp

Check command object for the `check_dhcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
dhcp_serverip   | **Optional.** The IP address of the DHCP server which we should get a response from.
dhcp_requestedip| **Optional.** The IP address which we should be offered by a DHCP server.
dhcp_timeout    | **Optional.** The timeout in seconds.
dhcp_interface  | **Optional.** The interface to use.
dhcp_mac        | **Optional.** The MAC address to use in the DHCP request.
dhcp_unicast    | **Optional.** Whether to use unicast requests. Defaults to false.


### <a id="plugin-check-command-dig"></a> dig

Check command object for the `check_dig` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
dig_server           | **Optional.** The DNS server to query. Defaults to "127.0.0.1".
dig_lookup           | **Optional.** The address that should be looked up.


### <a id="plugin-check-command-disk"></a> disk

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            	| Description
------------------------|------------------------
disk_wfree      	| **Optional.** The free space warning threshold in %. Defaults to 20.
disk_cfree      	| **Optional.** The free space critical threshold in %. Defaults to 10.
disk_inode_wfree 	| **Optional.** The free inode warning threshold.
disk_inode_cfree 	| **Optional.** The free inode critical threshold.
disk_partition		| **Optional.** The partition. **Deprecated in 2.3.**
disk_partition_excluded | **Optional.** The excluded partition. **Deprecated in 2.3.**
disk_partitions        	| **Optional.** The partition(s). Multiple partitions must be defined as array.
disk_partitions_excluded | **Optional.** The excluded partition(s). Multiple partitions must be defined as array.
disk_clear               | **Optional.** Clear thresholds.
disk_exact_match       | **Optional.** For paths or partitions specified with -p, only check for exact paths.
disk_errors_only       | **Optional.** Display only devices/mountpoints with errors. May be true or false.
disk_group             | **Optional.** Group paths. Thresholds apply to (free-)space of all partitions together
disk_kilobytes         | **Optional.** Same as --units kB. May be true or false.
disk_local             | **Optional.** Only check local filesystems. May be true or false.
disk_stat_remote_fs    | **Optional.** Only check local filesystems against thresholds. Yet call stat on remote filesystems to test if they are accessible (e.g. to detect Stale NFS Handles). Myy be true or false
disk_mountpoint        | **Optional.** Display the mountpoint instead of the partition. May be true or false.
disk_megabytes         | **Optional.** Same as --units MB. May be true or false.
disk_all               | **Optional.** Explicitly select all paths. This is equivalent to -R '.*'. May be true or false.
disk_eregi_path        | **Optional.** Case insensitive regular expression for path/partition (may be repeated).
disk_ereg_path         | **Optional.** Regular expression for path or partition (may be repeated).
disk_ignore_eregi_path | **Optional.** Regular expression to ignore selected path/partition (case insensitive) (may be repeated).
disk_ignore_ereg_path  | **Optional.** Regular expression to ignore selected path or partition (may be repeated).
disk_timeout           | **Optional.** Seconds before connection times out (default: 10).
disk_units             | **Optional.** Choose bytes, kB, MB, GB, TB (default: MB).
disk_exclude_type      | **Optional.** Ignore all filesystems of indicated type (may be repeated).


### <a id="plugin-check-command-dns"></a> dns

Check command object for the `check_dns` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
dns_lookup           | **Optional.** The hostname or IP to query the DNS for. Defaults to $host_name$.
dns_server           | **Optional.** The DNS server to query. Defaults to the server configured in the OS.
dns_expected_answer  | **Optional.** The answer to look for. A hostname must end with a dot. **Deprecated in 2.3.**
dns_expected_answers | **Optional.** The answer(s) to look for. A hostname must end with a dot. Multiple answers must be defined as array.
dns_authoritative    | **Optional.** Expect the server to send an authoritative answer.


### <a id="plugin-check-command-dummy"></a> dummy

Check command object for the `check_dummy` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
dummy_text      | **Optional.** Plugin output. Defaults to "Check was successful.".


### <a id="plugin-check-command-fping4"></a> fping4

Check command object for the `check_fping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
fping_address   | **Optional.** The host's IPv4 address. Defaults to "$address$".
fping_wrta      | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
fping_wpl       | **Optional.** The packet loss warning threshold in %. Defaults to 5.
fping_crta      | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
fping_cpl       | **Optional.** The packet loss critical threshold in %. Defaults to 15.
fping_number    | **Optional.** The number of packets to send. Defaults to 5.
fping_interval  | **Optional.** The interval between packets in milli-seconds. Defaults to 500.
fping_bytes	| **Optional.** The size of ICMP packet.
fping_target_timeout | **Optional.** The target timeout in milli-seconds.
fping_source_ip | **Optional.** The name or ip address of the source ip.
fping_source_interface | **Optional.** The source interface name.


### <a id="plugin-check-command-fping6"></a> fping6

Check command object for the `check_fping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
fping_address   | **Optional.** The host's IPv6 address. Defaults to "$address6$".
fping_wrta      | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
fping_wpl       | **Optional.** The packet loss warning threshold in %. Defaults to 5.
fping_crta      | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
fping_cpl       | **Optional.** The packet loss critical threshold in %. Defaults to 15.
fping_number    | **Optional.** The number of packets to send. Defaults to 5.
fping_interval  | **Optional.** The interval between packets in milli-seconds. Defaults to 500.
fping_bytes	| **Optional.** The size of ICMP packet.
fping_target_timeout | **Optional.** The target timeout in milli-seconds.
fping_source_ip | **Optional.** The name or ip address of the source ip.
fping_source_interface | **Optional.** The source interface name.


### <a id="plugin-check-command-ftp"></a> ftp

Check command object for the `check_ftp` plugin.

Custom Attributes:

Name               | Description
-------------------|--------------
ftp_address        | **Optional.** The host's address. Defaults to "$address$".


### <a id="plugin-check-command-hostalive"></a> hostalive

Check command object for the `check_ping` plugin with host check default values.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 80.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 100.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


### <a id="plugin-check-command-hpjd"></a> hpjd

Check command object for the `check_hpjd` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
hpjd_address    | **Optional.** The host's address. Defaults to "$address$".
hpjd_port       | **Optional.** The host's SNMP port. Defaults to 161.
hpjd_community  | **Optional.** The SNMP community. Defaults  to "public".


### <a id="plugin-check-command-http"></a> http

Check command object for the `check_http` plugin.

Custom Attributes:

Name                     | Description
-------------------------|--------------
http_address             | **Optional.** The host's address. Defaults to "$address".
http_vhost               | **Optional.** The virtual host that should be sent in the "Host" header.
http_uri                 | **Optional.** The request URI.
http_port                | **Optional.** The TCP port. Defaults to 80 when not using SSL, 443 otherwise.
http_ssl                 | **Optional.** Whether to use SSL. Defaults to false.
http_sni                 | **Optional.** Whether to use SNI. Defaults to false.
http_auth_pair           | **Optional.** Add 'username:password' authorization pair.
http_proxy_auth_pair     | **Optional.** Add 'username:password' authorization pair for proxy.
http_ignore_body         | **Optional.** Don't download the body, just the headers.
http_linespan            | **Optional.** Allow regex to span newline.
http_expect_body_regex   | **Optional.** A regular expression which the body must match against. Incompatible with http_ignore_body.
http_expect_body_eregi   | **Optional.** A case-insensitive expression which the body must match against. Incompatible with http_ignore_body.
http_invertregex         | **Optional.** Changes behaviour of http_expect_body_regex and http_expect_body_eregi to return CRITICAL if found, OK if not.
http_warn_time           | **Optional.** The warning threshold.
http_critical_time       | **Optional.** The critical threshold.
http_expect              | **Optional.** Comma-delimited list of strings, at least one of them is expected in the first (status) line of the server response. Default: HTTP/1.
http_certificate         | **Optional.** Minimum number of days a certificate has to be valid. Port defaults to 443.
http_clientcert          | **Optional.** Name of file contains the client certificate (PEM format).
http_privatekey          | **Optional.** Name of file contains the private key (PEM format).
http_headerstring        | **Optional.** String to expect in the response headers.
http_string              | **Optional.** String to expect in the content.
http_post                | **Optional.** URL encoded http POST data.
http_method              | **Optional.** Set http method (for example: HEAD, OPTIONS, TRACE, PUT, DELETE).
http_maxage              | **Optional.** Warn if document is more than seconds old.
http_contenttype         | **Optional.** Specify Content-Type header when POSTing.
http_useragent           | **Optional.** String to be sent in http header as User Agent.
http_header              | **Optional.** Any other tags to be sent in http header.
http_extendedperfdata    | **Optional.** Print additional perfdata. Defaults to "false".
http_onredirect          | **Optional.** How to handle redirect pages. Possible values: "ok" (default), "warning", "critical", "follow", "sticky" (like follow but stick to address), "stickyport" (like sticky but also to port)
http_pagesize            | **Optional.** Minimum page size required:Maximum page size required.
http_timeout             | **Optional.** Seconds before connection times out.


### <a id="plugin-check-command-icmp"></a> icmp

Check command object for the `check_icmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
icmp_address    | **Optional.** The host's address. This can either be a single address or an array of addresses. Defaults to "$address$".
icmp_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
icmp_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
icmp_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
icmp_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
icmp_source     | **Optional.** The source IP address to send packets from.
icmp_packets    | **Optional.** The number of packets to send. Defaults to 5.
icmp_packet_interval | **Optional** The maximum packet interval. Defaults to 80 (milliseconds).
icmp_target_interval | **Optional.** The maximum target interval.
icmp_hosts_alive | **Optional.** The number of hosts which have to be alive for the check to succeed.
icmp_data_bytes | **Optional.** Payload size for each ICMP request. Defaults to 8.
icmp_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 10 (seconds).


### <a id="plugin-check-command-imap"></a> imap

Check command object for the `check_imap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
imap_address    | **Optional.** The host's address. Defaults to "$address$".
imap_port       | **Optional.** The port that should be checked. Defaults to 143.


### <a id="plugin-check-command-load"></a> load

Check command object for the `check_load` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
load_wload1     | **Optional.** The 1-minute warning threshold. Defaults to 5.
load_wload5     | **Optional.** The 5-minute warning threshold. Defaults to 4.
load_wload15    | **Optional.** The 15-minute warning threshold. Defaults to 3.
load_cload1     | **Optional.** The 1-minute critical threshold. Defaults to 10.
load_cload5     | **Optional.** The 5-minute critical threshold. Defaults to 6.
load_cload15    | **Optional.** The 15-minute critical threshold. Defaults to 4.


### <a id="plugin-check-command-nrpe"></a> nrpe

Check command object for the `check_nrpe` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
nrpe_address    | **Optional.** The host's address. Defaults to "$address$".
nrpe_port       | **Optional.** The NRPE port. Defaults to 5668.
nrpe_command    | **Optional.** The command that should be executed.
nrpe_no_ssl     | **Optional.** Whether to disable SSL or not. Defaults to `false`.
nrpe_timeout_unknown | **Optional.** Whether to set timeouts to unknown instead of critical state. Defaults to `false`.
nrpe_timeout    | **Optional.** The timeout in seconds.
nrpe_arguments	| **Optional.** Arguments that should be passed to the command. Multiple arguments must be defined as array.


### <a id="plugin-check-command-nscp"></a> nscp

Check command object for the `check_nt` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
nscp_address    | **Optional.** The host's address. Defaults to "$address$".
nscp_port       | **Optional.** The NSClient++ port. Defaults to 12489.
nscp_password   | **Optional.** The NSClient++ password.
nscp_variable   | **Required.** The variable that should be checked.
nscp_params     | **Optional.** Parameters for the query. Multiple parameters must be defined as array.
nscp_warn       | **Optional.** The warning threshold.
nscp_crit       | **Optional.** The critical threshold.
nscp_timeout    | **Optional.** The query timeout in seconds.


### <a id="plugin-check-command-ntp-time"></a> ntp_time

Check command object for the `check_ntp_time` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ntp_address     | **Optional.** The host's address. Defaults to "$address$".


### <a id="plugin-check-command-passive"></a> passive

Specialised check command object for passive checks executing the `check_dummy` plugin with appropriate default values.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 3.
dummy_text      | **Optional.** Plugin output. Defaults to "No Passive Check Result Received.".


### <a id="plugin-check-command-ping4"></a> ping4

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

### <a id="plugin-check-command-ping6"></a> ping6

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv6 address. Defaults to "$address6$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


### <a id="plugin-check-command-pop"></a> pop

Check command object for the `check_pop` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
pop_address     | **Optional.** The host's address. Defaults to "$address$".
pop_port        | **Optional.** The port that should be checked. Defaults to 110.


### <a id="plugin-check-command-processes"></a> procs

Check command object for the `check_procs` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
procs_warning        | **Optional.** The process count warning threshold. Defaults to 250.
procs_critical       | **Optional.** The process count critical threshold. Defaults to 400.
procs_metric         | **Optional.** Check thresholds against metric.
procs_timeout        | **Optional.** Seconds before plugin times out.
procs_traditional    | **Optional.** Filter own process the traditional way by PID instead of /proc/pid/exe. Defaults to "false".
procs_state          | **Optional.** Only scan for processes that have one or more of the status flags you specify.
procs_ppid           | **Optional.** Only scan for children of the parent process ID indicated.
procs_vsz            | **Optional.** Only scan for processes with VSZ higher than indicated.
procs_rss            | **Optional.** Only scan for processes with RSS higher than indicated.
procs_pcpu           | **Optional.** Only scan for processes with PCPU higher than indicated.
procs_user           | **Optional.** Only scan for processes with user name or ID indicated.
procs_argument       | **Optional.** Only scan for processes with args that contain STRING.
procs_argument_regex | **Optional.** Only scan for processes with args that contain the regex STRING.
procs_command        | **Optional.** Only scan for exact matches of COMMAND (without path).
procs_nokthreads     | **Optional.** Only scan for non kernel threads. Defaults to "false".


### <a id="plugin-check-command-running-kernel"></a> running_kernel

Check command object for the `check_running_kernel` plugin
provided by the `nagios-plugins-contrib` package on Debian.

The `running_kernel` check command does not support any vars.


### <a id="plugin-check-command-simap"></a> simap

Check command object for the `check_simap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
simap_address   | **Optional.** The host's address. Defaults to "$address$".
simap_port      | **Optional.** The host's port.


### <a id="plugin-check-command-smtp"></a> smtp

Check command object for the `check_smtp` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
smtp_address         | **Optional.** The host's address. Defaults to "$address$".
smtp_port            | **Optional.** The port that should be checked. Defaults to 25.
smtp_mail_from       | **Optional.** Test a MAIL FROM command with the given email address.


### <a id="plugin-check-command-snmp"></a> snmp

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name                | Description
--------------------|--------------
snmp_address        | **Optional.** The host's address. Defaults to "$address$".
snmp_oid            | **Required.** The SNMP OID.
snmp_community      | **Optional.** The SNMP community. Defaults to "public".
snmp_warn           | **Optional.** The warning threshold.
snmp_crit           | **Optional.** The critical threshold.
snmp_string         | **Optional.** Return OK state if the string matches exactly with the output value
snmp_ereg           | **Optional.** Return OK state if extended regular expression REGEX matches with the output value
snmp_eregi          | **Optional.** Return OK state if case-insensitive extended REGEX matches with the output value
snmp_label          | **Optional.** Prefix label for output value
snmp_invert_search  | **Optional.** Invert search result and return CRITICAL state if found
snmp_units          | **Optional.** Units label(s) for output value (e.g., 'sec.').
snmp_timeout        | **Optional.** The command timeout in seconds. Defaults to 10 seconds.

### <a id="plugin-check-command-snmpv3"></a> snmpv3

Check command object for the `check_snmp` plugin, using SNMPv3 authentication and encryption options.

Custom Attributes:

Name              | Description
------------------|--------------
snmpv3_address    | **Optional.** The host's address. Defaults to "$address$".
snmpv3_user       | **Required.** The username to log in with.
snmpv3_auth_alg   | **Optional.** The authentication algorithm. Defaults to SHA.
snmpv3_auth_key   | **Required.** The authentication key.
snmpv3_priv_alg   | **Optional.** The encryption algorithm. Defaults to AES.
snmpv3_priv_key   | **Required.** The encryption key.
snmpv3_oid        | **Required.** The SNMP OID.
snmpv3_warn       | **Optional.** The warning threshold.
snmpv3_crit       | **Optional.** The critical threshold.
snmpv3_label      | **Optional.** Prefix label for output value.

### <a id="plugin-check-command-snmp-uptime"></a> snmp-uptime

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
snmp_community  | **Optional.** The SNMP community. Defaults to "public".


### <a id="plugin-check-command-spop"></a> spop

Check command object for the `check_spop` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
spop_address    | **Optional.** The host's address. Defaults to "$address$".
spop_port       | **Optional.** The host's port.


### <a id="plugin-check-command-ssh"></a> ssh

Check command object for the `check_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssh_address     | **Optional.** The host's address. Defaults to "$address$".
ssh_port        | **Optional.** The port that should be checked. Defaults to 22.
ssh_timeout     | **Optional.** Seconds before connection times out. Defaults to 10.


### <a id="plugin-check-command-ssl"></a> ssl

Check command object for the `check_tcp` plugin, using ssl-related options.

Custom Attributes:

Name                          | Description
------------------------------|--------------
ssl_address                   | **Optional.** The host's address. Defaults to "$address$".
ssl_port                      | **Required.** The port that should be checked.
ssl_timeout                   | **Optional.** Timeout in seconds for the connect and handshake. The plugin default is 10 seconds.
ssl_cert_valid_days_warn      | **Optional.** Warning threshold for days before the certificate will expire. When used, ssl_cert_valid_days_critical must also be set.
ssl_cert_valid_days_critical  | **Optional.** Critical threshold for days before the certificate will expire. When used, ssl_cert_valid_days_warn must also be set.


### <a id="plugin-check-command-ssmtp"></a> ssmtp

Check command object for the `check_ssmtp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssmtp_address   | **Required.** The host's address. Defaults to "$address$".
ssmtp_port      | **Optional.** The port that should be checked. Defaults to 465.


### <a id="plugin-check-command-swap"></a> swap

Check command object for the `check_swap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
swap_wfree      | **Optional.** The free swap space warning threshold in %. Defaults to 50.
swap_cfree      | **Optional.** The free swap space critical threshold in %. Defaults to 25.


### <a id="plugin-check-command-tcp"></a> tcp

Check command object for the `check_tcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
tcp_address     | **Optional.** The host's address. Defaults to "$address$".
tcp_port        | **Required.** The port that should be checked.


### <a id="plugin-check-command-udp"></a> udp

Check command object for the `check_udp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
udp_address     | **Optional.** The host's address. Defaults to "$address$".
udp_port        | **Required.** The port that should be checked.


### <a id="plugin-check-command-ups"></a> ups

Check command object for the `check_ups` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ups_address     | **Optional.** The host's address. Defaults to "$address$".
ups_name        | **Optional.** The UPS name. Defaults to `ups`.


### <a id="plugin-check-command-users"></a> users

Check command object for the `check_users` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
users_wgreater  | **Optional.** The user count warning threshold. Defaults to 20.
users_cgreater  | **Optional.** The user count critical threshold. Defaults to 50.


# <a id="snmp-manubulon-plugin-check-commands"></a> SNMP Manubulon Plugin Check Commands

The `SNMP Manubulon Plugin Check Commands` provide example configuration for plugin check
commands provided by the [SNMP Manubulon project](http://nagios.manubulon.com/index_snmp.html).

The SNMP manubulon plugin check commands assume that the global constant named `ManubulonPluginDir`
is set to the path where the Manubublon SNMP plugins are installed.

You can enable these plugin check commands by adding the following the include directive in your
configuration [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf) file:

    include <manubulon>

## Checks by Host Type

**N/A**      : Not available for this type.

**SNMP**     : Available for simple SNMP query.

**??**       : Untested.

**Specific** : Script name for platform specific checks.


  Host type               | Interface  | storage  | load/cpu  | mem | process  | env | specific
  ------------------------|------------|----------|-----------|-----|----------|-----|-------------------------
  Linux                   |   Yes      |   Yes    |   Yes     | Yes |   Yes    | No  |
  Windows                 |   Yes      |   Yes    |   Yes     | Yes |   Yes    | No  | check_snmp_win.pl
  Cisco router/switch     |   Yes      |   N/A    |   Yes     | Yes |   N/A    | Yes |
  HP router/switch        |   Yes      |   N/A    |   Yes     | Yes |   N/A    | No  |
  Bluecoat proxy          |   Yes      |   SNMP   |   Yes     | SNMP|   No     | Yes |
  CheckPoint on SPLAT     |   Yes      |   Yes    |   Yes     | Yes |   Yes    | No  | check_snmp_cpfw.pl
  CheckPoint on Nokia IP  |   Yes      |   Yes    |   Yes     | No  |   ??     | No  | check_snmp_vrrp.pl
  Boostedge               |   Yes      |   Yes    |   Yes     | Yes |   ??     | No  | check_snmp_boostedge.pl
  AS400                   |   Yes      |   Yes    |   Yes     | Yes |   No     | No  |
  NetsecureOne Netbox     |   Yes      |   Yes    |   Yes     | ??  |   Yes    | No  |
  Radware Linkproof       |   Yes      |   N/A    |   SNMP    | SNMP|   No     | No  | check_snmp_linkproof_nhr <br> check_snmp_vrrp.pl
  IronPort                |   Yes      |   SNMP   |   SNMP    | SNMP|   No     | Yes |
  Cisco CSS               |   Yes      |   ??     |   Yes     | Yes |   No     | ??  | check_snmp_css.pl


### <a id="plugin-check-command-snmp-load"></a> snmp-load

Check command object for the [check_snmp_load.pl](http://nagios.manubulon.com/snmp_load.html) plugin.

Custom Attributes:


Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$".
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to "false".
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to "false".
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to "false".
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to "false".
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn               | **Optional.** The warning threshold. Change the `snmp_load_type` var to "netsl" for using 3 values.
snmp_crit               | **Optional.** The critical threshold. Change the `snmp_load_type` var to "netsl" for using 3 values.
snmp_load_type          | **Optional.** Load type. Defaults to "stand". Check all available types in the [snmp load](http://nagios.manubulon.com/snmp_load.html) documentation.
snmp_perf               | **Optional.** Enable perfdata values. Defaults to "true".
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

### <a id="plugin-check-command-snmp-memory"></a> snmp-memory

Check command object for the [check_snmp_mem.pl](http://nagios.manubulon.com/snmp_mem.html) plugin.

Custom Attributes:

Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$".
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to "false".
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to "false".
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to "false".
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to "false".
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn               | **Optional.** The warning threshold.
snmp_crit               | **Optional.** The critical threshold.
snmp_perf               | **Optional.** Enable perfdata values. Defaults to "true".
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

### <a id="plugin-check-command-snmp-storage"></a> snmp-storage

Check command object for the [check_snmp_storage.pl](http://nagios.manubulon.com/snmp_storage.html) plugin.

Custom Attributes:

Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$".
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to "false".
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to "false".
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to "false".
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to "false".
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn               | **Optional.** The warning threshold.
snmp_crit               | **Optional.** The critical threshold.
snmp_storage_name       | **Optional.** Storage name. Default to regex "^/$$". More options available in the [snmp storage](http://nagios.manubulon.com/snmp_storage.html) documentation.
snmp_perf               | **Optional.** Enable perfdata values. Defaults to "true".
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

### <a id="plugin-check-command-snmp-interface"></a> snmp-interface

Check command object for the [check_snmp_int.pl](http://nagios.manubulon.com/snmp_int.html) plugin.

Custom Attributes:

Name                        | Description
----------------------------|--------------
snmp_address                | **Optional.** The host's address. Defaults to "$address$".
snmp_nocrypt                | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to "false".
snmp_community              | **Optional.** The SNMP community. Defaults to "public".
snmp_port                   | **Optional.** The SNMP port connection.
snmp_v2                     | **Optional.** SNMP version to 2c. Defaults to "false".
snmp_v3                     | **Optional.** SNMP version to 3. Defaults to "false".
snmp_login                  | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password               | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass        | **Optional.** Define to use SNMP version 3 priv password. Defaults to "false".
snmp_authprotocol           | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass               | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn                   | **Optional.** The warning threshold.
snmp_crit                   | **Optional.** The critical threshold.
snmp_interface              | **Optional.** Network interface name. Default to regex "eth0".
snmp_interface_perf         | **Optional.** Check the input/ouput bandwidth of the interface. Defaults to "true".
snmp_interface_label        | **Optional.** Add label before speed in output: in=, out=, errors-out=, etc...
snmp_interface_bits_bytes   | **Optional.** Output performance data in bits/s or Bytes/s. **Depends** on snmp_interface_kbits set to "true". Defaults to "true".
snmp_interface_percent      | **Optional.** Output performance data in % of max speed. Defaults to "false".
snmp_interface_kbits        | **Optional.** Make the warning and critical levels in KBits/s. Defaults to "true".
snmp_interface_megabytes    | **Optional.** Make the warning and critical levels in Mbps or MBps. **Depends** on snmp_interface_kbits set to "true". Defaults to "true".
snmp_interface_64bit        | **Optional.** Use 64 bits counters instead of the standard counters when checking bandwidth & performance data for interface >= 1Gbps. Defaults to "false".
snmp_interface_errors       | **Optional.** Add error & discard to Perfparse output. Defaults to "true".
snmp_interface_noregexp     | **Optional.** Do not use regexp to match interface name in description OID. Defaults to "false".
snmp_interface_delta        | **Optional.** Delta time of perfcheck. Defaults to "300" (5 min).
snmp_warncrit_percent       | **Optional.** Make the warning and critical levels in % of reported interface speed. If set **snmp_interface_megabytes** needs to be set to "false". Defaults to "false".
snmp_perf                   | **Optional.** Enable perfdata values. Defaults to "true".
snmp_timeout                | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

### <a id="plugin-check-command-snmp-process"></a> snmp-process

Check command object for the [check_snmp_process.pl](http://nagios.manubulon.com/snmp_process.html) plugin.

Custom Attributes:

Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$".
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to "false".
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to "false".
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to "false".
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to "false".
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default..
snmp_warn               | **Optional.** The warning threshold.
snmp_crit               | **Optional.** The critical threshold.
snmp_process_name       | **Optional.** Name of the process (regexp). No trailing slash!. Defaults to ".*".
snmp_perf               | **Optional.** Enable perfdata values. Defaults to "true".
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.
