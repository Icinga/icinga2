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
address         | **Required.** The host's address.
wrta            | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
wpl             | **Optional.** The packet loss warning threshold in %. Defaults to 5.
crta            | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
cpl             | **Optional.** The packet loss critical threshold in %. Defaults to 15.
packets         | **Optional.** The number of packets to send. Defaults to 5.
timeout         | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

### <a id="itl-ping6"></a> ping6

Check command object for the `check_ping` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address6        | **Required.** The host's IPv6 address.
wrta            | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
wpl             | **Optional.** The packet loss warning threshold in %. Defaults to 5.
crta            | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
cpl             | **Optional.** The packet loss critical threshold in %. Defaults to 15.
packets         | **Optional.** The number of packets to send. Defaults to 5.
timeout         | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

### <a id="itl-hostalive"></a> hostalive

Check command object for the `check_ping` plugin with host check default values.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's IPv4 address.
wrta            | **Optional.** The RTA warning threshold in milliseconds. Defaults to 3000.0.
wpl             | **Optional.** The packet loss warning threshold in %. Defaults to 80.
crta            | **Optional.** The RTA critical threshold in milliseconds. Defaults to 5000.0.
cpl             | **Optional.** The packet loss critical threshold in %. Defaults to 100.
packets         | **Optional.** The number of packets to send. Defaults to 5.
timeout         | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

### <a id="itl-dummy"></a> dummy

Check command object for the `check_dummy` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
state           | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
text            | **Optional.** Plugin output. Defaults to "Check was successful.".

### <a id="itl-passive"></a> passive

Specialised check command object for passive checks executing the `check_dummy` plugin with appropriate default values.

Custom Attributes:

Name            | Description
----------------|--------------
state           | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 3.
text            | **Optional.** Plugin output. Defaults to "No Passive Check Result Received.".

### <a id="itl-tcp"></a> tcp

Check command object for the `check_tcp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.
port            | **Required.** The port that should be checked.

### <a id="itl-udp"></a> udp

Check command object for the `check_udp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.
port            | **Required.** The port that should be checked.

### <a id="itl-http-vhost"></a> http_vhost

Check command object for the `check_http` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
vhost           | **Required.** The name of the virtual host that should be checked.

### <a id="itl-http-ip"></a> http_ip

Check command object for the `check_http` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.

### <a id="itl-https-vhost"></a> https_vhost

Check command object for the `check_http` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
vhost           | **Required.** The name of the virtual host that should be checked.

### <a id="itl-https-ip"></a> https_ip

Check command object for the `check_http` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.

### <a id="itl-smtp"></a> smtp

Check command object for the `check_smtp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.

### <a id="itl-ssmtp"></a> ssmtp

Check command object for the `check_ssmtp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.
port            | **Optional.** The port that should be checked. Defaults to 465.

### <a id="itl-ntp-time"></a> ntp_time

Check command object for the `check_ntp_time` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.

### <a id="itl-ssh"></a> ssh

Check command object for the `check_ssh` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.

### <a id="itl-disk"></a> disk

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
wfree           | **Optional.** The free space warning threshold in %. Defaults to 20.
cfree           | **Optional.** The free space critical threshold in %. Defaults to 10.

### <a id="itl-users"></a> users

Check command object for the `check_disk` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
wgreater        | **Optional.** The user count warning threshold. Defaults to 20.
cgreater        | **Optional.** The user count critical threshold. Defaults to 50.

### <a id="itl-processes"></a> processes

Check command object for the `check_processes` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
wgreater        | **Optional.** The process count warning threshold. Defaults to 250.
cgreater        | **Optional.** The process count critical threshold. Defaults to 400.

### <a id="itl-load"></a> load

Check command object for the `check_load` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
wload1          | **Optional.** The 1-minute warning threshold. Defaults to 5.
wload5          | **Optional.** The 5-minute warning threshold. Defaults to 4.
wload15         | **Optional.** The 15-minute warning threshold. Defaults to 3.
cload1          | **Optional.** The 1-minute critical threshold. Defaults to 10.
cload5          | **Optional.** The 5-minute critical threshold. Defaults to 6.
cload15         | **Optional.** The 15-minute critical threshold. Defaults to 4.

### <a id="itl-snmp"></a> snmp

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.
oid             | **Required.** The SNMP OID.
community       | **Optional.** The SNMP community. Defaults to "public".

### <a id="itl-snmp-uptime"></a> snmp-uptime

Check command object for the `check_snmp` plugin.

Custom Attributes:

Name            | Description
----------------|--------------
address         | **Required.** The host's address.
oid             | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
community       | **Optional.** The SNMP community. Defaults to "public".

### <a id="itl-icinga"></a> icinga

Check command for the built-in `icinga` check. This check returns performance
data for the current Icinga instance.

The `icinga` check command does not support any vars.
