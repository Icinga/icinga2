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

The NSClient++ agent uses its own configuration format while `check_nt`
can be embedded into the Icinga 2 `CheckCommand` configuration syntax.

Example:

    object CheckCommand "check_nscp" {
      import "plugin-check-command"

      command = [
        PluginDir + "/check_nt",
        "-H", "$address$",
        "-p", "$port$",
        "-v", "$remote_nscp_command$",
        "-l", "$partition$",
        "-w", "$warn$",
        "-c", "$crit$",
        "-s", "$pass$"
      ]

      vars = {
        "port" = "12489"
        "pass" = "supersecret"
      }
    }

    object Service "users" {
      import "generic-service"
  
      host_name = "remote-windows-host"

      check_command = "check_nscp"

      vars += {
        remote_nscp_command = "USEDDISKSPACE"
        partition = "c"
        warn = "70"
        crit = "80"
      }
    }

For details on the `NSClient++` configuration please refer to the [official documentation](http://www.nsclient.org/nscp/wiki/doc/configuration/0.4.x).

> **Note**
> 
> The format of the `NSClient++` configuration file has changed from 0.3.x to 0.4!

### <a id="agent-based-checks-icinga2-agent"></a> Icinga 2 Agent

A dedicated Icinga 2 agent supporting all platforms and using the native
Icinga 2 communication protocol supported with SSL certificates, IPv4/IPv6
support, etc. is on the [development roadmap](https://dev.icinga.org/projects/i2?jump=issues).
Meanwhile remote checkers in a [Cluster](#cluster) setup could act as
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



## <a id="distributed-monitoring"></a> Distributed Monitoring

An Icinga 2 cluster consists of two or more nodes and can reside on multiple
architectures. The base concept of Icinga 2 is the possibility to add additional
features using components. In case of a cluster setup you have to add the
cluster feature to all nodes. Before you start configuring the diffent nodes
it's necessary to setup the underlying communication layer based on SSL.

### <a id="certificate-authority-certificates"></a> Certificate Authority and Certificates

Icinga 2 comes with two scripts helping you to create CA and node certificates
for you Icinga 2 Cluster.

The first step is the creation of CA using the following command:

    icinga2-build-ca

Please make sure to export a variable containing an empty folder for the created
CA files:

    export ICINGA_CA="/root/icinga-ca"

In the next step you have to create a certificate and a key file for every node
using the following command:

    icinga2-build-key icinga2a

Please create a certificate and a key file for every node in the Icinga 2
Cluster and save the CA key in case you want to set up certificates for
additional nodes at a later date.

### <a id="enable-cluster-configuration"></a> Enable the Cluster Configuration

Until the cluster-component is moved into an independent feature you have to
enable the required libraries in the icinga2.conf configuration file:

    library "cluster"

### <a id="configure-nodename"></a> Configure the Icinga Node Name

Instead of using the default FQDN as node name you can optionally set
that value using the [NodeName](#global-constants) constant.
This setting must be unique on each cluster node, and must also match
the name of the local [Endpoint](#objecttype-endpoint) object and the
SSL certificate common name.

    const NodeName = "icinga2a"

Read further about additional [naming conventions](#cluster-naming-convention).

Not specifying the node name will default to FQDN. Make sure that all
configured endpoint names and set common names are in sync.

### <a id="configure-clusterlistener-object"></a> Configure the ClusterListener Object

The ClusterListener needs to be configured on every node in the cluster with the
following settings:

  Configuration Setting    |Value
  -------------------------|------------------------------------
  ca_path                  | path to ca.crt file
  cert_path                | path to server certificate
  key_path                 | path to server key
  bind_port                | port for incoming and outgoing conns
  peers                    | array of all reachable nodes
  ------------------------- ------------------------------------

A sample config part can look like this:

    /**
     * Load cluster library and configure ClusterListener using certificate files
     */
    library "cluster"

    object ClusterListener "cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt"
      cert_path = "/etc/icinga2/ca/icinga2a.crt"
      key_path = "/etc/icinga2/ca/icinga2a.key"

      bind_port = 8888

      peers = [ "icinga2b" ]
    }

The certificate files must be readable by the user Icinga 2 is running as. Also,
the private key file should not be world-readable.

Peers configures the direction used to connect multiple nodes together. If have
a three node cluster consisting of

* node-1
* node-2
* node-3

and `node-3` is only reachable from `node-2`, you have to consider this in your
peer configuration.

### <a id="configure-cluster-endpoints"></a> Configure Cluster Endpoints

In addition to the configured port and hostname every endpoint can have specific
abilities to send configuration files to other nodes and limit the hosts allowed
to send configuration files.

  Configuration Setting    |Value
  -------------------------|------------------------------------
  host                     | hostname
  port                     | port
  accept_config            | all nodes allowed to send configuration
  config_files             | all files sent to that node - MUST BE AN ABSOLUTE PATH
  config_files_recursive   | all files in a directory recursively sent to that node
  ------------------------- ------------------------------------

A sample config part can look like this:

    /**
     * Configure config master endpoint
     */

    object Endpoint "icinga2a" {
      host = "icinga2a.localdomain"
      port = 8888
      config_files_recursive = ["/etc/icinga2/conf.d"]
    }

If you update the configuration files on the configured file sender, it will
force a restart on all receiving nodes after validating the new config.

A sample config part for a config receiver endpoint can look like this:

    /**
     * Configure config receiver endpoint
     */

    object Endpoint "icinga2b" {
      host = "icinga2b.localdomain"
      port = 8888
      accept_config = [ "icinga2a" ]
    }

By default these configuration files are saved in /var/lib/icinga2/cluster/config.

In order to load configuration files which were received from a remote Icinga 2
instance you will have to add the following include directive to your
`icinga2.conf` configuration file:

    include_recursive LocalStateDir + "/lib/icinga2/cluster/config"

### <a id="cluster-naming-convention"></a> Cluster Naming Convention

The SSL certificate common name (CN) will be used by the [ClusterListener](pbjecttype-clusterlistener)
object to determine the local authority. This name must match the local [Endpoint](#objecttype-endpoint)
object name.

Example:

    # icinga2-build-key icinga2a
    ...
    Common Name (e.g. server FQDN or YOUR name) [icinga2a]:

    # vim cluster.conf

    object Endpoint "icinga2a" {
      host = "icinga2a.localdomain"
      port = 8888
    }

The [Endpoint](#objecttype-endpoint) name is further referenced as `peers` attribute on the
[ClusterListener](pbjecttype-clusterlistener) object.

    object Endpoint "icinga2b" {
      host = "icinga2b.localdomain"
      port = 8888
    }

    object ClusterListener "cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt"
      cert_path = "/etc/icinga2/ca/icinga2a.crt"
      key_path = "/etc/icinga2/ca/icinga2a.key"

      bind_port = 8888

      peers = [ "icinga2b" ]
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


### <a id="assign-services-to-cluster-nodes"></a> Assign Services to Cluster Nodes

By default all services are distributed among the cluster nodes with the `Checker`
feature enabled.
If you require specific services to be only executed by one or more checker nodes
within the cluster, you must define `authorities` as additional service object
attribute. Required Endpoints must be defined as array.

    apply Service "dmz-oracledb" {
      import "generic-service"

      authorities = [ "icinga2a" ]

      assign where "oracle" in host.groups
    }

The most common use case is building a master-slave cluster. The master node
does not have the `checker` feature enabled, and the slave nodes are checking
services based on their location, inheriting from a global service template
defining the authorities.

### <a id="cluster-health-check"></a> Cluster Health Check

The Icinga 2 [ITL](#itl) ships an internal check command checking all configured
`EndPoints` in the cluster setup. The check result will become critical if
one or more configured nodes are not connected.

Example:

    apply Service "cluster" {
        import "generic-service"

        check_interval = 1m
        check_command = "cluster"
        authorities = [ "icinga2a" ]

        assign where host.name = "icinga2a"
    }

Each cluster node should execute its own local cluster health check to
get an idea about network related connection problems from different
point of views. Use the `authorities` attribute to assign the service
check to the configured node.

### <a id="host-multiple-cluster-nodes"></a> Host With Multiple Cluster Nodes

Special scenarios might require multiple cluster nodes running on a single host.
By default Icinga 2 and its features will drop their runtime data below the prefix
`LocalStateDir`. By default packages will set that path to `/var`.
You can either set that variable as constant configuration
definition in [icinga2.conf](#icinga2-conf) or pass it as runtime variable to
the Icinga 2 daemon.

    # icinga2 -c /etc/icinga2/node1/icinga2.conf -DLocalStateDir=/opt/node1/var


### <a id="cluster-scenarios"></a> Cluster Scenarios

#### <a id="cluster-scenarios-features"></a> Features in Cluster

Each cluster instance may use available features. If you have multiple locations
or departments, they may write to their local database, or populate graphite.
Even further all commands are distributed (unless prohibited using [Domains](#domains)).

DB IDO on the left, graphite on the right side - works.
Icinga Web 2 on the left, checker and notifications on the right side - works too.
Everything on the left and on the right side - make sure to deal with duplicated notifications
and automated check distribution.

#### <a id="cluster-scenarios-location-based"></a> Location Based Cluster

That scenario fits if your instances are spread over the globe and they all report
to a central instance. Their network connection only works towards the central master
(or the master is able to connect, depending on firewall policies) which means
remote instances won't see each/connect to each other.

All events are synced to the central node, but the remote nodes can still run
local features such as a web interface, reporting, graphing, etc.

Imagine the following example with a central node in Nuremberg, and two remote DMZ
based instances in Berlin and Vienna. The configuration tree on the central instance
could look like this:

    conf.d/
      templates/
      germany/
        nuremberg/
          hosts.conf
        berlin/
          hosts.conf
      austria/
        vienna/
          hosts.conf

The configuration deployment should look like:

* The node `nuremberg` sends `conf.d/germany/berlin` to the `berlin` node.
* The node `nuremberg` sends `conf.d/austria/vienna` to the `vienna` node.

`conf.d/templates` is shared on all nodes.

The endpoint configuration on the `nuremberg` node would look like:

    object Endpoint "nuremberg" {
      host = "nuremberg.icinga.org"
      port = 8888
    }

    object Endpoint "berlin" {
      host = "berlin.icinga.org"
      port = 8888
      config_files_recursive = [ "/etc/icinga2/conf.d/templates",
                                 "/etc/icinga2/conf.d/germany/berlin" ]
    }

    object Endpoint "vienna" {
      host = "vienna.icinga.org"
      port = 8888
      config_files_recursive = [ "/etc/icinga2/conf.d/templates",
                                 "/etc/icinga2/conf.d/austria/vienna" ]
    }

Each remote node will only peer with the central `nuremberg` node. Therefore
only two endpoints are required for cluster connection. Furthermore the remote
node must include the received configuration by the cluster functionality.

Example for the configuration on the `berlin` node:

    object Endpoint "nuremberg" {
      host = "nuremberg.icinga.org"
      port = 8888
    }

    object Endpoint "berlin" {
      host = "berlin.icinga.org"
      port = 8888
      accept_config = [ "nuremberg" ]
    }

    include_recursive LocalStateDir + "/lib/icinga2/cluster/config"

Depenending on the network connectivity the connections can be either
established by the remote node or the central node.

Example for `berlin` node connecting to central `nuremberg` node:

    library "cluster"

    object ClusterListener "berlin-cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt"
      cert_path = "/etc/icinga2/ca/berlin.crt"
      key_path = "/etc/icinga2/ca/berlin.key"
      bind_port = 8888
      peers = [ "nuremberg" ]
    }

Example for central `nuremberg` node connecting to remote nodes:

    library "cluster"

    object ClusterListener "nuremberg-cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt"
      cert_path = "/etc/icinga2/ca/nuremberg.crt"
      key_path = "/etc/icinga2/ca/nuremberg.key"
      bind_port = 8888
      peers = [ "berlin", "vienna" ]
    }

The central node should not do any checks by itself. There's two possibilities to achieve
that:

* Disable the `checker` feature
* Pin the service object configuration to the remote endpoints using the [authorities](#assign-services-to-cluster-nodes)
attribute.


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
      many/

If you are planning to have some checks executed by a specific set of checker nodes
just pin them using the [authorities](#assign-services-to-cluster-nodes) attribute.

Example on the `central` node:

    object Endpoint "central" {
      host = "central.icinga.org"
      port = 8888
    }

    object Endpoint "checker1" {
      host = "checker1.icinga.org"
      port = 8888
      config_files_recursive = [ "/etc/icinga2/conf.d" ]
    }

    object Endpoint "checker2" {
      host = "checker2.icinga.org"
      port = 8888
      config_files_recursive = [ "/etc/icinga2/conf.d" ]
    }

    object ClusterListener "central-cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt"
      cert_path = "/etc/icinga2/ca/central.crt"
      key_path = "/etc/icinga2/ca/central.key"
      bind_port = 8888
      peers = [ "checker1", "checker2" ]
    }

Example on `checker1` node:

    object Endpoint "central" {
      host = "central.icinga.org"
      port = 8888
    }

    object Endpoint "checker1" {
      host = "checker1.icinga.org"
      port = 8888
      accept_config = [ "central" ]
    }

    object Endpoint "checker2" {
      host = "checker2.icinga.org"
      port = 8888
      accept_config = [ "central" ]
    }

    object ClusterListener "checker1-cluster" {
      ca_path = "/etc/icinga2/ca/ca.crt"
      cert_path = "/etc/icinga2/ca/checker1.crt"
      key_path = "/etc/icinga2/ca/checker1.key"
      bind_port = 8888
    }


#### <a id="cluster-scenarios-high-availability"></a> High Availability

Two nodes in a high availability setup require an [initial cluster sync](#initial-cluster-sync).
Furthermore the active master node should deploy the configuration to the
second node, if that does not already happen by your provisioning tool. It primarly
depends which features are enabled/used. It is still required that some failover
mechanism detects for example which instance will be the notification "master".


#### <a id="cluster-scenarios-multiple-hierachies"></a> Multiple Hierachies

Your central instance collects all check results for reporting and graphing and also
does some sort of additional notifications.
The customers got their own instances in their local DMZs. They are limited to read/write
only their services, but replicate all events back to the central instance.
Within each DMZ there are additional check instances also serving interfaces for local
departments. The customers instances will collect all results, but also send them back to
your central instance.
Additionally the customers instance on the second level in the middle prohibits you from
sending commands to the down below department nodes. You're only allowed to receive the
results, and a subset of each customers configuration too.

Your central instance will generate global reports, aggregate alert notifications and check
additional dependencies (for example, the customers internet uplink and bandwidth usage).

The customers instance will only check a subset of local services and delegate the rest
to each department. Even though it acts as configuration master with a central dashboard
for all departments managing their configuration tree which is then deployed to all
department instances. Furthermore the central NOC is able to see what's going on.

The instances in the departments will serve a local interface, and allow the administrators
to reschedule checks or acknowledge problems for their services.



### <a id="domains"></a> Domains

A [Service](#objecttype-service) object can be restricted using the `domains` attribute
array specifying endpoint privileges.
A Domain object specifices the ACLs applied for each [Endpoint](#objecttype-endpoint).

The following example assigns the domain `dmz-db` to the service `dmz-oracledb`. Endpoint
`icinga-node-dmz-1` does not allow any object modification (no commands, check results) and only
relays local messages to the remote node(s). The endpoint `icinga-node-dmz-2` processes all
messages read and write (accept check results, commands and also relay messages to remote
nodes).

That way the service `dmz-oracledb` on endpoint `icinga-node-dmz-1` will not be modified
by any cluster event message, and could be checked by the local authority too presenting
a different state history. `icinga-node-dmz-2` still receives all cluster message updates
from the `icinga-node-dmz-1` endpoint.

    object Host "dmz-host1" {
      import "generic-host"
    }

    object Service "dmz-oracledb" {
      import "generic-service"

      host_name = "dmz-host1"

      domains = [ "dmz-db" ]
      authorities = [ "icinga-node-dmz-1", "icinga-node-dmz-2"]
    }

    object Domain "dmz-db" {
      acl = {
        "icinga-node-dmz-1" = DomainPrivReadOnly
        "icinga-node-dmz-2" = DomainPrivReadWrite
      }
    }

