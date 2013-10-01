## Icinga Template Library

### Overview

The Icinga Template Library (ITL) implements standard templates and object
definitions for commonly used services.

You can include the ITL by using the *include* directive in your configuration
file:

    include <itl/itl.conf>

### Check Commands

#### ping4

Check command object for the *check_ping* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.
wrta            | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
wpl             | **Optional.** The packet loss warning threshold in %. Defaults to 5.
crta            | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
cpl             | **Optional.** The packet loss critical threshold in %. Defaults to 15.
packets         | **Optional.** The number of packets to send. Defaults to 5.
timeout         | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### ping6

Check command object for the *check_ping* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address6        | **Required.** The host's IPv6 address.
wrta            | **Optional.** The RTA warning threshold in milliseconds. Defaults to 100.
wpl             | **Optional.** The packet loss warning threshold in %. Defaults to 5.
crta            | **Optional.** The RTA critical threshold in milliseconds. Defaults to 200.
cpl             | **Optional.** The packet loss critical threshold in %. Defaults to 15.
packets         | **Optional.** The number of packets to send. Defaults to 5.
timeout         | **Optional.** The plugin timeout in seconds. Defaults to 0 (no timeout).

#### dummy

Check command object for the *check_dummy* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
state           | **Optional.** The state. Can be one of 0 (ok), 1 (warning), 2 (critical) and 3 (unknown). Defaults to 0.
text            | **Optional.** Plugin output. Defaults to "Check was successful.".

#### tcp

Check command object for the *check_tcp* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.
port            | **Required.** The port that should be checked.

#### udp

Check command object for the *check_udp* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.
port            | **Required.** The port that should be checked.

#### http_vhost

Check command object for the *check_http* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
vhost           | **Required.** The name of the virtual host that should be checked.

#### http_ip

Check command object for the *check_http* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.

#### https_vhost

Check command object for the *check_http* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
vhost           | **Required.** The name of the virtual host that should be checked.

#### https_ip

Check command object for the *check_http* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.

#### smtp

Check command object for the *check_smtp* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.

#### ssmtp

Check command object for the *check_ssmtp* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.
port            | **Optional.** The port that should be checked. Defaults to 465.

#### ntp_time

Check command object for the *check_ntp_time* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.

#### ssh

Check command object for the *check_ssh* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.

#### disk

Check command object for the *check_disk* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
wfree           | **Optional.** The free space warning threshold in %. Defaults to 20.
cfree           | **Optional.** The free space critical threshold in %. Defaults to 10.

#### users

Check command object for the *check_disk* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
wgreater        | **Optional.** The user count warning threshold. Defaults to 20.
cgreater        | **Optional.** The user count warning threshold. Defaults to 50.

#### processes

Check command object for the *check_processes* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
wgreater        | **Optional.** The process count warning threshold. Defaults to 250.
cgreater        | **Optional.** The process count warning threshold. Defaults to 400.

#### load

Check command object for the *check_load* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
wload1          | **Optional.** The 1-minute warning threshold. Defaults to 5.
wload5          | **Optional.** The 5-minute warning threshold. Defaults to 4.
wload15         | **Optional.** The 15-minute warning threshold. Defaults to 3.
cload1          | **Optional.** The 1-minute critical threshold. Defaults to 10.
cload5          | **Optional.** The 5-minute critical threshold. Defaults to 6.
cload15         | **Optional.** The 15-minute critical threshold. Defaults to 4.

#### snmp

Check command object for the *check_snmp* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.
oid             | **Required.** The SNMP OID.
community       | **Optional.** The SNMP community. Defaults to "public".

#### snmp-uptime

Check command object for the *check_snmp* plugin.

Macros:

Name            | Description
----------------|--------------
plugindir       | **Required.** The directory containing this plugin.
address         | **Required.** The host's address.
oid             | **Optional.** The SNMP OID. Defaults to "1.3.6.1.2.1.1.3.0".
community       | **Optional.** The SNMP community. Defaults to "public".
