# <a id="icinga-template-library"></a> Icinga Template Library

## <a id="itl-overview"></a> Overview

The Icinga Template Library (ITL) implements standard templates and object
definitions for commonly used services.

You can include the ITL by using the `include` directive in your configuration
file:

    include <itl/itl.conf>

The ITL assumes that there's a global constant named `PluginDir` which contains
the path of the plugins from the Monitoring Plugins project.

## <a id="itl-check-commands"></a> Check Commands

### <a id="itl-ping4"></a> ping4

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

### <a id="itl-ping6"></a> ping6

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

### <a id="itl-hostalive"></a> hostalive

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

### <a id="itl-dummy"></a> dummy

Check command object for the `check_dummy` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
dummy_text      | **Optional.** Plugin output. Defaults to "Check was successful.".

### <a id="itl-passive"></a> passive

Specialised check command object for passive checks executing the `check_dummy` plugin with appropriate default values.

Custom Attributes:

Name            | Description
----------------|--------------
dummy_state     | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 3.
dummy_text      | **Optional.** Plugin output. Defaults to "No Passive Check Result Received.".

### <a id="itl-tcp"></a> tcp

Check command object for the `check_tcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
tcp_address     | **Optional.** The host's address. Defaults to "$address$".
tcp_port        | **Required.** The port that should be checked.

### <a id="itl-udp"></a> udp

Check command object for the `check_udp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
udp_address     | **Optional.** The host's address. Defaults to "$address$".
udp_port        | **Required.** The port that should be checked.

### <a id="itl-http-ip"></a> http

Check command object for the `check_http` plugin.

Custom Attributes:

Name               | Description
-------------------|--------------
http_address       | **Optional.** The host's address. Defaults to "$address".
http_vhost         | **Optional.** The virtual host that should be sent in the "Host" header.
http_uri           | **Optional.** The request URI.
http_port          | **Optional.** The TCP port. Defaults to 80 when not using SSL, 443 otherwise.
http_ssl           | **Optional.** Whether to use SSL. Defaults to false.
http_warn_time     | **Optional.** The warning threshold.
http_critical_time | **Optional.** The critical threshold.

### <a id="itl-smtp"></a> smtp

Check command object for the `check_smtp` plugin.

Custom Attributes:

Name                 | Description
---------------------|--------------
smtp_address         | **Optional.** The host's address. Defaults to "$address$".

### <a id="itl-ssmtp"></a> ssmtp

Check command object for the `check_ssmtp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssmtp_address   | **Required.** The host's address. Defaults to "$address$".
ssmtp_port      | **Optional.** The port that should be checked. Defaults to 465.

### <a id="itl-ntp-time"></a> ntp_time

Check command object for the `check_ntp_time` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ntp_address     | **Optional.** The host's address. Defaults to "$address$".

### <a id="itl-ssh"></a> ssh

Check command object for the `check_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
ssh_address     | **Optional.** The host's address. Defaults to "$address$".
ssh_port        | **Optional.** The port that should be checked. Defaults to 22.

### <a id="itl-disk"></a> disk

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
disk_wfree      | **Optional.** The free space warning threshold in %. Defaults to 20.
disk_cfree      | **Optional.** The free space critical threshold in %. Defaults to 10.

### <a id="itl-users"></a> users

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
users_wgreater  | **Optional.** The user count warning threshold. Defaults to 20.
users_cgreater  | **Optional.** The user count critical threshold. Defaults to 50.

### <a id="itl-processes"></a> procs

Check command object for the `check_procs` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
procs_wgreater  | **Optional.** The process count warning threshold. Defaults to 250.
procs_cgreater  | **Optional.** The process count critical threshold. Defaults to 400.

### <a id="itl-swap"></a> swap

Check command object for the `check_swap` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
swap_wfree      | **Optional.** The free swap space warning threshold in %. Defaults to 50.
swap_cfree      | **Optional.** The free swap space critical threshold in %. Defaults to 25.

### <a id="itl-load"></a> load

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

### <a id="itl-snmp"></a> snmp

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Required.** The SNMP OID.
snmp_community  | **Optional.** The SNMP community. Defaults to "public".

### <a id="itl-snmp-uptime"></a> snmp-uptime

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
snmp_address    | **Optional.** The host's address. Defaults to "$address$".
snmp_oid        | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
snmp_community  | **Optional.** The SNMP community. Defaults to "public".

### <a id="itl-apt"></a> apt

Check command for the `check_apt` plugin.

The `apt` check command does not support any vars.

### <a id="itl-icinga"></a> icinga

Check command for the built-in `icinga` check. This check returns performance
data for the current Icinga instance.

The `icinga` check command does not support any vars.

