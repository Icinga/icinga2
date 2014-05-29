# <a id="monitoring-remote-systems"></a> Monitoring Remote Systems

## <a id="agent-less-checks"></a> Agent-less Checks

If the remote service is available using a network protocol and port,
and a [check plugin](#setting-up-check-plugins) is available, you don't
necessarily need a local client installed. Rather choose a plugin and
configure all parameters and thresholds. The [Icinga 2 Template Library](#itl)
already ships various examples.

## <a id="agent-based-checks"></a> Agent-based Checks

If the remote services are not directly accessible through the network, a
local agent installation exposing the results to check queries can
become handy.

### <a id="agent-based-checks-snmp"></a> SNMP

The SNMP daemon runs on the remote system and answers SNMP queries by plugin
binaries. The [Monitoring Plugins package](#setting-up-check-plugins) ships
the `check_snmp` plugin binary, but there are plenty of [existing plugins](#integrate-additional-plugins)
for specific use cases already around, for example monitoring Cisco routers.

The following example uses the [SNMP ITL](#itl-snmp) `CheckCommand` and just
overrides the `snmp_oid` custom attribute. A service is created for all hosts which
have the `snmp-community` custom attribute.

    apply Service "uptime" {
      import "generic-service"

      check_command = "snmp"
      vars.snmp_oid = "1.3.6.1.2.1.1.3.0"
  
      assign where host.vars.snmp_community != ""
    }

### <a id="agent-based-checks-ssh"></a> SSH

Calling a plugin using the SSH protocol to execute a plugin on the remote server fetching
its return code and output. The `by_ssh` command object is part of the built-in templates and
requires the `check_by_ssh` check plugin which is available in the [Monitoring Plugins package](#setting-up-check-plugins).

    object CheckCommand "by_ssh_swap" {
      import "by_ssh"

      vars.by_ssh_command = "/usr/lib/nagios/plugins/check_swap -w $by_ssh_swap_warn$ -c $by_ssh_swap_crit$"
      vars.by_ssh_swap_warn = "75%"
      vars.by_ssh_swap_crit = "50%"
    }

    object Service "swap" {
      import "generic-service"

      host_name = "remote-ssh-host"

      check_command = "by_ssh_swap"

      vars.by_ssh_logname = "icinga"
    }

### <a id="agent-based-checks-nrpe"></a> NRPE

[NRPE](http://docs.icinga.org/latest/en/nrpe.html) runs as daemon on the remote client including
the required plugins and command definitions.
Icinga 2 calls the `check_nrpe` plugin binary in order to query the configured command on the
remote client.

The NRPE daemon uses its own configuration format in nrpe.cfg while `check_nrpe`
can be embedded into the Icinga 2 `CheckCommand` configuration syntax.

Example:

    object CheckCommand "check_nrpe" {
      import "plugin-check-command"

      command = [
        PluginDir + "/check_nrpe",
        "-H", "$address$",
        "-c", "$remote_nrpe_command$",
      ]
    }

    object Service "users" {
      import "generic-service"
  
      host_name = "remote-nrpe-host"

      check_command = "check_nrpe"
      vars.remote_nrpe_command = "check_users"
    }

nrpe.cfg:

    command[check_users]=/usr/local/icinga/libexec/check_users -w 5 -c 10

### <a id="agent-based-checks-nsclient"></a> NSClient++

[NSClient++](http://nsclient.org) works on both Windows and Linux platforms and is well
known for its magnificent Windows support. There are alternatives like the WMI interface,
but using `NSClient++` will allow you to run local scripts similar to check plugins fetching
the required output and performance counters.

You can use the `check_nt` plugin from the Monitoring Plugins project to query NSClient++.
Icinga 2 provides the [nscp check command](#plugin-check-command-nscp) for this:

Example:

    object Service "disk" {
      import "generic-service"
  
      host_name = "remote-windows-host"

      check_command = "nscp"

      vars.nscp_variable = "USEDDISKSPACE"
      vars.nscp_params = "c"
      vars.nscp_warn = 70
      vars.nscp_crit = 80
    }

For details on the `NSClient++` configuration please refer to the [official documentation](http://www.nsclient.org/nscp/wiki/doc/configuration/0.4.x).

### <a id="agent-based-checks-icinga2-agent"></a> Icinga 2 Agent

A dedicated Icinga 2 agent supporting all platforms and using the native
Icinga 2 communication protocol supported with SSL certificates, IPv4/IPv6
support, etc. is on the [development roadmap](https://dev.icinga.org/projects/i2?jump=issues).
Meanwhile remote checkers in a [cluster](#distributed-monitoring-high-availability) setup could act as
immediate replacement, but without any local configuration - or pushing
their standalone configuration back to the master node including their check
result messages.

### <a id="agent-based-checks-snmp-traps"></a> Passive Check Results and SNMP Traps

SNMP Traps can be received and filtered by using [SNMPTT](http://snmptt.sourceforge.net/) and specific trap handlers
passing the check results to Icinga 2.

> **Note**
>
> The host and service object configuration must be available on the Icinga 2
> server in order to process passive check results.

### <a id="agent-based-checks-nsca-ng"></a> NSCA-NG

[NSCA-ng](http://www.nsca-ng.org) provides a client-server pair that allows the
remote sender to push check results into the Icinga 2 `ExternalCommandListener`
feature.

The [Icinga 2 Vagrant Demo VM](#vagrant) ships a demo integration and further samples.



## <a id="distributed-monitoring-high-availability"></a> Distributed Monitoring and High Availability

An Icinga 2 cluster consists of two or more nodes and can reside on multiple
architectures. The base concept of Icinga 2 is the possibility to add additional
features using components. In case of a cluster setup you have to add the api feature
to all nodes.

An Icinga 2 cluster can be used for the following scenarios:

* [High Availability](#cluster-scenarios-high-availability). All instances in the `Zone` elect one active master and run as Active/Active cluster.
* [Distributed Zones](#cluster-scenarios-distributed-zones). A master zone and one or more satellites in their zones.
* [Load Distribution](#cluster-scenarios-load-distribution). A configuration master and multiple checker satellites.

Before you start configuring the diffent nodes it is necessary to setup the underlying
communication layer based on SSL.

### <a id="certificate-authority-certificates"></a> Certificate Authority and Certificates

Icinga 2 ships two scripts assisting with CA and node certificate creation
for your Icinga 2 cluster.

The first step is the creation of CA running the following command:

    # icinga2-build-ca

Please make sure to export the environment variable `ICINGA_CA` pointing to
an empty folder for the newly created CA files:

    # export ICINGA_CA="/root/icinga-ca"

Now create a certificate and key file for each node running the following command
(replace `icinga2a` with the required hostname):

    # icinga2-build-key icinga2a

Repeat the step for all nodes in your cluster scenario. Save the CA key in case
you want to set up certificates for additional nodes at a later time.

### <a id="configure-nodename"></a> Configure the Icinga Node Name

Instead of using the default FQDN as node name you can optionally set
that value using the [NodeName](#global-constants) constant.
This setting must be unique for each node, and must also match
the name of the local [Endpoint](#objecttype-endpoint) object and the
SSL certificate common name.

    const NodeName = "icinga2a"

Read further about additional [naming conventions](#cluster-naming-convention).

Not specifying the node name will make Icinga 2 using the FQDN. Make sure that all
configured endpoint names and common names are in sync.

### <a id="cluster-naming-convention"></a> Cluster Naming Convention

The SSL certificate common name (CN) will be used by the [ApiListener](#objecttype-apilistener)
object to determine the local authority. This name must match the local [Endpoint](#objecttype-endpoint)
object name.

Example:

    # icinga2-build-key icinga2a
    ...
    Common Name (e.g. server FQDN or YOUR name) [icinga2a]:

    # vim cluster.conf

    object Endpoint "icinga2a" {
      host = "icinga2a.icinga.org"
    }

The [Endpoint](#objecttype-endpoint) name is further referenced as `endpoints` attribute on the
[Zone](objecttype-zone) object.

    object Endpoint "icinga2b" {
      host = "icinga2b.icinga.org"
    }

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b" ]
    }

Specifying the local node name using the [NodeName](#global-constants) variable requires
the same name as used for the endpoint name and common name above. If not set, the FQDN is used.

    const NodeName = "icinga2a"
    

### <a id="configure-clusterlistener-object"></a> Configure the ApiListener Object

The [ApiListener](#objecttype-apilistener) object needs to be configured on
every node in the cluster with the following settings:

A sample config looks like:

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_config = true
    }

You can simply enable the `api` feature using

    # icinga2-enable-feature api

Edit `/etc/icinga2/features-enabled/api.conf` if you require the configuration
synchronisation enabled.

The certificate files must be readable by the user Icinga 2 is running as. Also,
the private key file must not be world-readable.


### <a id="configure-cluster-endpoints"></a> Configure Cluster Endpoints

`Endpoint` objects specify the `host` and `port` settings for the cluster nodes.
This configuration can be the same on all nodes in the cluster only containing
connection information.

A sample configuration looks like:

    /**
     * Configure config master endpoint
     */

    object Endpoint "icinga2a" {
      host = "icinga2a.icinga.org"
    }

If this endpoint object is reachable on a different port, you must configure the
`ApiListener` on the local `Endpoint` object accordingly too.


### <a id="configure-cluster-zones"></a> Configure Cluster Zones

`Zone` objects specify the endpoints located in a zone. That way your distributed setup can be
seen as zones connected together instead of multiple instances in that specific zone.

Zones can be used for [high availability](#cluster-scenarios-high-availability),
[distributed setups](#cluster-scenarios-distributed-zones) and
[load distribution](#cluster-scenarios-load-distribution).

Each Icinga 2 `Endpoint` must be put into its respective `Zone`. In this example, you will
define the zone `config-ha-master` where the `icinga2a` and `icinga2b` endpoints
are located. The `check-satellite` zone consists of `icinga2c` only, but more nodes could
be added.

The `config-ha-master` zone acts as High-Availability setup - the Icinga 2 instances elect
one active master where all features are running on (for example `icinga2a`). In case of
failure of the `icinga2a` instance, `icinga2b` will take over automatically.

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b" ]
    }

The `check-satellite` zone is a separated location and only sends back their checkresults to
the defined parent zone `config-ha-master`.

    object Zone "check-satellite" {
      endpoints = [ "icinga2c" ]
      parent = "config-ha-master"
    }


#### <a id="cluster-zone-config-sync"></a> Zone Configuration Synchronisation

By default all objects for specific zones should be organized in

    /etc/icinga2/zones.d/<zonename>

These zone packages are then distributed to all nodes in the same zone, and
to their respective target zone instances.

Each configured zone must exist with the same directory name. The parent zone
syncs the configuration to the child zones, if allowed.

    object Zone "master" {
      endpoints = [ "icinga2a" ]
    }

    object Zone "checker" {
      endpoints = [ "icinga2b" ]
      parent = "master"
    }

    /etc/icinga2/zones.d
      master
        health.conf
      checker
        health.conf
        demo.conf

If the local configuration is newer than the received update Icinga 2 will skip the synchronisation
process.

> **Note**
>
> `zones.d` must not be included in [icinga2.conf](#icinga2-conf). Icinga 2 automatically
> determines the required include directory. This can be overridden using the
> [global constant](#global-constants) `ZonesDir`.

#### <a id="zone-synchronisation-permissions"></a> Zone Configuration Permissions

Each [ApiListener](#objecttype-apilistener) object must have the `accept_config` attribute
set to `true` to receive configuration from the parent `Zone` members. Default value is `false`.

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_config = true
    }

### <a id="initial-cluster-sync"></a> Initial Cluster Sync

In order to make sure that all of your cluster nodes have the same state you will
have to pick one of the nodes as your initial "master" and copy its state file
to all the other nodes.

You can find the state file in `/var/lib/icinga2/icinga2.state`. Before copying
the state file you should make sure that all your cluster nodes are properly shut
down.


### <a id="cluster-health-check"></a> Cluster Health Check

The Icinga 2 [ITL](#itl) ships an internal check command checking all configured
`EndPoints` in the cluster setup. The check result will become critical if
one or more configured nodes are not connected.

Example:

    apply Service "cluster" {
        check_command = "cluster"
        check_interval = 5s
        retry_interval = 1s

        assign where host.name == "icinga2a"
    }

Each cluster node should execute its own local cluster health check to
get an idea about network related connection problems from different
points of view.

### <a id="host-multiple-cluster-nodes"></a> Host With Multiple Cluster Nodes

Special scenarios might require multiple cluster nodes running on a single host.
By default Icinga 2 and its features will place their runtime data below the prefix
`LocalStateDir`. By default packages will set that path to `/var`.
You can either set that variable as constant configuration
definition in [icinga2.conf](#icinga2-conf) or pass it as runtime variable to
the Icinga 2 daemon.

    # icinga2 -c /etc/icinga2/node1/icinga2.conf -DLocalStateDir=/opt/node1/var


### <a id="cluster-scenarios"></a> Cluster Scenarios

#### <a id="cluster-scenarios-features"></a> Features in Cluster Zones

Each cluster zone may use available features. If you have multiple locations
or departments, they may write to their local database, or populate graphite.
Even further all commands are distributed.

DB IDO on the left, graphite on the right side - works.
Icinga Web 2 on the left, checker and notifications on the right side - works too.
Everything on the left and on the right side - make sure to deal with duplicated notifications
and automated check distribution.

#### <a id="cluster-scenarios-distributed-zones"></a> Distributed Zones

That scenario fits if your instances are spread over the globe and they all report
to a central instance. Their network connection only works towards the central master
(or the master is able to connect, depending on firewall policies) which means
remote instances won't see each/connect to each other.

All events are synced to the central node, but the remote nodes can still run
local features such as a web interface, reporting, graphing, etc. in their own specified
zone.

Imagine the following example with a central node in Nuremberg, and two remote DMZ
based instances in Berlin and Vienna. The configuration tree on the central instance
could look like this:

    conf.d/
      templates/
    zones.d
      nuremberg/
        local.conf
      berlin/
        hosts.conf
      vienna/
        hosts.conf

The configuration deployment should look like:

* The master node sends `zones.d/berlin` to the `berlin` child zone.
* The master node sends `zones.d/vienna` to the `vienna` child zone.

The endpoint configuration would look like:

    object Endpoint "nuremberg-master" {
      host = "nuremberg.icinga.org"
    }

    object Endpoint "berlin-satellite" {
      host = "berlin.icinga.org"
    }

    object Endpoint "vienna-satellite" {
      host = "vienna.icinga.org"
    }

The zones would look like:

    object Zone "nuremberg" {
      endpoints = [ "nuremberg-master" ]
    }

    object Zone "berlin" {
      endpoints = [ "berlin-satellite" ]
      parent = "nuremberg"
    }

    object Zone "vienna" {
      endpoints = [ "vienna-satellite" ]
      parent = "nuremberg"
    }

The `nuremberg-master` zone will only execute local checks, and receive
check results from the satellite nodes in the zones `berlin` and `vienna`.


#### <a id="cluster-scenarios-load-distribution"></a> Load Distribution

If you are planning to off-load the checks to a defined set of remote workers
you can achieve that by:

* Deploying the configuration on all nodes.
* Let Icinga 2 distribute the load amongst all available nodes.

That way all remote check instances will receive the same configuration
but only execute their part. The central instance can also execute checks,
but you may also disable the `Checker` feature.

    conf.d/
      templates/
    zones.d/
      central/
      checker/

If you are planning to have some checks executed by a specific set of checker nodes
you have to define additional zones and define these check objects there.

Endpoints:

    object Endpoint "central-node" {
      host = "central.icinga.org"
    }

    object Endpoint "checker1-node" {
      host = "checker1.icinga.org"
    }

    object Endpoint "checker2-node" {
      host = "checker2.icinga.org"
    }


Zones:

    object Zone "central" {
      endpoints = [ "central-node" ]
    }

    object Zone "checker" {
      endpoints = [ "checker1-node", "checker2-node" ]
      parent = "central"
    }


#### <a id="cluster-scenarios-high-availability"></a> High Availability

High availability with Icinga 2 is possible by putting multiple nodes into
a dedicated `Zone`. All nodes will elect their active master, and retry an
election once the current active master failed.

Selected features (such as DB IDO) will only be active on the current active master.
All other passive nodes will pause the features without reload/restart.

Connections from other zones will be accepted by all active and passive nodes
but all are forwarded to the current active master dealing with the check results,
commands, etc.

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b", "icinga2c" ]
    }

Two or more nodes in a high availability setup require an [initial cluster sync](#initial-cluster-sync).


#### <a id="cluster-scenarios-multiple-hierachies"></a> Multiple Hierachies

Your central zone collects all check results for reporting and graphing and also
does some sort of additional notifications.
The customers got their own instances in their local DMZ zones. They are limited to read/write
only their services, but replicate all events back to the central instance.
Within each DMZ there are additional check instances also serving interfaces for local
departments. The customers instances will collect all results, but also send them back to
your central instance.
Additionally the customers instance on the second level in the middle prohibits you from
sending commands to the subjacent department nodes. You're only allowed to receive the
results, and a subset of each customers configuration too.

Your central zone will generate global reports, aggregate alert notifications, and check
additional dependencies (for example, the customers internet uplink and bandwidth usage).

The customers zone instances will only check a subset of local services and delegate the rest
to each department. Even though it acts as configuration master with a central dashboard
for all departments managing their configuration tree which is then deployed to all
department instances. Furthermore the central NOC is able to see what's going on.

The instances in the departments will serve a local interface, and allow the administrators
to reschedule checks or acknowledge problems for their services.






