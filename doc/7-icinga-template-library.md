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

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name         | Description
-------------|---------------
cluster_zone | **Optional.** The zone name. Defaults to "$host.name$".

### <a id="itl-icinga-ido"></a> ido

Check command for the built-in `ido` check.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name         | Description
-------------|---------------
ido_type     | **Required.** The type of the IDO connection object. Can be either "IdoMysqlConnection" or "IdoPgsqlConnection".
ido_name     | **Required.** The name of the IDO connection object.

### <a id="itl-random"></a> random

Check command for the built-in `random` check. This check returns random states
and adds the check source to the check output.

For test and demo purposes only. The `random` check command does not support
any vars.

# <a id="plugin-check-commands"></a> Plugin Check Commands

The Plugin Check Commands provides example configuration for plugin check commands
provided by the Monitoring Plugins project.

By default the Plugin Check Commands are included in the `icinga2.conf` configuration
file:

    include <plugins>

The plugin check commands assume that there's a global constant named `PluginDir`
which contains the path of the plugins from the Monitoring Plugins project.

## <a id="plugin-check-command-apt"></a> apt

Check command for the `check_apt` plugin.

The `apt` check command does not support any vars.


## <a id="plugin-check-command-by-ssh"></a> by_ssh

Check command object for the `check_by_ssh` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
by_ssh_address  | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
by_ssh_port     | **Optional.** The SSH port. Defaults to 22.
by_ssh_command  | **Optional.** The command that should be executed.
by_ssh_logname  | **Optional.** The SSH username.
by_ssh_identity | **Optional.** The SSH identity.
by_ssh_quiet    | **Optional.** Whether to suppress SSH warnings. Defaults to false.
by_ssh_warn     | **Optional.** The warning threshold.
by_ssh_crit     | **Optional.** The critical threshold.
by_ssh_timeout  | **Optional.** The timeout in seconds.


## <a id="plugin-check-command-dhcp"></a> dhcp

Check command object for the `check_dhcp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
dhcp_serverip   | **Optional.** The IP address of the DHCP server which we should get a response from.
dhcp_requestedip| **Optional.** The IP address which we should be offered by a DHCP server.
dhcp_timeout    | **Optional.** The timeout in seconds.
dhcp_interface  | **Optional.** The interface to use.
dhcp_mac        | **Optional.** The MAC address to use in the DHCP request.
dhcp_unicast    | **Optional.** Whether to use unicast requests. Defaults to false.


## <a id="plugin-check-command-dig"></a> dig

Check command object for the `check_dig` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                 | Description
---------------------|--------------
dig_server           | **Optional.** The DNS server to query. Defaults to "127.0.0.1".
dig_lookup           | **Optional.** The address that should be looked up.


## <a id="plugin-check-command-disk"></a> disk

Check command object for the `check_disk` plugin.

> **Note**
>
> `disk_wfree` and `disk_cfree` require the percent sign compared to older versions.
> If omitted, disk units can be used. This has been changed in **2.3.0**.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            	| Description
------------------------|------------------------
disk_wfree      	| **Optional.** The free space warning threshold. Defaults to "20%". If the percent sign is omitted, units from `disk_units` are used.
disk_cfree      	| **Optional.** The free space critical threshold. Defaults to "10%". If the percent sign is omitted, units from `disk_units` are used.
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

## <a id="plugin-check-command-disk-smb"></a> disk_smb

Check command object for the `check_disk_smb` plugin.

> **Note**
>
> `disk_smb_wused` and `disk_smb_cused` require the percent sign. If omitted, disk units can be used.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            	| Description
------------------------|------------------------
disk_smb_hostname	| **Required.** NetBIOS name of the server.
disk_smb_share		| **Required.** Share name being queried.
disk_smb_workgroup	| **Optional.** Workgroup or Domain used (defaults to 'WORKGROUP' if omitted).
disk_smb_address	| **Optional.** IP address of the host (only necessary if host belongs to another network).
disk_smb_username	| **Optional.** Username for server log-in (defaults to 'guest' if omitted).
disk_smb_password	| **Optional.** Password for server log-in (defaults to an empty password if omitted).
disk_smb_wused      	| **Optional.** The used space warning threshold. Defaults to "85%". If the percent sign is omitted, use optional disk units.
disk_smb_cused      	| **Optional.** The used space critical threshold. Defaults to "95%". If the percent sign is omitted, use optional disk units.
disk_smb_port		| **Optional.** Connection port, e.g. `139` or `445`. Defaults to `smbclient` default if omitted.

## <a id="plugin-check-command-dns"></a> dns

Check command object for the `check_dns` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                 | Description
---------------------|--------------
dns_lookup           | **Optional.** The hostname or IP to query the DNS for. Defaults to "$host_name$".
dns_server           | **Optional.** The DNS server to query. Defaults to the server configured in the OS.
dns_expected_answer  | **Optional.** The answer to look for. A hostname must end with a dot. **Deprecated in 2.3.**
dns_expected_answers | **Optional.** The answer(s) to look for. A hostname must end with a dot. Multiple answers must be defined as array.
dns_authoritative    | **Optional.** Expect the server to send an authoritative answer.
dns_wtime            | **Optional.** Return warning if elapsed time exceeds value.
dns_ctime            | **Optional.** Return critical if elapsed time exceeds value.
dns_timeout          | **Optional.** Seconds before connection times out. Defaults to 10.


## <a id="plugin-check-command-dummy"></a> dummy

Check command object for the `check_dummy` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
dummy_text      | **Optional.** Plugin output. Defaults to "Check was successful.".


## <a id="plugin-check-command-fping4"></a> fping4

Check command object for the `check_fping` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

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


## <a id="plugin-check-command-fping6"></a> fping6

Check command object for the `check_fping` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

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


## <a id="plugin-check-command-ftp"></a> ftp

Check command object for the `check_ftp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name               | Description
-------------------|--------------
ftp_address        | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ftp_port           | **Optional.** The FTP port number.
ftp_expect         | **Optional.** String to expect in server response (may be repeated).
ftp_all            | **Optional.** All expect strings need to occur in server response. Defaults to false.
ftp_escape_send    | **Optional.** Enable usage of \n, \r, \t or \\\\ in send string.
ftp_send           | **Optional.** String to send to the server.
ftp_escape_quit    | **Optional.** Enable usage of \n, \r, \t or \\\\ in quit string.
ftp_quit           | **Optional.** String to send server to initiate a clean close of the connection.
ftp_refuse         | **Optional.** Accept TCP refusals with states ok, warn, crit. Defaults to crit.
ftp_mismatch       | **Optional.** Accept expected string mismatches with states ok, warn, crit. Defaults to warn.
ftp_jail           | **Optional.** Hide output from TCP socket.
ftp_maxbytes       | **Optional.** Close connection once more than this number of bytes are received.
ftp_delay          | **Optional.** Seconds to wait between sending string and polling for response.
ftp_certificate    | **Optional.** Minimum number of days a certificate has to be valid. 1st value is number of days for warning, 2nd is critical (if not specified: 0) - seperated by comma.
ftp_ssl            | **Optional.** Use SSL for the connection. Defaults to false.
ftp_wtime          | **Optional.** Response time to result in warning status (seconds).
ftp_ctime          | **Optional.** Response time to result in critical status (seconds).
ftp_timeout        | **Optional.** Seconds before connection times out. Defaults to 10.


## <a id="plugin-check-command-hostalive"></a> hostalive

Check command object for the `check_ping` plugin with host check default values. This variant
uses the host's `address` attribute if available and falls back to using the `address6` attribute
if the `address` attribute is not set.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 80.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 100.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


## <a id="plugin-check-command-hostalive4"></a> hostalive4

Check command object for the `check_ping` plugin with host check default values. This variant
uses the host's `address` attribute.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 80.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 100.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


## <a id="plugin-check-command-hostalive6"></a> hostalive6

Check command object for the `check_ping` plugin with host check default values. This variant
uses the host's `address6` attribute.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv6 address. Defaults to "$address6$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 80.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 100.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


## <a id="plugin-check-command-hpjd"></a> hpjd

Check command object for the `check_hpjd` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
hpjd_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
hpjd_port       | **Optional.** The host's SNMP port. Defaults to 161.
hpjd_community  | **Optional.** The SNMP community. Defaults  to "public".


## <a id="plugin-check-command-http"></a> http

Check command object for the `check_http` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                     | Description
-------------------------|--------------
http_address             | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
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
http_extendedperfdata    | **Optional.** Print additional perfdata. Defaults to false.
http_onredirect          | **Optional.** How to handle redirect pages. Possible values: "ok" (default), "warning", "critical", "follow", "sticky" (like follow but stick to address), "stickyport" (like sticky but also to port)
http_pagesize            | **Optional.** Minimum page size required:Maximum page size required.
http_timeout             | **Optional.** Seconds before connection times out.


## <a id="plugin-check-command-icmp"></a> icmp

Check command object for the `check_icmp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

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


## <a id="plugin-check-command-imap"></a> imap

Check command object for the `check_imap` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
imap_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
imap_port       | **Optional.** The port that should be checked. Defaults to 143.


## <a id="plugin-check-command-ldap"></a> ldap

Check command object for the `check_ldap` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ldap_address    | **Optional.** Host name, IP Address, or unix socket (must be an absolute path). Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ldap_port       | **Optional.** Port number. Defaults to 389.
ldap_attr	| **Optional.** LDAP attribute to search for (default: "(objectclass=*)"
ldap_base       | **Required.** LDAP base (eg. ou=myunit,o=myorg,c=at).
ldap_bind       | **Optional.** LDAP bind DN (if required).
ldap_pass       | **Optional.** LDAP password (if required).
ldap_starttls   | **Optional.** Use STARTSSL mechanism introduced in protocol version 3.
ldap_ssl        | **Optional.** Use LDAPS (LDAP v2 SSL method). This also sets the default port to 636.
ldap_v2         | **Optional.** Use LDAP protocol version 2 (enabled by default).
ldap_v3         | **Optional.** Use LDAP protocol version 3 (disabled by default)
ldap_warning	| **Optional.** Response time to result in warning status (seconds).
ldap_critical	| **Optional.** Response time to result in critical status (seconds).
ldap_timeout	| **Optional.** Seconds before connection times out (default: 10).
ldap_verbose	| **Optional.** Show details for command-line debugging (disabled by default)

## <a id="plugin-check-command-load"></a> load

Check command object for the `check_load` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
load_wload1     | **Optional.** The 1-minute warning threshold. Defaults to 5.
load_wload5     | **Optional.** The 5-minute warning threshold. Defaults to 4.
load_wload15    | **Optional.** The 15-minute warning threshold. Defaults to 3.
load_cload1     | **Optional.** The 1-minute critical threshold. Defaults to 10.
load_cload5     | **Optional.** The 5-minute critical threshold. Defaults to 6.
load_cload15    | **Optional.** The 15-minute critical threshold. Defaults to 4.
load_percpu     | **Optional.** Divide the load averages by the number of CPUs (when possible). Defaults to false.


## <a id="plugin-check-command-nrpe"></a> nrpe

Check command object for the `check_nrpe` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
nrpe_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
nrpe_port       | **Optional.** The NRPE port. Defaults to 5666.
nrpe_command    | **Optional.** The command that should be executed.
nrpe_no_ssl     | **Optional.** Whether to disable SSL or not. Defaults to `false`.
nrpe_timeout_unknown | **Optional.** Whether to set timeouts to unknown instead of critical state. Defaults to `false`.
nrpe_timeout    | **Optional.** The timeout in seconds.
nrpe_arguments	| **Optional.** Arguments that should be passed to the command. Multiple arguments must be defined as array.


## <a id="plugin-check-command-nscp"></a> nscp

Check command object for the `check_nt` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
nscp_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
nscp_port       | **Optional.** The NSClient++ port. Defaults to 12489.
nscp_password   | **Optional.** The NSClient++ password.
nscp_variable   | **Required.** The variable that should be checked.
nscp_params     | **Optional.** Parameters for the query. Multiple parameters must be defined as array.
nscp_warn       | **Optional.** The warning threshold.
nscp_crit       | **Optional.** The critical threshold.
nscp_timeout    | **Optional.** The query timeout in seconds.
nscp_showall    | **Optional.** Use with SERVICESTATE to see working services or PROCSTATE for running processes. Defaults to false.


## <a id="plugin-check-command-ntp-time"></a> ntp_time

Check command object for the `check_ntp_time` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ntp_address     | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ntp_port        | **Optional.** Port number (default: 123).
ntp_quit        | **Optional.** Returns UNKNOWN instead of CRITICAL if offset cannot be found.
ntp_warning     | **Optional.** Offset to result in warning status (seconds).
ntp_critical    | **Optional.** Offset to result in critical status (seconds).
ntp_timeoffset  | **Optional.** Expected offset of the ntp server relative to local server (seconds).
ntp_timeout     | **Optional.** Seconds before connection times out (default: 10).


## <a id="plugin-check-command-ntp-peer"></a> ntp_peer

Check command object for the `check_ntp_peer` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ntp_address     | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ntp_port        | **Optional.** The port to use. Default to 123.
ntp_warning     | **Optional.** Offset to result in warning status (seconds).
ntp_critical    | **Optional.** Offset to result in critical status (seconds).
ntp_wstratum    | **Optional.** Warning threshold for stratum of server's synchronization peer.
ntp_cstratum    | **Optional.** Critical threshold for stratum of server's synchronization peer.
ntp_wjitter     | **Optional.** Warning threshold for jitter.
ntp_cjitter     | **Optional.** Critical threshold for jitter.
ntp_wsource     | **Optional.** Warning threshold for number of usable time sources.
ntp_csource     | **Optional.** Critical threshold for number of usable time sources.
ntp_timeout     | **Optional.** Seconds before connection times out (default: 10).


## <a id="plugin-check-command-passive"></a> passive

Specialised check command object for passive checks executing the `check_dummy` plugin with appropriate default values.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 3.
dummy_text      | **Optional.** Plugin output. Defaults to "No Passive Check Result Received.".


## <a id="plugin-check-command-ping"></a> ping

Check command object for the `check_ping` plugin. This command uses the host's `address` attribute
if available and falls back to using the `address6` attribute if the `address` attribute is not set.


Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


## <a id="plugin-check-command-ping4"></a> ping4

Check command object for the `check_ping` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv4 address. Defaults to "$address$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

## <a id="plugin-check-command-ping6"></a> ping6

Check command object for the `check_ping` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ping_address    | **Optional.** The host's IPv6 address. Defaults to "$address6$".
ping_wrta       | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
ping_wpl        | **Optional.** The packet loss warning threshold in %. Defaults to 5.
ping_crta       | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
ping_cpl        | **Optional.** The packet loss critical threshold in %. Defaults to 15.
ping_packets    | **Optional.** The number of packets to send. Defaults to 5.
ping_timeout    | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).


## <a id="plugin-check-command-pop"></a> pop

Check command object for the `check_pop` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
pop_address     | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
pop_port        | **Optional.** The port that should be checked. Defaults to 110.


## <a id="plugin-check-command-processes"></a> procs

Check command object for the `check_procs` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                 | Description
---------------------|--------------
procs_warning        | **Optional.** The process count warning threshold. Defaults to 250.
procs_critical       | **Optional.** The process count critical threshold. Defaults to 400.
procs_metric         | **Optional.** Check thresholds against metric.
procs_timeout        | **Optional.** Seconds before plugin times out.
procs_traditional    | **Optional.** Filter own process the traditional way by PID instead of /proc/pid/exe. Defaults to false.
procs_state          | **Optional.** Only scan for processes that have one or more of the status flags you specify.
procs_ppid           | **Optional.** Only scan for children of the parent process ID indicated.
procs_vsz            | **Optional.** Only scan for processes with VSZ higher than indicated.
procs_rss            | **Optional.** Only scan for processes with RSS higher than indicated.
procs_pcpu           | **Optional.** Only scan for processes with PCPU higher than indicated.
procs_user           | **Optional.** Only scan for processes with user name or ID indicated.
procs_argument       | **Optional.** Only scan for processes with args that contain STRING.
procs_argument_regex | **Optional.** Only scan for processes with args that contain the regex STRING.
procs_command        | **Optional.** Only scan for exact matches of COMMAND (without path).
procs_nokthreads     | **Optional.** Only scan for non kernel threads. Defaults to false.


## <a id="plugin-check-command-simap"></a> simap

Check command object for the `check_simap` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
simap_address   | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
simap_port      | **Optional.** The host's port.


## <a id="plugin-check-command-smtp"></a> smtp

Check command object for the `check_smtp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                  | Description
----------------------|--------------
smtp_address          | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
smtp_port             | **Optional.** The port that should be checked. Defaults to 25.
smtp_mail_from        | **Optional.** Test a MAIL FROM command with the given email address.
smtp_expect           | **Optional.** String to expect in first line of server response (default: '220').
smtp_command          | **Optional.** SMTP command (may be used repeatedly).
smtp_response         | **Optional.** Expected response to command (may be used repeatedly).
smtp_helo_fqdn        | **Optional.** FQDN used for HELO
smtp_certificate_age  | **Optional.** Minimum number of days a certificate has to be valid.
smtp_starttls         | **Optional.** Use STARTTLS for the connection.
smtp_authtype         | **Optional.** SMTP AUTH type to check (default none, only LOGIN supported).
smtp_authuser         | **Optional.** SMTP AUTH username.
smtp_authpass         | **Optional.** SMTP AUTH password.
smtp_ignore_quit      | **Optional.** Ignore failure when sending QUIT command to server.
smtp_warning          | **Optional.** Response time to result in warning status (seconds).
smtp_critical         | **Optional.** Response time to result in critical status (seconds).
smtp_timeout          | **Optional.** Seconds before connection times out (default: 10).


## <a id="plugin-check-command-snmp"></a> snmp

Check command object for the `check_snmp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                | Description
--------------------|--------------
snmp_address        | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
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
snmp_version        | **Optional.** Version to use. E.g. 1, 2, 2c or 3.
snmp_miblist        | **Optional.** MIB's to use, comma separated. Defaults to "ALL".
snmp_rate_multiplier | **Optional.** Converts rate per second. For example, set to 60 to convert to per minute.
snmp_rate           | **Optional.** Boolean. Enable rate calculation.
snmp_getnext        | **Optional.** Boolean. Use SNMP GETNEXT. Defaults to false.
snmp_timeout        | **Optional.** The command timeout in seconds. Defaults to 10 seconds.

## <a id="plugin-check-command-snmpv3"></a> snmpv3

Check command object for the `check_snmp` plugin, using SNMPv3 authentication and encryption options.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name              | Description
------------------|--------------
snmpv3_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmpv3_user       | **Required.** The username to log in with.
snmpv3_auth_alg   | **Optional.** The authentication algorithm. Defaults to SHA.
snmpv3_seclevel   | **Optional.** The security level. Defaults to authPriv.
snmpv3_auth_key   | **Required,** The authentication key. Required if `snmpv3_seclevel` is set to `authPriv` otherwise optional.
snmpv3_priv_alg   | **Optional.** The encryption algorithm. Defaults to AES.
snmpv3_priv_key   | **Required.** The encryption key.
snmpv3_oid        | **Required.** The SNMP OID.
snmpv3_warn       | **Optional.** The warning threshold.
snmpv3_crit       | **Optional.** The critical threshold.
snmpv3_label      | **Optional.** Prefix label for output value.

## <a id="plugin-check-command-snmp-uptime"></a> snmp-uptime

Check command object for the `check_snmp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmp_oid        | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
snmp_community  | **Optional.** The SNMP community. Defaults to "public".


## <a id="plugin-check-command-spop"></a> spop

Check command object for the `check_spop` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
spop_address    | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
spop_port       | **Optional.** The host's port.


## <a id="plugin-check-command-ssh"></a> ssh

Check command object for the `check_ssh` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ssh_address     | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ssh_port        | **Optional.** The port that should be checked. Defaults to 22.
ssh_timeout     | **Optional.** Seconds before connection times out. Defaults to 10.


## <a id="plugin-check-command-ssl"></a> ssl

Check command object for the `check_tcp` plugin, using ssl-related options.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                          | Description
------------------------------|--------------
ssl_address                   | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ssl_port                      | **Required.** The port that should be checked.
ssl_timeout                   | **Optional.** Timeout in seconds for the connect and handshake. The plugin default is 10 seconds.
ssl_cert_valid_days_warn      | **Optional.** Warning threshold for days before the certificate will expire. When used, ssl_cert_valid_days_critical must also be set.
ssl_cert_valid_days_critical  | **Optional.** Critical threshold for days before the certificate will expire. When used, ssl_cert_valid_days_warn must also be set.


## <a id="plugin-check-command-ssmtp"></a> ssmtp

Check command object for the `check_ssmtp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ssmtp_address   | **Required.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ssmtp_port      | **Optional.** The port that should be checked. Defaults to 465.


## <a id="plugin-check-command-swap"></a> swap

Check command object for the `check_swap` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
swap_wfree      | **Optional.** The free swap space warning threshold in %. Defaults to 50.
swap_cfree      | **Optional.** The free swap space critical threshold in %. Defaults to 25.


## <a id="plugin-check-command-tcp"></a> tcp

Check command object for the `check_tcp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
tcp_address     | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
tcp_port        | **Required.** The port that should be checked.
tcp_expect      | **Optional.** String to expect in server response (may be repeated).
tcp_all         | **Optional.** All expect strings need to occur in server response. Defaults to false.
tcp_escape_send | **Optional.** Enable usage of \n, \r, \t or \\\\ in send string.
tcp_send        | **Optional.** String to send to the server.
tcp_escape_quit | **Optional.** Enable usage of \n, \r, \t or \\\\ in quit string.
tcp_quit        | **Optional.** String to send server to initiate a clean close of the connection.
tcp_refuse      | **Optional.** Accept TCP refusals with states ok, warn, crit. Defaults to crit.
tcp_mismatch    | **Optional.** Accept expected string mismatches with states ok, warn, crit. Defaults to warn.
tcp_jail        | **Optional.** Hide output from TCP socket.
tcp_maxbytes    | **Optional.** Close connection once more than this number of bytes are received.
tcp_delay       | **Optional.** Seconds to wait between sending string and polling for response.
tcp_certificate | **Optional.** Minimum number of days a certificate has to be valid. 1st value is number of days for warning, 2nd is critical (if not specified: 0) - seperated by comma.
tcp_ssl         | **Optional.** Use SSL for the connection. Defaults to false.
tcp_wtime       | **Optional.** Response time to result in warning status (seconds).
tcp_ctime       | **Optional.** Response time to result in critical status (seconds).
tcp_timeout     | **Optional.** Seconds before connection times out. Defaults to 10.


## <a id="plugin-check-command-udp"></a> udp

Check command object for the `check_udp` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
udp_address     | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
udp_port        | **Required.** The port that should be checked.
udp_send        | **Required.** The payload to send in the UDP datagram.
udp_expect      | **Required.** The payload to expect in the response datagram.
udp_quit        | **Optional.** The payload to send to 'close' the session.


## <a id="plugin-check-command-ups"></a> ups

Check command object for the `check_ups` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
ups_address     | **Required.** The address of the host running upsd. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
ups_name        | **Required.** The UPS name. Defaults to `ups`.
ups_port        | **Optional.** The port to which to connect. Defaults to 3493.
ups_variable    | **Optional.** The variable to monitor. Must be one of LINE, TEMP, BATTPCT or LOADPCT. If this is not set, the check only relies on the value of `ups.status`.
ups_warning     | **Optional.** The warning threshold for the selected variable.
ups_critical    | **Optional.** The critical threshold for the selected variable.
ups_celsius     | **Optional.** Display the temperature in degrees Celsius instead of Fahrenheit. Defaults to `false`.
ups_timeout     | **Optional.** The number of seconds before the connection times out. Defaults to 10.


## <a id="plugin-check-command-users"></a> users

Check command object for the `check_users` plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
users_wgreater  | **Optional.** The user count warning threshold. Defaults to 20.
users_cgreater  | **Optional.** The user count critical threshold. Defaults to 50.


# <a id="windows-plugins"></a>Icinga 2 Windows plugins

To allow a basic monitoring of Windows clients Icinga 2 comes with a set of Windows only plugins. While trying to mirror the functionalities of their linux cousins from the monitoring-plugins package, the differences between Windows and Linux are too big to be able use the same CheckCommands for both systems.

A check-commands-windows.conf comes with Icinga 2, it asumes that the Windows Plugins are installed in the PluginDir set in your constants.conf. To enable them the following include directive is needed in you icinga2.conf:

	include <windows-plugins>

One of the differences between the Windows plugins and their linux counterparts is that they consistently do not require thresholds to run, functioning like dummies without.


## <a id="windows-plugins-thresholds"></a>Threshold syntax

So not specified differently the thresholds for the plugins all follow the same pattern

Threshold    | Meaning
:------------|:----------
"29"         | The threshold is 29.
"!29"        | The threshold is 29, but the negative of the result is returned.
"[10-40]"    | The threshold is a range from (including) 20 to 40, a value inside means the threshold has been exceeded.
"![10-40]"   | Same as above, but the result is inverted.


## <a id="windows-plugins-disk-windows"></a>disk-windows

Check command object for the `check_disk.exe` plugin.
Aggregates the free disk space of all volumes and mount points it can find, or the ones defined in `disk_win_path`. Ignores removable storage like fash drives and discs (CD, DVD etc.).

Custom attributes:

Name            | Description
:---------------|:------------
disk\_win\_warn | **Optional**. The warning threshold.
disk\_win\_crit | **Optional**. The critical threshold.
disk\_win\_path | **Optional**. Check only these paths, default checks all.
disk\_win\_unit | **Optional**. Use this unit to display disk space, thresholds are interpreted in this unit. Defaults to "mb", possible values are: b, kb, mb, gb and tb.


## <a id="windows-plugins-load-windows"></a>load-windows

Check command object for the `check_load.exe` plugin.
This plugin collects the inverse of the performance counter `\Processor(_Total)\% Idle Time` two times, with a wait time of one second between the collection. To change this wait time use [`perfmon-windows`](7-icinga-template-library.md#windows-plugins-load-windows).

Custom attributes:

Name            | Description
:---------------|:------------
load\_win\_warn | **Optional**. The warning threshold.
load\_win\_crit | **Optional**. The critical threshold.


## <a id="windows-plugins-memory-windows"></a>memory-windows

Check command object for the `check_memory.exe` plugin.
The memory collection is instant.

Custom attributes:

Name              | Description
:-----------------|:------------
memory\_win\_warn | **Optional**. The warning threshold.
memory\_win\_crit | **Optional**. The critical threshold.
memory\_win\_unit | **Optional**. The unit to display the received value in, thresholds are interpreted in this unit. Defaults to "mb" (megabye), possible values are: b, kb, mb, gb and tb.


## <a id="windows-plugins-network-windows"></a>network-windows

Check command object for the `check_network.exe` plugin.
Collects the total Bytes inbount and outbound for all interfaces in one second, to itemise interfaces or use a different collection interval use [`perfmon-windows`](7-icinga-template-library.md#windows-plugins-load-windows).

Custom attributes:

Name               | Description
:------------------|:------------
network\_win\_warn | **Optional**. The warning threshold.
network\_win\_crit | **Optional**. The critical threshold.


## <a id="windows-plugins-permon-windows"></a>perfmon-windows

Check command object for the `check_perfmon.exe` plugin.
This plugins allows to collect data from a Performance Counter. After the first data collection a second one is done after `perfmon_win_wait` milliseconds. When you know `perfmon_win_counter` only requires one set of data to provide valid data you can set `perfmon_win_wait` to `0`.

To recieve a list of possible Performance Counter Objects run `check_perfmon.exe --print-objects` and to view an objects instances and counters run `check_perfmon.exe --print-object-info -P "name of object"`

Custom attributes:

Name                  | Description
:---------------------|:------------
perfmon\_win\_warn    | **Optional**. The warning threshold.
perfmon\_win\_crit    | **Optional**. The critical threshold.
perfmon\_win\_counter | **Required**. The Performance Counter to use. Ex. `\Processor(_Total)\% Idle Time`.
perfmon\_win\_wait    | **Optional**. Time in milliseconds to wait between data collection (default: 1000).
perfmon\_win\_type    | **Optional**. Format in which to expect perfomance values. Possible are: long, int64 and double (default).


## <a id="windows-plugins-ping-windows"></a>ping-windows

Check command object for the `check_ping.exe` plugin.
ping-windows should automaticly detect whether `ping_win_address` is an IPv4 or IPv6 address, if not use ping4-windows and ping6-windows. Also note that check\_ping.exe waits at least `ping_win_timeout` milliseconds between the pings.

Custom attributes:

Name               | Description
:------------------|:------------
ping\_win\_warn    | **Optional**. The warning threshold. RTA and package loss seperated by comma.
ping\_win\_crit    | **Optional**. The critical threshold. RTA and package loss seperated by comma.
ping\_win\_address | **Required**. An IPv4 or IPv6 address
ping\_win\_packets | **Optional**. Number of packages to send. Default: 5.
ping\_win\_timeout | **Optional**. The timeout in milliseconds. Default: 1000


## <a id="windows-plugins-procs-windows"></a>procs-windows

Check command object for `check_procs.exe` plugin.
When useing `procs_win_user` this plugins needs adminstratice privileges to access the processes of other users, to just enumerate them no additional privileges are required.

Custom attributes:

Name             | Description
:----------------|:------------
procs\_win\_warn | **Optional**. The warning threshold.
procs\_win\_crit | **Optional**. The critical threshold.
procs\_win\_user | **Optional**. Count this useres processes.


## <a id="windows-plugins-service-windows"></a>service-windows

Check command object for `check_service.exe` plugin.
This checks thresholds work different since the binary decision whether a service is running or not does not allow for three states. As a default `check_service.exe` will return CRITICAL when `service_win_service` is not running, the `service_win_warn` flag changes this to WARNING.

Custom attributes:

Name                  | Description
:---------------------|:------------
service\_win\_warn    | **Optional**. Warn when service is not running.
service\_win\_service | **Required**. The critical threshold.


## <a id="windows-plugins-swap-windows"></a>swap-windows

Check command object for `check_swap.exe` plugin.
The data collection is instant.

Custom attributes:

Name            | Description
:---------------|:------------
swap\_win\_warn | **Optional**. The warning threshold.
swap\_win\_crit | **Optional**. The critical threshold.
swap\_win\_unit | **Optional**. The unit to display the received value in, thresholds are interpreted in this unit. Defaults to "mb" (megabyte).


## <a id="windows-plugins-update-windows"></a>update-windows

Check command object for `check_update.exe` plugin.
Querying Microsoft for Windows updates can take multiple seconds to minutes. An update is treated as important when it has the WSUS flag for SecurityUpdates or CriticalUpdates.

Custom attributes:

Name                | Description
:-------------------|:------------
update\_win\_warn   | If set returns warning when important updates are available
update\_win\_crit   | If set return critical when important updates that require a reboot are available.
update\_win\_reboot | Set to treat 'may need update' as 'definitely needs update'


## <a id="windows-plugins-uptime-windows"></a>uptime-windows

Check command opject for `check_uptime.exe` plugin.
Uses GetTickCount64 to get the uptime, so boot time is not included.

Custom attributes:

Name              | Description
:-----------------|:------------
uptime\_win\_warn | **Optional**. The warning threshold.
uptime\_win\_crit | **Optional**. The critical threshold.
uptime\_win\_unit | **Optional**. The unit to display the received value in, thresholds are interpreted in this unit. Defaults to "s"(seconds), possible values are ms (milliseconds), s, m (minutes), h (hours).


## <a id="windows-plugins-users-windows"></a>users-windows

Check command object for `check_users.exe` plugin.

Custom attributes:

Name             | Description
:----------------|:------------
users\_win\_warn | **Optional**. The warning threshold.
users\_win\_crit | **Optional**. The critical threshold.


# <a id="nscp-plugin-check-commands"></a> NSClient++ Check Commands

Icinga 2 can use the `nscp client` command to run arbitrary NSClient++ checks.

You can enable these check commands by adding the following the include directive in your
[icinga2.conf](4-configuring-icinga-2.md#icinga2-conf) configuration file:

    include <nscp>

You can also optionally specify an alternative installation directory for NSClient++ by adding
the NscpPath constant in your [constants.conf](4-configuring-icinga-2.md#constants-conf) configuration
file:

    const NscpPath = "C:\\Program Files (x86)\\NSClient++"

By default Icinga 2 uses the Microsoft Installer API to determine where NSClient++ is installed. It should
not be necessary to manually set this constant.

Note that it is not necessary to run NSClient++ as a Windows service for these commands to work.

## <a id="nscp-check-local"></a> nscp-local

Check command object for NSClient++

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name            | Description
----------------|--------------
nscp_log_level  | **Optional.** The log level. Defaults to "critical".
nscp_load_all   | **Optional.** Whether to load all modules. Defaults to true.
nscp_boot       | **Optional.** Whether to use the --boot option. Defaults to true.
nscp_query      | **Required.** The NSClient++ query. Try `nscp client -q x` for a list.
nscp_arguments  | **Optional.** An array of query arguments.

## <a id="nscp-check-local-cpu"></a> nscp-local-cpu

Check command object for the `check_cpu` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-memory"></a> nscp-local-memory

Check command object for the `check_memory` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-os-version"></a> nscp-local-os-version

Check command object for the `check_os_version` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-pagefile"></a> nscp-local-pagefile

Check command object for the `check_pagefile` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-process"></a> nscp-local-process

Check command object for the `check_process` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-service"></a> nscp-local-service

Check command object for the `check_service` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-uptime"></a> nscp-local-uptime

Check command object for the `check_uptime` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-version"></a> nscp-local-version

Check command object for the `check_version` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

## <a id="nscp-check-local-disk"></a> nscp-local-disk

Check command object for the `check_drivesize` NSClient++ plugin.

This command has the same custom attributes like the `nscp-local` check command.

# <a id="snmp-manubulon-plugin-check-commands"></a> SNMP Manubulon Plugin Check Commands

The `SNMP Manubulon Plugin Check Commands` provide example configuration for plugin check
commands provided by the [SNMP Manubulon project](http://nagios.manubulon.com/index_snmp.html).

The SNMP manubulon plugin check commands assume that the global constant named `ManubulonPluginDir`
is set to the path where the Manubublon SNMP plugins are installed.

You can enable these plugin check commands by adding the following the include directive in your
[icinga2.conf](4-configuring-icinga-2.md#icinga2-conf) configuration file:

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


## <a id="plugin-check-command-snmp-load"></a> snmp-load

Check command object for the [check_snmp_load.pl](http://nagios.manubulon.com/snmp_load.html) plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):


Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to false.
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to false.
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to false.
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to false.
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn               | **Optional.** The warning threshold. Change the `snmp_load_type` var to "netsl" for using 3 values.
snmp_crit               | **Optional.** The critical threshold. Change the `snmp_load_type` var to "netsl" for using 3 values.
snmp_load_type          | **Optional.** Load type. Defaults to "stand". Check all available types in the [snmp load](http://nagios.manubulon.com/snmp_load.html) documentation.
snmp_perf               | **Optional.** Enable perfdata values. Defaults to true.
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

## <a id="plugin-check-command-snmp-memory"></a> snmp-memory

Check command object for the [check_snmp_mem.pl](http://nagios.manubulon.com/snmp_mem.html) plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to false.
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to false.
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to false.
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to false.
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn               | **Optional.** The warning threshold.
snmp_crit               | **Optional.** The critical threshold.
snmp_is_cisco		| **Optional.** Change OIDs for Cisco switches. Defaults to false.
snmp_perf               | **Optional.** Enable perfdata values. Defaults to true.
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

## <a id="plugin-check-command-snmp-storage"></a> snmp-storage

Check command object for the [check_snmp_storage.pl](http://nagios.manubulon.com/snmp_storage.html) plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to false.
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to false.
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to false.
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to false.
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn               | **Optional.** The warning threshold.
snmp_crit               | **Optional.** The critical threshold.
snmp_storage_name       | **Optional.** Storage name. Default to regex "^/$$". More options available in the [snmp storage](http://nagios.manubulon.com/snmp_storage.html) documentation.
snmp_perf               | **Optional.** Enable perfdata values. Defaults to true.
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

## <a id="plugin-check-command-snmp-interface"></a> snmp-interface

Check command object for the [check_snmp_int.pl](http://nagios.manubulon.com/snmp_int.html) plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                        | Description
----------------------------|--------------
snmp_address                | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmp_nocrypt                | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to false.
snmp_community              | **Optional.** The SNMP community. Defaults to "public".
snmp_port                   | **Optional.** The SNMP port connection.
snmp_v2                     | **Optional.** SNMP version to 2c. Defaults to false.
snmp_v3                     | **Optional.** SNMP version to 3. Defaults to false.
snmp_login                  | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password               | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass        | **Optional.** Define to use SNMP version 3 priv password. Defaults to false.
snmp_authprotocol           | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass               | **Required.** SNMP version 3 priv password. No value defined as default.
snmp_warn                   | **Optional.** The warning threshold.
snmp_crit                   | **Optional.** The critical threshold.
snmp_interface              | **Optional.** Network interface name. Default to regex "eth0".
snmp_interface_perf         | **Optional.** Check the input/ouput bandwidth of the interface. Defaults to true.
snmp_interface_label        | **Optional.** Add label before speed in output: in=, out=, errors-out=, etc...
snmp_interface_bits_bytes   | **Optional.** Output performance data in bits/s or Bytes/s. **Depends** on snmp_interface_kbits set to true. Defaults to true.
snmp_interface_percent      | **Optional.** Output performance data in % of max speed. Defaults to false.
snmp_interface_kbits        | **Optional.** Make the warning and critical levels in KBits/s. Defaults to true.
snmp_interface_megabytes    | **Optional.** Make the warning and critical levels in Mbps or MBps. **Depends** on snmp_interface_kbits set to true. Defaults to true.
snmp_interface_64bit        | **Optional.** Use 64 bits counters instead of the standard counters when checking bandwidth & performance data for interface >= 1Gbps. Defaults to false.
snmp_interface_errors       | **Optional.** Add error & discard to Perfparse output. Defaults to true.
snmp_interface_noregexp     | **Optional.** Do not use regexp to match interface name in description OID. Defaults to false.
snmp_interface_delta        | **Optional.** Delta time of perfcheck. Defaults to "300" (5 min).
snmp_warncrit_percent       | **Optional.** Make the warning and critical levels in % of reported interface speed. If set **snmp_interface_megabytes** needs to be set to false. Defaults to false.
snmp_perf                   | **Optional.** Enable perfdata values. Defaults to true.
snmp_timeout                | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

## <a id="plugin-check-command-snmp-process"></a> snmp-process

Check command object for the [check_snmp_process.pl](http://nagios.manubulon.com/snmp_process.html) plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
snmp_address            | **Optional.** The host's address. Defaults to "$address$" if the host's `address` attribute is set, "$address6$" otherwise.
snmp_nocrypt            | **Optional.** Define SNMP encryption. If set **snmp_v3** needs to be set. Defaults to false.
snmp_community          | **Optional.** The SNMP community. Defaults to "public".
snmp_port               | **Optional.** The SNMP port connection.
snmp_v2                 | **Optional.** SNMP version to 2c. Defaults to false.
snmp_v3                 | **Optional.** SNMP version to 3. Defaults to false.
snmp_login              | **Optional.** SNMP version 3 username. Defaults to "snmpuser".
snmp_password           | **Required.** SNMP version 3 password. No value defined as default.
snmp_v3_use_privpass    | **Optional.** Define to use SNMP version 3 priv password. Defaults to false.
snmp_authprotocol       | **Optional.** SNMP version 3 authentication protocol. Defaults to "md5,des".
snmp_privpass           | **Required.** SNMP version 3 priv password. No value defined as default..
snmp_warn               | **Optional.** The warning threshold.
snmp_crit               | **Optional.** The critical threshold.
snmp_process_name       | **Optional.** Name of the process (regexp). No trailing slash!. Defaults to ".*".
snmp_perf               | **Optional.** Enable perfdata values. Defaults to true.
snmp_timeout            | **Optional.** The command timeout in seconds. Defaults to 5 seconds.

# <a id="plugins-contrib"></a> Plugins Contrib

The Plugins contrib collects various contributed command definitions.

These check commands assume that the global constant named `PluginContribDir`
is set to the path where the user installs custom plugins and can be enabled by uncommenting the corresponding line in icinga2.conf.

## <a id="plugins-contrib-databases"></a> Databases

All database plugins go in this category.

### <a id="plugins-contrib-command-mssql_health"></a> mssql_health

The plugin `mssql_health` utilises Perl DBD::Sybase based on FreeTDS to connect to MSSQL databases for monitoring.
For release tarballs, detailed documentation especially on the different modes and scripts for creating a monitoring user see [https://labs.consol.de](https://labs.consol.de/nagios/check_mssql_health/). For development check [https://github.com](https://github.com/lausser/check_mssql_health).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                             | Description
---------------------------------|------------------------------------------------------------------------------------------------------------------------------
mssql_health_hostname            | **Optional.** Specifies the database hostname or address. No default because you typically use "mssql_health_server".
mssql_health_port                | **Optional.** Specifies the database port. No default because you typically use "mssql_health_server".
mssql_health_server              | **Optional.** The name of a predefined connection (in freetds.conf).
mssql_health_currentdb           | **Optional.** The name of a database which is used as the current database for the connection.
mssql_health_username            | **Optional.** The username for the database connection.
mssql_health_password            | **Optional.** The password for the database connection.
mssql_health_warning             | **Optional.** The warning threshold depending on the mode.
mssql_health_critical            | **Optional.** The critical threshold depending on the mode.
mssql_health_mode                | **Required.** The mode uses predefined keywords for the different checks. For example "connection-time", "database-free" or "sql".
mssql_health_name                | **Optional.** Depending on the mode this could be the database name or a SQL statement.
mssql_health_name2               | **Optional.** If "mssql_health_name" is a sql statement, "mssql_health_name2" can be used to appear in the output and the performance data.
mssql_health_regexp              | **Optional.** If set to true, "mssql_health_name" will be interpreted as a regular expression. Defaults to false.
mssql_health_units               | **Optional.** This is used for a better output of mode=sql and for specifying thresholds for mode=tablespace-free. Possible values are "%", "KB", "MB" and "GB".
mssql_health_offlineok           | **Optional.** Set this to true, if offline databases are perfectly ok for you. Defaults to false.
mssql_health_commit              | **Optional.** Set this to true to turn on autocommit for the dbd::sybase module. Defaults to false.

### <a id="plugins-contrib-command-mysql_health"></a> mysql_health

The plugin `mysql_health` utilises Perl DBD::MySQL to connect to MySQL databases for monitoring.
For release tarballs and detailed documentation especially on the different modes and required permissions see [https://labs.consol.de](https://labs.consol.de/nagios/check_mysql_health/). For development check [https://github.com](https://github.com/lausser/check_mysql_health).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                             | Description
---------------------------------|------------------------------------------------------------------------------------------------------------------------------
mysql_health_hostname            | **Required.** Specifies the database hostname or address. Defaults to "$address$" or "$address6$" if the `address` attribute is not set.
mysql_health_port                | **Optional.** Specifies the database port. Defaults to 3306 (or 1186 for "mysql_health_mode" cluster).
mysql_health_socket              | **Optional.** Specifies the database unix socket. No default.
mysql_health_username            | **Optional.** The username for the database connection.
mysql_health_password            | **Optional.** The password for the database connection.
mysql_health_database            | **Optional.** The database to connect to. Defaults to information_schema.
mysql_health_warning             | **Optional.** The warning threshold depending on the mode.
mysql_health_critical            | **Optional.** The critical threshold depending on the mode.
mysql_health_mode                | **Required.** The mode uses predefined keywords for the different checks. For example "connection-time", "slave-lag" or "sql".
mysql_health_name                | **Optional.** The SQL statement to be executed with "mysql_health_mode" sql.
mysql_health_name2               | **Optional.** If "mysql_health_name" is a sql statement, "mysql_health_name2" can be used to appear in the output and the performance data.
mysql_health_units               | **Optional.** This is used for a better output of mode=sql and for specifying thresholds for mode=tablespace-free. Possible values are "%", "KB", "MB" and "GB".
mysql_health_labelformat         | **Optional.** One of those formats pnp4nagios or groundwork. Defaults to pnp4nagios.

### <a id="plugins-contrib-command-oracle_health"></a> oracle_health

The plugin `oracle_health` utilises Perl DBD::Oracle based on oracle-instantclient-sdk or sqlplus to connect to Oracle databases for monitoring.
For release tarballs and detailed documentation especially on the different modes and required permissions see [https://labs.consol.de](https://labs.consol.de/nagios/check_oracle_health/). For development check [https://github.com](https://github.com/lausser/check_oracle_health).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                             | Description
---------------------------------|------------------------------------------------------------------------------------------------------------------------------
oracle_health_connect            | **Required.** Specifies the database connection string (from tnsnames.ora).
oracle_health_username           | **Optional.** The username for the database connection.
oracle_health_password           | **Optional.** The password for the database connection.
oracle_health_warning            | **Optional.** The warning threshold depending on the mode.
oracle_health_critical           | **Optional.** The critical threshold depending on the mode.
oracle_health_mode               | **Required.** The mode uses predefined keywords for the different checks. For example "connection-time", "flash-recovery-area-usage" or "sql".
oracle_health_name               | **Optional.** The tablespace, datafile, wait event, latch, enqueue depending on the mode or SQL statement to be executed with "oracle_health_mode" sql.
oracle_health_name2              | **Optional.** If "oracle_health_name" is a sql statement, "oracle_health_name2" can be used to appear in the output and the performance data.
oracle_health_regexp             | **Optional.** If set to true, "oracle_health_name" will be interpreted as a regular expression. Defaults to false.
oracle_health_units              | **Optional.** This is used for a better output of mode=sql and for specifying thresholds for mode=tablespace-free. Possible values are "%", "KB", "MB" and "GB".
oracle_health_ident              | **Optional.** If set to true outputs instance and database names. Defaults to false.
oracle_health_commit             | **Optional.** Set this to true to turn on autocommit for the dbd::oracle module. Defaults to false.
oracle_health_noperfdata         | **Optional.** Set this to true if you want to disable perfdata. Defaults to false.

Environment Macros:

Name                | Description
--------------------|------------------------------------------------------------------------------------------------------------------------------------------
ORACLE_HOME         | **Required.** Specifies the location of the oracle instant client libraries. Defaults to "/usr/lib/oracle/11.2/client64/lib". Can be overridden by setting "oracle_home".
TNS_ADMIN           | **Required.** Specifies the location of the tnsnames.ora including the database connection strings. Defaults to "/etc/icinga2/plugin-configs". Can be overridden by setting "oracle_tns_admin".

### <a id="plugins-contrib-command-postgres"></a> postgres

The plugin `postgres` utilises the psql binary to connect to PostgreSQL databases for monitoring.
For release tarballs and detailed documentation especially the different actions and required persmissions see [https://bucardo.org/wiki/Check_postgres](https://bucardo.org/wiki/Check_postgres). For development check [https://github.com](https://github.com/bucardo/check_postgres).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                             | Description
---------------------------------|------------------------------------------------------------------------------------------------------------------------------
postgres_host        | **Optional.** Specifies the database hostname or address. Defaults to "$address$" or "$address6$" if the `address` attribute is not set. If "postgres_unixsocket" is set to true falls back to unix socket.
postgres_port        | **Optional.** Specifies the database port. Defaults to 5432.
postgres_dbname      | **Optional.** Specifies the database name to connect to. Defaults to "postgres" or "template1".
postgres_dbuser      | **Optional.** The username for the database connection. Defaults to "postgres".
postgres_dbpass      | **Optional.** The password for the database connection. You can use a .pgpass file instead.
postgres_dbservice   | **Optional.** Specifies the service name to use inside of pg_service.conf.
postgres_warning     | **Optional.** Specifies the warning threshold, range depends on the action.
postgres_critical    | **Optional.** Specifies the critical threshold, range depends on the action.
postgres_include     | **Optional.** Specifies name(s) items to specifically include (e.g. tables), depends on the action.
postgres_exclude     | **Optional.** Specifies name(s) items to specifically exclude (e.g. tables), depends on the action.
postgres_includeuser | **Optional.** Include objects owned by certain users.
postgres_excludeuser | **Optional.** Exclude objects owned by certain users.
postgres_standby     | **Optional.** Assume that the server is in continious WAL recovery mode if set to true. Defaults to false.
postgres_production  | **Optional.** Assume that the server is in production mode if set to true. Defaults to false.
postgres_action      | **Required.** Determines the test executed.
postgres_unixsocket  | **Optional.** If "postgres_unixsocket" is set to true the unix socket is used instead of an address. Defaults to false.

### <a id="plugins-contrib-command-mongodb"></a> mongodb

The plugin `mongodb` utilises Python PyMongo.
For development check [https://github.com](https://github.com/mzupan/nagios-plugin-mongodb).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                             | Description
---------------------------------|------------------------------------------------------------------------------------------------------------------------------
mongodb_host                     | **Required.** Specifies the hostname or address.
mongodb_port                     | **Required.** The port mongodb is runnung on.
mongodb_user                     | **Optional.** The username you want to login as
mongodb_passwd                   | **Optional.** The password you want to use for that user
mongodb_warning                  | **Optional.** The warning threshold we want to set
mongodb_critical                 | **Optional.** The critical threshold we want to set
mongodb_action                   | **Required.** The action you want to take
mongodb_maxlag                   | **Optional.** Get max replication lag (for replication_lag action only)
mongodb_mappedmemory             | **Optional.** Get mapped memory instead of resident (if resident memory can not be read)
mongodb_perfdata                 | **Optional.** Enable output of Nagios performance data
mongodb_database                 | **Optional.** Specify the database to check
mongodb_alldatabases             | **Optional.** Check all databases (action database_size)
mongodb_ssl                      | **Optional.** Connect using SSL
mongodb_replicaset               | **Optional.** Connect to replicaset
mongodb_querytype                | **Optional.** The query type to check [query|insert|update|delete|getmore|command] from queries_per_second
mongodb_collection               | **Optional.** Specify the collection to check
mongodb_sampletime               | **Optional.** Time used to sample number of pages faults

### <a id="plugins-contrib-command-elasticsearch"></a> elasticsearch

An [ElasticSearch](https://www.elastic.co/products/elasticsearch) availability
and performance monitoring plugin available for download at [GitHub](https://github.com/anchor/nagios-plugin-elasticsearch).
The plugin requires the HTTP API enabled on your ElasticSearch node.

Name                         | Description
-----------------------------|-------------------------------------------------------------------------------------------------------
elasticsearch_failuredomain  | **Optional.** A comma-separated list of ElasticSearch attributes that make up your cluster's failure domain.
elasticsearch_host           | **Optional.** Hostname or network address to probe. Defaults to 'localhost'.
elasticsearch_masternodes    | **Optional.** Issue a warning if the number of master-eligible nodes in the cluster drops below this number. By default, do not monitor the number of nodes in the cluster.
elasticsearch_port           | **Optional.** TCP port to probe.  The ElasticSearch API should be listening here. Defaults to 9200.
elasticsearch_prefix         | **Optional.** Optional prefix (e.g. 'es') for the ElasticSearch API. Defaults to ''.
elasticsearch_yellowcritical | **Optional.** Instead of issuing a 'warning' for a yellow cluster state, issue a 'critical' alert. Defaults to false.


## <a id="plugins-contrib-ipmi"></a> IPMI Devices

This category includes all plugins for IPMI devices.

### <a id="plugins-contrib-command-ipmi-sensor"></a> ipmi-sensor

With the plugin `ipmi-sensor` provided by <a href="https://www.thomas-krenn.com/">Thomas-Krenn.AG</a> you can monitor sensor data for IPMI devices. See https://www.thomas-krenn.com/en/wiki/IPMI_Sensor_Monitoring_Plugin for installation and configuration instructions.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                             | Description
---------------------------------|-----------------------------------------------------------------------------------------------------
ipmi_address                     | **Required.** Specifies the remote host (IPMI device) to check. Defaults to "$address$".
ipmi_config_file                 | **Optional.** Path to the FreeIPMI configuration file. It should contain IPMI username, IPMI password, and IPMI privilege-level.
ipmi_username                    | **Optional.** The IPMI username.
ipmi_password                    | **Optional.** The IPMI password.
ipmi_privilege_level             | **Optional.** The IPMI privilege level of the IPMI user.
ipmi_backward_compatibility_mode | **Optional.** Enable backward compatibility mode, useful for FreeIPMI 0.5.* (this omits FreeIPMI options "--quiet-cache" and "--sdr-cache-recreate").
ipmi_sensor_type                 | **Optional.** Limit sensors to query based on IPMI sensor type. Examples for IPMI sensor types are 'Fan', 'Temperature' and 'Voltage'.
ipmi_exclude_sensor_id           | **Optional.** Exclude sensor matching ipmi_sensor_id.
ipmi_sensor_id                   | **Optional.** Include sensor matching ipmi_sensor_id.
ipmi_protocal_lan_version        | **Optional.** Change the protocol LAN version. Defaults to "LAN_2_0".
ipmi_number_of_active_fans       | **Optional.** Number of fans that should be active. Otherwise a WARNING state is returned.
ipmi_show_fru                    | **Optional.** Print the product serial number if it is available in the IPMI FRU data.
ipmi_no_sel_checking             | **Optional.** Turn off system event log checking via ipmi-sel.

## <a id="plugins-contrib-network-components"></a> Network Components

This category includes all plugins for various network components like routers, switches and firewalls.

### <a id="plugins-contrib-command-interfacetable"></a> interfacetable

The plugin `interfacetable` generates a html page containing information about the monitored node and all of its interfaces. The actively developed and maintained version is `interfacetable_v3t` provided by `Yannick Charton` on [http://www.tontonitch.com](http://www.tontonitch.com/tiki/tiki-index.php?page=Nagios+plugins+-+interfacetable_v3t) or [https://github.com](https://github.com/Tontonitch/interfacetable_v3t).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                                | Description
------------------------------------|-----------------------------------------------------------------------------------------------------
interfacetable_hostquery            | **Required.** Specifies the remote host to poll. Defaults to "$address$".
interfacetable_hostdisplay          | **Optional.** Specifies the hostname to display in the HTML link. Defaults to "$host.display_name$".
interfacetable_regex                | **Optional.** Interface names and property names for some other options will be interpreted as regular expressions. Defaults to false.
interfacetable_outputshort          | **Optional.** Reduce the verbosity of the plugin output. Defaults to false.
interfacetable_exclude              | **Optional.** Comma separated list of interfaces globally excluded from the monitoring.
interfacetable_include              | **Optional.** Comma separated list of interfaces globally included in the monitoring.
interfacetable_aliasmatching        | **Optional.** Allow you to specify alias in addition to interface names. Defaults to false.
interfacetable_excludetraffic       | **Optional.** Comma separated list of interfaces excluded from traffic checks.
interfacetable_includetraffic       | **Optional.** Comma separated list of interfaces included for traffic checks.
interfacetable_warningtraffic       | **Optional.** Interface traffic load percentage leading to a warning alert.
interfacetable_criticaltraffic      | **Optional.** Interface traffic load percentage leading to a critical alert.
interfacetable_pkt                  | **Optional.** Add unicast/non-unicast pkt stats for each interface.
interfacetable_trackproperty        | **Optional.** List of tracked properties.
interfacetable_excludeproperty      | **Optional.** Comma separated list of interfaces excluded from the property tracking.
interfacetable_includeproperty      | **Optional.** Comma separated list of interfaces included in the property tracking.
interfacetable_community            | **Optional.** Specifies the snmp v1/v2c community string. Defaults to "public" if using snmp v1/v2c, ignored using v3.
interfacetable_snmpv2               | **Optional.** Use snmp v2c. Defaults to false.
interfacetable_login                | **Optional.** Login for snmpv3 authentication.
interfacetable_passwd               | **Optional.** Auth password for snmpv3 authentication.
interfacetable_privpass             | **Optional.** Priv password for snmpv3 authentication.
interfacetable_protocols            | **Optional.** Authentication protocol,Priv protocol for snmpv3 authentication.
interfacetable_domain               | **Optional.** SNMP transport domain.
interfacetable_contextname          | **Optional.** Context name for the snmp requests.
interfacetable_port                 | **Optional.** SNMP port. Defaults to standard port.
interfacetable_64bits               | **Optional.** Use SNMP 64-bits counters. Defaults to false.
interfacetable_maxrepetitions       | **Optional.** Increasing this value may enhance snmp query performances by gathering more results at one time.
interfacetable_snmptimeout          | **Optional.** Define the Transport Layer timeout for the snmp queries.
interfacetable_snmpretries          | **Optional.** Define the number of times to retry sending a SNMP message.
interfacetable_unixsnmp             | **Optional.** Use unix snmp utilities for snmp requests. Defaults to false, which means use the perl bindings.
interfacetable_enableperfdata       | **Optional.** Enable port performance data. Defaults to false.
interfacetable_perfdataformat       | **Optional.** Define which performance data will be generated. Possible values are "full" (default), "loadonly", "globalonly".
interfacetable_perfdatathreshold    | **Optional.** Define which thresholds are printed in the generated performance data. Possible values are "full" (default), "loadonly", "globalonly".
interfacetable_perfdatadir          | **Optional.** When specified, the performance data are also written directly to a file, in the specified location.
interfacetable_perfdataservicedesc  | **Optional.** Specify additional parameters for output performance data to PNP. Defaults to "$service.name$", only affects **interfacetable_perfdatadir**.
interfacetable_grapher              | **Optional.** Specify the used graphing solution. Possible values are "pnp4nagios" (default), "nagiosgrapher", "netwaysgrapherv2" and "ingraph".
interfacetable_grapherurl           | **Optional.** Graphing system url. Default depends on **interfacetable_grapher**.
interfacetable_portperfunit         | **Optional.** Traffic could be reported in bits (counters) or in bps (calculated value).
interfacetable_nodetype             | **Optional.** Specify the node type, for specific information to be printed / specific oids to be used. Possible values: "standard" (default), "cisco", "hp", "netscreen", "netapp", "bigip", "bluecoat", "brocade", "brocade-nos", "nortel", "hpux".
interfacetable_duplex               | **Optional.** Add the duplex mode property for each interface in the interface table. Defaults to false.
interfacetable_stp                  | **Optional.** Add the stp state property for each interface in the interface table. Defaults to false.
interfacetable_vlan                 | **Optional.** Add the vlan attribution property for each interface in the interface table. Defaults to false. This option is available only for the following nodetypes: "cisco", "hp", "nortel"
interfacetable_noipinfo             | **Optional.** Remove the ip information for each interface from the interface table. Defaults to false.
interfacetable_alias                | **Optional.** Add the alias information for each interface in the interface table. Defaults to false.
interfacetable_accessmethod         | **Optional.** Access method for a shortcut to the host in the HTML page. Format is : <method>[:<target>] Where method can be: ssh, telnet, http or https.
interfacetable_htmltablelinktarget  | **Optional.** Specifies the windows or the frame where the [details] link will load the generated html page. Possible values are: "_blank", "_self" (default), "_parent", "_top", or a frame name.
interfacetable_delta                | **Optional.** Set the delta used for interface throuput calculation in seconds.
interfacetable_ifs                  | **Optional.** Input field separator. Defaults to ",".
interfacetable_cache                | **Optional.** Define the retention time of the cached data in seconds.
interfacetable_noifloadgradient     | **Optional.** Disable color gradient from green over yellow to red for the load percentage. Defaults to false.
interfacetable_nohuman              | **Optional.** Do not translate bandwidth usage in human readable format. Defaults to false.
interfacetable_snapshot             | **Optional.** Force the plugin to run like if it was the first launch. Defaults to false.
interfacetable_timeout              | **Optional.** Define the global timeout limit of the plugin in seconds. Defaults to "15s".
interfacetable_css                  | **Optional.** Define the css stylesheet used by the generated html files. Possible values are "classic", "icinga", "icinga-alternate1" or "nagiosxi".
interfacetable_config               | **Optional.** Specify a config file to load.
interfacetable_noconfigtable        | **Optional.** Disable configuration table on the generated HTML page. Defaults to false.
interfacetable_notips               | **Optional.** Disable the tips in the generated html tables. Defaults to false.
interfacetable_defaulttablesorting  | **Optional.** Default table sorting can be "index" (default) or "name".
interfacetable_tablesplit           | **Optional.** Generate multiple interface tables, one per interface type. Defaults to false.
interfacetable_notype               | **Optional.** Remove the interface type for each interface. Defaults to false.

### <a id="plugins-contrib-command-iftraffic"></a> iftraffic

The plugin [check_iftraffic](https://exchange.icinga.org/exchange/iftraffic)
checks the utilization of a given interface name using the SNMP protocol.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|---------------------------------------------------------
iftraffic_address	| **Required.** Specifies the remote host. Defaults to "$address$".
iftraffic_community	| **Optional.** SNMP community. Defaults to "public'" if omitted.
iftraffic_interface	| **Required.** Queried interface name.
iftraffic_bandwidth	| **Required.** Interface maximum speed in kilo/mega/giga/bits per second.
iftraffic_units		| **Optional.** Interface units can be one of these values: `g` (gigabits/s),`m` (megabits/s), `k` (kilobits/s),`b` (bits/s)
iftraffic_warn		| **Optional.** Percent of bandwidth usage necessary to result in warning status (defaults to `85%`).
iftraffic_crit		| **Optional.** Percent of bandwidth usage necessary to result in critical status (defaults to `98%`).
iftraffic_max_counter	| **Optional.** Maximum counter value of net devices in kilo/mega/giga/bytes.

## <a id="plugins-contrib-web"></a> Web

This category includes all plugins for web-based checks.

### <a id="plugin-check-command-webinject"></a> webinject

Check command object for the [check_webinject](http://http://www.webinject.org/manual.html) plugin.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
webinject_config_file   | **Optional.** There is a configuration file named 'config.xml' that is used to store configuration settings for your project. You can use this to specify which test case files to run and to set some constants and settings to be used by WebInject.
webinject_output        | **Optional.** This option is followed by a directory name or a prefix to prepended to the output files. This is used to specify the location for writing output files (http.log, results.html, and results.xml). If a directory name is supplied (use either an absolute or relative path and make sure to add the trailing slash), all output files are written to this directory. If the trailing slash is ommitted, it is assumed to a prefix and this will be prepended to the output files. You may also use a combination of a directory and prefix.
webinject_no_output     | **Optional.** Suppresses all output to STDOUT except the results summary.
webinject_timeout       | **Optional.** The value [given in seconds] will be compared to the global time elapsed to run all the tests. If the tests have all been successful, but have taken more time than the 'globaltimeout' value, a warning message is sent back to Icinga.
webinject_report_type   | **Optional.** This setting is used to enable output formatting that is compatible for use with specific external programs. The available values you can set this to are: nagios, mrtg, external and standard.
webinject_testcase_file | **Optional.** When you launch WebInject in console mode, you can optionally supply an argument for a testcase file to run. It will look for this file in the directory that webinject.pl resides in. If no filename is passed from the command line, it will look in config.xml for testcasefile declarations. If no files are specified, it will look for a default file named 'testcases.xml' in the current [webinject] directory. If none of these are found, the engine will stop and give you an error.

### <a id="plugin-check-command-jmx4perl"></a> jmx4perl

The plugin `jmx4perl` utilizes the api provided by the jolokia web application to query java message beans on an application server. It is part of the perl module provided by Roland Huß on [cpan](http://search.cpan.org/~roland/jmx4perl/) including a detailed [documentation](http://search.cpan.org/~roland/jmx4perl/scripts/check_jmx4perl) containing installation tutorial, security advices und usage examples.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                         | Description
-----------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------
jmx4perl_url                 | **Required.** URL to agent web application. Defaults to "http://$address$:8080/jolokia".
jmx4perl_product             | **Optional.** Name of app server product (e.g. jboss), by default is uses an autodetection facility.
jmx4perl_alias               | **Optional.** Alias name for attribute (e.g. MEMORY_HEAP_USED). All availables aliases can be viewed by executing `jmx4perl aliases` on the command line.
jmx4perl_mbean               | **Optional.** MBean name (e.g. java.lang:type=Memory).
jmx4perl_attribute           | **Optional.** Attribute name (e.g. HeapMemoryUsage).
jmx4perl_operation           | **Optional.** Operation to execute.
jmx4perl_value               | **Optional.** Shortcut for specifying mbean/attribute/path. Slashes within names must be escaped with backslash.
jmx4perl_delta               | **Optional.** Switches on incremental mode. Optional argument are seconds used for normalizing.
jmx4perl_path                | **Optional.** Inner path for extracting a single value from a complex attribute or return value (e.g. used).
jmx4perl_target              | **Optional.** JSR-160 Service URL specifing the target server.
jmx4perl_target_user         | **Optional.** Username to use for JSR-160 connection.
jmx4perl_target_password     | **Optional.** Password to use for JSR-160 connection.
jmx4perl_proxy               | **Optional.** Proxy to use.
jmx4perl_user                | **Optional.** User for HTTP authentication.
jmx4perl_password            | **Optional.** Password for HTTP authentication.
jmx4perl_name                | **Optional.** Name to use for output, by default a standard value based on the MBean and attribute will be used.
jmx4perl_method              | **Optional.** HTTP method to use, either get or post. By default a method is determined automatically based on the request type.
jmx4perl_base                | **Optional.** Base name, which when given, interprets critical and warning values as relative in the range 0 .. 100%. Must be given in the form mbean/attribute/path.
jmx4perl_base_mbean          | **Optional.** Base MBean name, interprets critical and warning values as relative in the range 0 .. 100%. Requires "jmx4perl_base_attribute".
jmx4perl_base_attribute      | **Optional.** Base attribute for a relative check. Requires "jmx4perl_base_mbean".
jmx4perl_base_path           | **Optional.** Base path for relative checks, where this path is used on the base attribute's value.
jmx4perl_unit                | **Optional.** Unit of measurement of the data retreived. Recognized values are [B|KB|MN|GB|TB] for memory values and [us|ms|s|m|h|d] for time values.
jmx4perl_null                | **Optional.** Value which should be used in case of a null return value of an operation or attribute. Defaults to null.
jmx4perl_string              | **Optional.** Force string comparison for critical and warning checks. Defaults to false.
jmx4perl_numeric             | **Optional.** Force numeric comparison for critical and warning checks. Defaults to false.
jmx4perl_critical            | **Optional.** Critical threshold for value.
jmx4perl_warning             | **Optional.** Warning threshold for value.
jmx4perl_label               | **Optional.** Label to be used for printing out the result of the check. For placeholders which can be used see the documentation.
jmx4perl_perfdata            | **Optional.** Whether performance data should be omitted, which are included by default. Defaults to "on" for numeric values, to "off" for strings.
jmx4perl_unknown_is_critical | **Optional.** Map UNKNOWN errors to errors with a CRITICAL status. Defaults to false.
jmx4perl_timeout             | **Optional.** Seconds before plugin times out. Defaults to "15".


## <a id="plugins-contrib-operating-system"></a> Operating System

In this category you can find plugins for gathering information about your operating system or the system beneath like memory usage.

### <a id="plugins-contrib-command-mem"></a> mem

The plugin `mem` is used for gathering information about memory usage on linux and unix hosts. It is able to count cache memory as free when comparing it to the thresholds. It is provided by `Justin Ellison` on [https://github.com](https://github.com/justintime/nagios-plugins). For more details see the developers blog [http://sysadminsjourney.com](http://sysadminsjourney.com/content/2009/06/04/new-and-improved-checkmempl-nagios-plugin).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name         | Description
-------------|-----------------------------------------------------------------------------------------------------------------------
mem_used     | **Optional.** Tell the plugin to check for used memory in opposite of **mem_free**. Must specify one of these as true.
mem_free     | **Optional.** Tell the plugin to check for free memory in opposite of **mem_used**. Must specify one of these as true.
mem_cache    | **Optional.** If set to true plugin will count cache as free memory. Defaults to false.
mem_warning  | **Required.** Specifiy the warning threshold as number interpreted as percent.
mem_critical | **Required.** Specifiy the critical threshold as number interpreted as percent.

## <a id="plugin-contrib-command-running-kernel"></a> running_kernel

Check command object for the `check_running_kernel` plugin
provided by the `nagios-plugins-contrib` package on Debian.

The `running_kernel` check command does not support any vars.


## <a id="plugins-contrib-virtualization"></a> Virtualization

This category includes all plugins for various virtualization technologies.

### <a id="plugins-contrib-command-esxi-hardware"></a> esxi_hardware

The plugin `esxi_hardware` is a plugin to monitor hardware of ESXi servers through the vmware api and cim service. It is provided by `Claudio Kuenzler` on [http://www.claudiokuenzler.com](http://www.claudiokuenzler.com/nagios-plugins/check_esxi_hardware.php). For instruction on creating the required local user and workarounds for some hardware types have a look on his homepage.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
esxi_hardware_host      | **Required.** Specifies the host to monitor. Defaults to "$address$".
esxi_hardware_user      | **Required.** Specifies the user for polling. Must be a local user of the root group on the system. Can also be provided as a file path file:/path/to/.passwdfile, then first string of file is used.
esxi_hardware_pass      | **Required.** Password of the user. Can also be provided as a file path file:/path/to/.passwdfile, then second string of file is used.
esxi_hardware_vendor    | **Optional.** Defines the vendor of the server: "auto", "dell", "hp", "ibm", "intel", "unknown" (default).
esxi_hardware_html      | **Optional.** Add web-links to hardware manuals for Dell servers (use your country extension). Only useful with **esxi_hardware_vendor** = dell.
esxi_hardware_ignore    | **Optional.** Comma separated list of elements to ignore.
esxi_hardware_perfdata  | **Optional.** Add performcedata for graphers like PNP4Nagios to the output. Defaults to false.
esxi_hardware_nopower   | **Optional.** Do not collect power performance data, when **esxi_hardware_perfdata** is set to true. Defaults to false.
esxi_hardware_novolts   | **Optional.** Do not collect voltage performance data, when **esxi_hardware_perfdata** is set to true. Defaults to false.
esxi_hardware_nocurrent | **Optional.** Do not collect current performance data, when **esxi_hardware_perfdata** is set to true. Defaults to false.
esxi_hardware_notemp    | **Optional.** Do not collect temperature performance data, when **esxi_hardware_perfdata** is set to true. Defaults to false.
esxi_hardware_nofan     | **Optional.** Do not collect fan performance data, when **esxi_hardware_perfdata** is set to true. Defaults to false.

# <a id="plugins-contrib-vmware"></a> VMware

Check commands for the [check_vmware_esx](https://github.com/BaldMansMojo/check_vmware_esx) plugin.

## <a id="plugins-contrib-vmware-esx-dc-volumes"></a> vmware-esx-dc-volumes

Check command object for the `check_vmware_esx` plugin. Shows all datastore volumes info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_subselect        | **Optional.** Volume name to be checked the free space.
vmware_gigabyte         | **Optional.** Output in GB instead of MB.
vmware_usedspace        | **Optional.** Output used space instead of free. Defaults to "false".
vmware_alertonly        | **Optional.** List only alerting volumes. Defaults to "false".
vmware_exclude          | **Optional.** Blacklist volumes name. No value defined as default.
vmware_include          | **Optional.** Whitelist volumes name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_dc_volume_used   | **Optional.** Output used space instead of free. Defaults to "true".
vmware_warn             | **Optional.** The warning threshold for volumes. Defaults to "80%".
vmware_crit             | **Optional.** The critical threshold for volumes. Defaults to "90%".


## <a id="plugins-contrib-vmware-esx-dc-runtime-info"></a> vmware-esx-dc-runtime-info

Check command object for the `check_vmware_esx` plugin. Shows all runtime info for the datacenter/Vcenter.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-dc-runtime-listvms"></a> vmware-esx-dc-runtime-listvms

Check command object for the `check_vmware_esx` plugin. List of vmware machines and their power state. BEWARE!! In larger environments systems can cause trouble displaying the informations needed due to the mass of data. Use **vmware_alertonly** to avoid this.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_alertonly        | **Optional.** List only alerting VMs. Important here to avoid masses of data.
vmware_exclude          | **Optional.** Blacklist VMs name. No value defined as default.
vmware_include          | **Optional.** Whitelist VMs name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-dc-runtime-listhost"></a> vmware-esx-dc-runtime-listhost

Check command object for the `check_vmware_esx` plugin. List of VMware ESX hosts and their power state.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_alertonly        | **Optional.** List only alerting hosts. Important here to avoid masses of data.
vmware_exclude          | **Optional.** Blacklist VMware ESX hosts. No value defined as default.
vmware_include          | **Optional.** Whitelist VMware ESX hosts. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-dc-runtime-listcluster"></a> vmware-esx-dc-runtime-listcluster

Check command object for the `check_vmware_esx` plugin. List of VMware clusters and their states.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_alertonly        | **Optional.** List only alerting hosts. Important here to avoid masses of data.
vmware_exclude          | **Optional.** Blacklist VMware cluster. No value defined as default.
vmware_include          | **Optional.** Whitelist VMware cluster. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-dc-runtime-issues"></a> vmware-esx-dc-runtime-issues

Check command object for the `check_vmware_esx` plugin. All issues for the host.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist issues. No value defined as default.
vmware_include          | **Optional.** Whitelist issues. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-dc-runtime-status"></a> vmware-esx-dc-runtime-status

Check command object for the `check_vmware_esx` plugin. Overall object status (gray/green/red/yellow).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-dc-runtime-tools"></a> vmware-esx-dc-runtime-tools

Check command object for the `check_vmware_esx` plugin. Vmware Tools status.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Required.** Datacenter/vCenter hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_poweredonly      | **Optional.** List only VMs which are powered on. No value defined as default.
vmware_alertonly        | **Optional.** List only alerting VMs. Important here to avoid masses of data.
vmware_exclude          | **Optional.** Blacklist VMs. No value defined as default.
vmware_include          | **Optional.** Whitelist VMs. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-check"></a> vmware-esx-soap-host-check

Check command object for the `check_vmware_esx` plugin. Simple check to verify a successfull connection to VMware SOAP API.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-uptime"></a> vmware-esx-soap-host-uptime

Check command object for the `check_vmware_esx` plugin. Displays uptime of the VMware host.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-cpu"></a> vmware-esx-soap-host-cpu

Check command object for the `check_vmware_esx` plugin. CPU usage in percentage.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. Defaults to "80%".
vmware_crit             | **Optional.** The critical threshold in percent. Defaults to "90%".


## <a id="plugins-contrib-vmware-esx-soap-host-cpu-ready"></a> vmware-esx-soap-host-cpu-ready

Check command object for the `check_vmware_esx` plugin. Percentage of time that the virtual machine was ready, but could not get scheduled to run on the physical CPU. CPU ready time is dependent on the number of virtual machines on the host and their CPU loads. High or growing ready time can be a hint CPU bottlenecks.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-cpu-wait"></a> vmware-esx-soap-host-cpu-wait

Check command object for the `check_vmware_esx` plugin. CPU time spent in wait state. The wait total includes time spent the CPU idle, CPU swap wait, and CPU I/O wait states. High or growing wait time can be a hint I/O bottlenecks.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-cpu-usage"></a> vmware-esx-soap-host-cpu-usage

Check command object for the `check_vmware_esx` plugin. Actively used CPU of the host, as a percentage of the total available CPU. Active CPU is approximately equal to the ratio of the used CPU to the available CPU.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. Defaults to "80%".
vmware_crit             | **Optional.** The critical threshold in percent. Defaults to "90%".


## <a id="plugins-contrib-vmware-esx-soap-host-mem"></a> vmware-esx-soap-host-mem

Check command object for the `check_vmware_esx` plugin. All mem info(except overall and no thresholds).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-mem-usage"></a> vmware-esx-soap-host-mem-usage

Check command object for the `check_vmware_esx` plugin. Average mem usage in percentage.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. Defaults to "80%".
vmware_crit             | **Optional.** The critical threshold in percent. Defaults to "90%".


## <a id="plugins-contrib-vmware-esx-soap-host-mem-consumed"></a> vmware-esx-soap-host-mem-consumed

Check command object for the `check_vmware_esx` plugin. Amount of machine memory used on the host. Consumed memory includes Includes memory used by the Service Console, the VMkernel vSphere services, plus the total consumed metrics for all running virtual machines in MB.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. No value defined as default.
vmware_crit             | **Optional.** The critical threshold in percent. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-mem-swapused"></a> vmware-esx-soap-host-mem-swapused

Check command object for the `check_vmware_esx` plugin. Amount of memory that is used by swap. Sum of memory swapped of all powered on VMs and vSphere services on the host in MB. In case of an error all VMs with their swap used will be displayed.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. No value defined as default.
vmware_crit             | **Optional.** The critical threshold in percent. No value defined as default.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-mem-overhead"></a> vmware-esx-soap-host-mem-overhead

Check command object for the `check_vmware_esx` plugin. Additional mem used by VM Server in MB.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. No value defined as default.
vmware_crit             | **Optional.** The critical threshold in percent. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-mem-memctl"></a> vmware-esx-soap-host-mem-memctl

Check command object for the `check_vmware_esx` plugin. The sum of all vmmemctl values in MB for all powered-on virtual machines, plus vSphere services on the host. If the balloon target value is greater than the balloon value, the VMkernel inflates the balloon, causing more virtual machine memory to be reclaimed. If the balloon target value is less than the balloon value, the VMkernel deflates the balloon, which allows the virtual machine to consume additional memory if needed.used by VM memory control driver. In case of an error all VMs with their vmmemctl values will be displayed.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in percent. No value defined as default.
vmware_crit             | **Optional.** The critical threshold in percent. No value defined as default.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-net"></a> vmware-esx-soap-host-net

Check command object for the `check_vmware_esx` plugin. Shows net info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist NICs. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist expression as regexp.


## <a id="plugins-contrib-vmware-esx-soap-host-net-usage"></a> vmware-esx-soap-host-net-usage

Check command object for the `check_vmware_esx` plugin. Overall network usage in KBps(Kilobytes per Second).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in KBps(Kilobytes per Second). No value defined as default.
vmware_crit             | **Optional.** The critical threshold in KBps(Kilobytes per Second). No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-net-receive"></a> vmware-esx-soap-host-net-receive

Check command object for the `check_vmware_esx` plugin. Data receive in KBps(Kilobytes per Second).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in KBps(Kilobytes per Second). No value defined as default.
vmware_crit             | **Optional.** The critical threshold in KBps(Kilobytes per Second). No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-net-send"></a> vmware-esx-soap-host-net-send

Check command object for the `check_vmware_esx` plugin. Data send in KBps(Kilobytes per Second).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold in KBps(Kilobytes per Second). No value defined as default.
vmware_crit             | **Optional.** The critical threshold in KBps(Kilobytes per Second). No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-net-nic"></a> vmware-esx-soap-host-net-nic

Check command object for the `check_vmware_esx` plugin. Check all active NICs.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist NICs. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist expression as regexp.


## <a id="plugins-contrib-vmware-esx-soap-host-volumes"></a> vmware-esx-soap-host-volumes

Check command object for the `check_vmware_esx` plugin. Shows all datastore volumes info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_subselect        | **Optional.** Volume name to be checked the free space.
vmware_gigabyte         | **Optional.** Output in GB instead of MB.
vmware_usedspace        | **Optional.** Output used space instead of free. Defaults to "false".
vmware_alertonly        | **Optional.** List only alerting volumes. Defaults to "false".
vmware_exclude          | **Optional.** Blacklist volumes name. No value defined as default.
vmware_include          | **Optional.** Whitelist volumes name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_warn             | **Optional.** The warning threshold for volumes. Defaults to "80%".
vmware_crit             | **Optional.** The critical threshold for volumes. Defaults to "90%".
vmware_spaceleft        | **Optional.** This has to be used in conjunction with thresholds as mentioned above.


## <a id="plugins-contrib-vmware-esx-soap-host-io"></a> vmware-esx-soap-host-io

Check command object for the `check_vmware_esx` plugin. Shows all disk io info. Without subselect no thresholds can be given. All I/O values are aggregated from historical intervals over the past 24 hours with a 5 minute sample rate.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-io-aborted"></a> vmware-esx-soap-host-io-aborted

Check command object for the `check_vmware_esx` plugin. Number of aborted SCSI commands.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-resets"></a> vmware-esx-soap-host-io-resets

Check command object for the `check_vmware_esx` plugin. Number of SCSI bus resets.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-read"></a> vmware-esx-soap-host-io-read

Check command object for the `check_vmware_esx` plugin. Average number of kilobytes read from the disk each second.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-read-latency"></a> vmware-esx-soap-host-io-read-latency

Check command object for the `check_vmware_esx` plugin. Average amount of time (ms) to process a SCSI read command issued from the Guest OS to the virtual machine.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-write"></a> vmware-esx-soap-host-io-write

Check command object for the `check_vmware_esx` plugin. Average number of kilobytes written to disk each second.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-write-latency"></a> vmware-esx-soap-host-io-write-latency

Check command object for the `check_vmware_esx` plugin. Average amount of time (ms) taken to process a SCSI write command issued by the Guest OS to the virtual machine.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-usage"></a> vmware-esx-soap-host-io-usage

Check command object for the `check_vmware_esx` plugin. Aggregated disk I/O rate. For hosts, this metric includes the rates for all virtual machines running on the host.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-kernel-latency"></a> vmware-esx-soap-host-io-kernel-latency

Check command object for the `check_vmware_esx` plugin. Average amount of time (ms) spent by VMkernel processing each SCSI command.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-device-latency"></a> vmware-esx-soap-host-io-device-latency

Check command object for the `check_vmware_esx` plugin. Average amount of time (ms) to complete a SCSI command from the physical device.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-queue-latency"></a> vmware-esx-soap-host-io-queue-latency

Check command object for the `check_vmware_esx` plugin. Average amount of time (ms) spent in the VMkernel queue.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-io-total-latency"></a> vmware-esx-soap-host-io-total-latency

Check command object for the `check_vmware_esx` plugin. Average amount of time (ms) taken during the collection interval to process a SCSI command issued by the guest OS to the virtual machine. The sum of kernelWriteLatency and deviceWriteLatency.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-media"></a> vmware-esx-soap-host-media

Check command object for the `check_vmware_esx` plugin. List vm's with attached host mounted media like cd,dvd or floppy drives. This is important for monitoring because a virtual machine with a mount cd or dvd drive can not be moved to another host.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist VMs name. No value defined as default.
vmware_include          | **Optional.** Whitelist VMs name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-service"></a> vmware-esx-soap-host-service

Check command object for the `check_vmware_esx` plugin. Shows host service info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist services name. No value defined as default.
vmware_include          | **Optional.** Whitelist services name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime"></a> vmware-esx-soap-host-runtime

Check command object for the `check_vmware_esx` plugin. Shows runtime info: VMs, overall status, connection state, health, storagehealth, temperature and sensor are represented as one value and without thresholds.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-con"></a> vmware-esx-soap-host-runtime-con

Check command object for the `check_vmware_esx` plugin. Shows connection state.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-listvms"></a> vmware-esx-soap-host-runtime-listvms

Check command object for the `check_vmware_esx` plugin. List of VMware machines and their status.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist VMs name. No value defined as default.
vmware_include          | **Optional.** Whitelist VMs name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-status"></a> vmware-esx-soap-host-runtime-status

Check command object for the `check_vmware_esx` plugin. Overall object status (gray/green/red/yellow).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-health"></a> vmware-esx-soap-host-runtime-health

Check command object for the `check_vmware_esx` plugin. Checks cpu/storage/memory/sensor status.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist status name. No value defined as default.
vmware_include          | **Optional.** Whitelist status name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-health-listsensors"></a> vmware-esx-soap-host-runtime-health-listsensors

Check command object for the `check_vmware_esx` plugin. List all available sensors(use for listing purpose only).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist status name. No value defined as default.
vmware_include          | **Optional.** Whitelist status name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-health-nostoragestatus"></a> vmware-esx-soap-host-runtime-health-nostoragestatus

Check command object for the `check_vmware_esx` plugin. This is to avoid a double alarm if you use **vmware-esx-soap-host-runtime-health** and **vmware-esx-soap-host-runtime-storagehealth**.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist status name. No value defined as default.
vmware_include          | **Optional.** Whitelist status name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-storagehealth"></a> vmware-esx-soap-host-runtime-storagehealth

Check command object for the `check_vmware_esx` plugin. Local storage status check.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist storage name. No value defined as default.
vmware_include          | **Optional.** Whitelist storage name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-temp"></a> vmware-esx-soap-host-runtime-temp

Check command object for the `check_vmware_esx` plugin. Lists all temperature sensors.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist sensor name. No value defined as default.
vmware_include          | **Optional.** Whitelist sensor name. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-runtime-issues"></a> vmware-esx-soap-host-runtime-issues

Check command object for the `check_vmware_esx` plugin. Lists all configuration issues for the host.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist configuration issues. No value defined as default.
vmware_include          | **Optional.** Whitelist configuration issues. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-storage"></a> vmware-esx-soap-host-storage

Check command object for the `check_vmware_esx` plugin. Shows Host storage info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist adapters, luns and paths. No value defined as default.
vmware_include          | **Optional.** Whitelist adapters, luns and paths. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.


## <a id="plugins-contrib-vmware-esx-soap-host-storage-adapter"></a> vmware-esx-soap-host-storage-adapter

Check command object for the `check_vmware_esx` plugin. List host bus adapters.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist adapters. No value defined as default.
vmware_include          | **Optional.** Whitelist adapters. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-storage-lun"></a> vmware-esx-soap-host-storage-lun

Check command object for the `check_vmware_esx` plugin. List SCSI logical units. The listing will include: LUN, canonical name of the disc, all of displayed name which is not part of the canonical name and status.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_exclude          | **Optional.** Blacklist luns. No value defined as default.
vmware_include          | **Optional.** Whitelist luns. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-host-storage-path"></a> vmware-esx-soap-host-storage-path

Check command object for the `check_vmware_esx` plugin. List multipaths and the associated paths.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_host             | **Required.** ESX or ESXi hostname.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_alertonly        | **Optional.** List only alerting units. Important here to avoid masses of data. Defaults to "false".
vmware_exclude          | **Optional.** Blacklist paths. No value defined as default.
vmware_include          | **Optional.** Whitelist paths. No value defined as default.
vmware_isregexp         | **Optional.** Treat blacklist and whitelist expressions as regexp.
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-cpu"></a> vmware-esx-soap-vm-cpu

Check command object for the `check_vmware_esx` plugin. Shows all CPU usage info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd



## <a id="plugins-contrib-vmware-esx-soap-vm-cpu-ready"></a> vmware-esx-soap-vm-cpu-ready

Check command object for the `check_vmware_esx` plugin. Percentage of time that the virtual machine was ready, but could not get scheduled to run on the physical CPU.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-cpu-wait"></a> vmware-esx-soap-vm-cpu-wait

Check command object for the `check_vmware_esx` plugin. CPU time spent in wait state. The wait total includes time spent the CPU idle, CPU swap wait, and CPU I/O wait states. High or growing wait time can be a hint I/O bottlenecks.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-cpu-usage"></a> vmware-esx-soap-vm-cpu-usage

Check command object for the `check_vmware_esx` plugin. Amount of actively used virtual CPU, as a percentage of total available CPU. This is the host's view of the CPU usage, not the guest operating system view. It is the average CPU utilization over all available virtual CPUs in the virtual machine.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** Warning threshold in percent. Defaults to "80%".
vmware_crit             | **Optional.** Critical threshold in percent. Defaults to "90%".


## <a id="plugins-contrib-vmware-esx-soap-vm-mem"></a> vmware-esx-soap-vm-mem

Check command object for the `check_vmware_esx` plugin. Shows all memory info, except overall.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-mem-usage"></a> vmware-esx-soap-vm-mem-usage

Check command object for the `check_vmware_esx` plugin. Average mem usage in percentage of configured virtual machine "physical" memory.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** Warning threshold in percent. Defaults to "80%".
vmware_crit             | **Optional.** Critical threshold in percent. Defaults to "90%".


## <a id="plugins-contrib-vmware-esx-soap-vm-mem-consumed"></a> vmware-esx-soap-vm-mem-consumed

Check command object for the `check_vmware_esx` plugin. Amount of guest physical memory in MB consumed by the virtual machine for guest memory. Consumed memory does not include overhead memory. It includes shared memory and memory that might be reserved, but not actually used. Use this metric for charge-back purposes.<br>
**vm consumed memory = memory granted - memory saved**

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-mem-memctl"></a> vmware-esx-soap-vm-mem-memctl

Check command object for the `check_vmware_esx` plugin. Amount of guest physical memory that is currently reclaimed from the virtual machine through ballooning. This is the amount of guest physical memory that has been allocated and pinned by the balloon driver.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.



## <a id="plugins-contrib-vmware-esx-soap-vm-net"></a> vmware-esx-soap-vm-net

Check command object for the `check_vmware_esx` plugin. Shows net info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-net-usage"></a> vmware-esx-soap-vm-net-usage

Check command object for the `check_vmware_esx` plugin. Overall network usage in KBps(Kilobytes per Second).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-net-receive"></a> vmware-esx-soap-vm-net-receive

Check command object for the `check_vmware_esx` plugin. Receive in KBps(Kilobytes per Second).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-net-send"></a> vmware-esx-soap-vm-net-send

Check command object for the `check_vmware_esx` plugin. Send in KBps(Kilobytes per Second).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-io"></a> vmware-esx-soap-vm-io

Check command object for the `check_vmware_esx` plugin. SShows all disk io info. Without subselect no thresholds can be given. All I/O values are aggregated from historical intervals over the past 24 hours with a 5 minute sample rate.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-io-read"></a> vmware-esx-soap-vm-io-read

Check command object for the `check_vmware_esx` plugin. Average number of kilobytes read from the disk each second.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-io-write"></a> vmware-esx-soap-vm-io-write

Check command object for the `check_vmware_esx` plugin. Average number of kilobytes written to disk each second.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-io-usage"></a> vmware-esx-soap-vm-io-usage

Check command object for the `check_vmware_esx` plugin. Aggregated disk I/O rate.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime"></a> vmware-esx-soap-vm-runtime

Check command object for the `check_vmware_esx` plugin. Shows virtual machine runtime info.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-con"></a> vmware-esx-soap-vm-runtime-con

Check command object for the `check_vmware_esx` plugin. Shows the connection state.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-powerstate"></a> vmware-esx-soap-vm-runtime-powerstate

Check command object for the `check_vmware_esx` plugin. Shows virtual machine power state: poweredOn, poweredOff or suspended.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-status"></a> vmware-esx-soap-vm-runtime-status

Check command object for the `check_vmware_esx` plugin. Overall object status (gray/green/red/yellow).

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-consoleconnections"></a> vmware-esx-soap-vm-runtime-consoleconnections

Check command object for the `check_vmware_esx` plugin. Console connections to virtual machine.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_warn             | **Optional.** The warning threshold. No value defined as default.
vmware_crit             | **Optional.** The critical threshold. No value defined as default.


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-gueststate"></a> vmware-esx-soap-vm-runtime-gueststate

Check command object for the `check_vmware_esx` plugin. Guest OS status. Needs VMware Tools installed and running.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd

## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-tools"></a> vmware-esx-soap-vm-runtime-tools

Check command object for the `check_vmware_esx` plugin. Guest OS status. VMware tools  status.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd


## <a id="plugins-contrib-vmware-esx-soap-vm-runtime-issues"></a> vmware-esx-soap-vm-runtime-issues

Check command object for the `check_vmware_esx` plugin. All issues for the virtual machine.

Custom attributes passed as [command parameters](3-monitoring-basics.md#command-passing-parameters):

Name                    | Description
------------------------|--------------
vmware_datacenter       | **Optional.** Datacenter/vCenter hostname. Conflicts with **vmware_host**.
vmware_host             | **Optional.** ESX or ESXi hostname. Conflicts with **vmware_datacenter**.
vmware_vmname           | **Required.** Virtual machine name.
vmware_sslport          | **Optional.** SSL port connection. Defaults to "443".
vmware_ignoreunknown    | **Optional.** Sometimes 3 (unknown) is returned from a component. But the check itself is ok. With this option the plugin will return OK (0) instead of UNKNOWN (3). Defaults to "false".
vmware_ignorewarning    | **Optional.** Sometimes 2 (warning) is returned from a component. But the check itself is ok (from an operator view). With this option the plugin will return OK (0) instead of WARNING (1). Defaults to "false".
vmware_timeout          | **Optional.** Seconds before plugin times out. Defaults to "90".
vmware_trace            | **Optional.** Set verbosity level of vSphere API request/respond trace.
vmware_sessionfile      | **Optional.** Session file name enhancement.
vmware_sessionfiledir   | **Optional.** Path to store the **vmware_sessionfile** file. Defaults to "/var/spool/icinga2/tmp".
vmware_nosession        | **Optional.** No auth session - IT SHOULD BE USED FOR TESTING PURPOSES ONLY!. Defaults to "false".
vmware_username         | **Optional.** The username to connect to Host or vCenter server. No value defined as default.
vmware_password         | **Optional.** The username's password. No value defined as default.
vmware_authfile         | **Optional.** Use auth file instead username/password to session connect. No effect if **vmware_username** and **vmware_password** are defined <br> **Autentication file content:** <br>  username=vmuser <br> password=p@ssw0rd
vmware_multiline        | **Optional.** Multiline output in overview. This mean technically that a multiline output uses a HTML **\<br\>** for the GUI. No value defined as default.
