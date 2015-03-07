# <a id="monitoring-remote-systems"></a> Monitoring Remote Systems

## <a id="monitoring-remote-systems-overview"></a> Overview

There's a variety of possibilities to monitor remote servers and services. First off you should
decide how your primary monitoring master is able to reach these hosts and services.

* direct connection querying the service interface (for example `http`), so-called [agent-less checks](9-monitoring-remote-systems.md#agent-less-checks)
* local checks requiring an additional daemon as communication device for your monitoring server

## <a id="agent-less-checks"></a> Agent-less Checks

If the remote service is available using a network protocol and port,
and a [check plugin](2-getting-started.md#setting-up-check-plugins) is available, you don't
necessarily need a local client installed. Rather choose a plugin and
configure all parameters and thresholds. The [Icinga 2 Template Library](7-icinga-template-library.md#icinga-template-library)
already ships various examples like

* [ping4](7-icinga-template-library.md#plugin-check-command-ping4), [ping6](7-icinga-template-library.md#plugin-check-command-ping6),
[fping4](7-icinga-template-library.md#plugin-check-command-fping4), [fping6](7-icinga-template-library.md#plugin-check-command-fping6), [hostalive](7-icinga-template-library.md#plugin-check-command-hostalive)
* [tcp](7-icinga-template-library.md#plugin-check-command-tcp), [udp](7-icinga-template-library.md#plugin-check-command-udp), [ssl](7-icinga-template-library.md#plugin-check-command-ssl)
* [http](7-icinga-template-library.md#plugin-check-command-http), [ftp](7-icinga-template-library.md#plugin-check-command-ftp)
* [smtp](7-icinga-template-library.md#plugin-check-command-smtp), [ssmtp](7-icinga-template-library.md#plugin-check-command-ssmtp),
[imap](7-icinga-template-library.md#plugin-check-command-imap), [simap](7-icinga-template-library.md#plugin-check-command-simap),
[pop](7-icinga-template-library.md#plugin-check-command-pop), [spop](7-icinga-template-library.md#plugin-check-command-spop)
* [ntp_time](7-icinga-template-library.md#plugin-check-command-ntp-time)
* [ssh](7-icinga-template-library.md#plugin-check-command-ssh)
* [dns](7-icinga-template-library.md#plugin-check-command-dns), [dig](7-icinga-template-library.md#plugin-check-command-dig), [dhcp](7-icinga-template-library.md#plugin-check-command-dhcp)

There are numerous check plugins contributed by community members available
on the internet. If you found one for your requirements, [integrate them into Icinga 2](3-monitoring-basics.md#command-plugin-integration).

Start your search at

* [Icinga Exchange](https://exchange.icinga.org)
* [Icinga Wiki](https://wiki.icinga.org)

An example is provided in the sample configuration in the getting started
section shipped with Icinga 2 ([hosts.conf](5-configuring-icinga-2.md#hosts-conf), [services.conf](5-configuring-icinga-2.md#services-conf)).


## <a id="agent-based-checks"></a> Agent-based Checks

If the remote services are not directly accessible through the network, a
local agent installation exposing the results to check queries can
become handy.

Icinga 2 itself can be used as agent (client, satellite) in this scenario, but there
are also a couple of addons available for this task.

The most famous ones are listed below.

## <a id="agent-based-checks-linux-unix"></a> Agent-based Checks for Linux/Unix

The agent runs as daemon and communicates with the master requesting a check being executed
or local stored information (SNMP OID). The Icinga 2 client continues to execute checks
when the connection dies, and does not need the master as check scheduler like the other
listed agents.

* Icinga 2 Client
* SSH
* SNMP
* NRPE

## <a id="agent-based-checks-windows"></a> Agent-based Checks for Windows

The Windows agent runs as administrative service and offers direct plugin execution and/or
local check result being sent to the master instance.

* Icinga 2 Client
* NSClient++

SNMP could also be used, but was deprecated in Windows Server 2012. Alternatively you can
look into the WMI interface.

