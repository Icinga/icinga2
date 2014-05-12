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
overrides the `oid` custom attribute. A service is created for all hosts which
have the `community` custom attribute.

    apply Service "uptime" {
      import "generic-service"

      check_command = "snmp"
      vars.oid = "1.3.6.1.2.1.1.3.0"
  
      assign where host.vars.community
    }

### <a id="agent-based-checks-ssh"></a> SSH

Calling a plugin using the SSH protocol to execute a plugin on the remote server fetching
its return code and output. `check_by_ssh` is available in the [Monitoring Plugins package](#setting-up-check-plugins).

    object CheckCommand "check_by_ssh_swap" {
      import "plugin-check-command"

      command = [ PluginDir + "/check_by_ssh",
                  "-l", "remoteuser",
                  "-H", "$address$",
                  "-C", "\"/usr/local/icinga/libexec/check_swap -w $warn$ -c $crit$\""
                ]
    }

    object Service "swap" {
      import "generic-service"

      host_name = "remote-ssh-host"

      check_command = "check_by_ssh_swap"
      vars = {
            "warn" = "50%"
            "crit" = "75%"
      }
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
Meanwhile remote checkers in a [Cluster](#distributed-monitoring-high-availability) setup could act as
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

Before you start configuring the diffent nodes it's necessary to setup the underlying
communication layer based on SSL.

### <a id="certificate-authority-certificates"></a> Certificate Authority and Certificates

Icinga 2 comes with two scripts helping you to create CA and node certificates
for your Icinga 2 Cluster.

The first step is the creation of CA using the following command:

    icinga2-build-ca

Please make sure to export a variable containing an empty folder for the created
CA files:

    export ICINGA_CA="/root/icinga-ca"

In the next step you have to create a certificate and a key file for every node
using the following command:

    icinga2-build-key icinga2a

Please create a certificate and a key file for every node in the Icinga 2
cluster and save the CA key in case you want to set up certificates for
additional nodes at a later date.

### <a id="configure-nodename"></a> Configure the Icinga Node Name

Instead of using the default FQDN as node name you can optionally set
that value using the [NodeName](#global-constants) constant.
This setting must be unique on each node, and must also match
the name of the local [Endpoint](#objecttype-endpoint) object and the
SSL certificate common name.

    const NodeName = "icinga2a"

Read further about additional [naming conventions](#cluster-naming-convention).

Not specifying the node name will default to FQDN. Make sure that all
configured endpoint names and set common names are in sync.

### <a id="configure-clusterlistener-object"></a> Configure the ApiListener Object

The ApiListener object needs to be configured on every node in the cluster with the
following settings:

  Configuration Setting    |Value
  -------------------------|------------------------------------
  ca_path                  | path to ca.crt file
  cert_path                | path to server certificate
  key_path                 | path to server key
  bind_port                | port for incoming and outgoing connections. Defaults to `5665`.


A sample config part can look like this:

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
    }

You can simply enable the `api` feature using

    # icinga2-enable-feature api

And edit `/etc/icinga2/features-enabled/api.conf` if you require any changes.

The certificate files must be readable by the user Icinga 2 is running as. Also,
the private key file must not be world-readable.


### <a id="configure-cluster-endpoints"></a> Configure Cluster Endpoints

In addition to the configured port and hostname every endpoint can have specific
abilities to send configuration files to other nodes and limit the hosts allowed
to send configuration files.

  Configuration Setting    |Value
  -------------------------|------------------------------------
  host                     | hostname
  port                     | port



A sample config part can look like this:

    /**
     * Configure config master endpoint
     */

    object Endpoint "icinga2a" {
      host = "icinga2a.localdomain"
      port = 5665
    }


### <a id="configure-cluster-zones"></a> Configure Cluster Zones

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

The `check-satellite` zone is a seperated location and only sends back their checkresults to
the defined parent zone `config-ha-master`.

    object Zone "check-satellite" {
      endpoints = [ "icinga2c" ]
      parent = "config-ha-master"
    }

TODO - FIXME

Additional permissions for configuration/status sync and remote commands.


### <a id="cluster-naming-convention"></a> Cluster Naming Convention

The SSL certificate common name (CN) will be used by the [ApiListener](pbjecttype-apilistener)
object to determine the local authority. This name must match the local [Endpoint](#objecttype-endpoint)
object name.

Example:

    # icinga2-build-key icinga2a
    ...
    Common Name (e.g. server FQDN or YOUR name) [icinga2a]:

    # vim cluster.conf

    object Endpoint "icinga2a" {
      host = "icinga2a.localdomain"
      port = 5665
    }

The [Endpoint](#objecttype-endpoint) name is further referenced as `endpoints` attribute on the
[Zone](objecttype-zone) object.

    object Endpoint "icinga2b" {
      host = "icinga2b.localdomain"
      port = 5665
    }

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b" ]
    }

Specifying the local node name using the [NodeName](#global-constants) variable requires
the same name as used for the endpoint name and common name above. If not set, the FQDN is used.

    const NodeName = "icinga2a"


### <a id="initial-cluster-sync"></a> Initial Cluster Sync

In order to make sure that all of your cluster nodes have the same state you will
have to pick one of the nodes as your initial "master" and copy its state file
to all the other nodes.

You can find the state file in `/var/lib/icinga2/icinga2.state`. Before copying
the state file you should make sure that all your cluster nodes are properly shut
down.


### <a id="object-configuration-for-zones"></a> Object Configuration for Zones

TODO - FIXME

By default all objects for specific zones should be organized in

    /etc/icinga2/zones.d/<zonename>

These zone packages are then distributed to all nodes in the same zone, and
to their respective target zone instances.


### <a id="cluster-health-check"></a> Cluster Health Check

The Icinga 2 [ITL](#itl) ships an internal check command checking all configured
`EndPoints` in the cluster setup. The check result will become critical if
one or more configured nodes are not connected.

Example:

    apply Service "cluster" {
        import "generic-service"

        check_interval = 1m
        check_command = "cluster"

        assign where host.name = "icinga2a"
    }

Each cluster node should execute its own local cluster health check to
get an idea about network related connection problems from different
point of views.

### <a id="host-multiple-cluster-nodes"></a> Host With Multiple Cluster Nodes

Special scenarios might require multiple cluster nodes running on a single host.
By default Icinga 2 and its features will drop their runtime data below the prefix
`LocalStateDir`. By default packages will set that path to `/var`.
You can either set that variable as constant configuration
definition in [icinga2.conf](#icinga2-conf) or pass it as runtime variable to
the Icinga 2 daemon.

    # icinga2 -c /etc/icinga2/node1/icinga2.conf -DLocalStateDir=/opt/node1/var


### <a id="cluster-scenarios"></a> Cluster Scenarios

#### <a id="cluster-scenarios-features"></a> Features in Cluster Zones

Each cluster zone may use available features. If you have multiple locations
or departments, they may write to their local database, or populate graphite.
Even further all commands are distributed (unless prohibited using [Domains](#domains)).

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
        hosts.conf
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
      port = 5665
    }

    object Endpoint "berlin-satellite" {
      host = "berlin.icinga.org"
      port = 5665
    }

    object Endpoint "vienna-satellite" {
      host = "vienna.icinga.org"
      port = 5665
    }

The zones would look like:

    object Zone "nuremberg" {
      endpoints = [ "nuremberg-master" ]
    }

    object Zone "berlin" {
      endpoints = [ "berlin-satellite" ]
      parent = "nuremberg-master"
    }

    object Zone "vienna" {
      endpoints = [ "vienna-satellite" ]
      parent = "nuremberg-master"
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
      many/

If you are planning to have some checks executed by a specific set of checker nodes
you have to define additional zones and define these check objects there.

Endpoints:

    object Endpoint "central" {
      host = "central.icinga.org"
      port = 5665
    }

    object Endpoint "checker1" {
      host = "checker1.icinga.org"
      port = 5665
    }

    object Endpoint "checker2" {
      host = "checker2.icinga.org"
      port = 5665
    }


Zones:

    object Zone "master" {
      endpoints = [ "central" ]
    }

    object Zone "many" {
      endpoints = [ "checker1", "checker2" ]
      parent = "master"
    }


#### <a id="cluster-scenarios-high-availability"></a> High Availability

High availability with Icinga 2 is possible by putting multiple nodes into
a dedicated `Zone`. All nodes will elect their active master, and retry an
election once the current active master failed.

Features such as DB IDO will only be active on the current active master.
All other passive nodes will pause the features without reload/restart.

Connections from other zones will be accepted by all active and passive nodes
but all are forwarded to the current active master dealing with the check results,
commands, etc.

    object Zone "ha-master" {
      endpoints = [ "icinga2a", "icinga2b", "icinga2c" ]
    }

TODO - FIXME

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
sending commands to the down below department nodes. You're only allowed to receive the
results, and a subset of each customers configuration too.

Your central zone will generate global reports, aggregate alert notifications and check
additional dependencies (for example, the customers internet uplink and bandwidth usage).

The customers zone instances will only check a subset of local services and delegate the rest
to each department. Even though it acts as configuration master with a central dashboard
for all departments managing their configuration tree which is then deployed to all
department instances. Furthermore the central NOC is able to see what's going on.

The instances in the departments will serve a local interface, and allow the administrators
to reschedule checks or acknowledge problems for their services.



### <a id="zones"></a> Zones

`Zone` objects specify the endpoints located in a zone, and additional restrictions. That
way your distributed setup can be seen as zones connected together instead of multiple
instances in that specific zone.

Zones can be used for [high availability](#cluster-scenarios-high-availability),
[distributed setups](#cluster-scenarios-distributed-zones) and
[load distribution](#cluster-scenarios-load-distribution).

### <a id="zone-synchronisation"></a> Zone Synchronisation

TODO - FIXME

### <a id="zone-permissions"></a> Zone Permissions

TODO - FIXME

