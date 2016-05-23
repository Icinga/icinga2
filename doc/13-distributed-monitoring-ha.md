# <a id="distributed-monitoring-high-availability"></a> Distributed Monitoring and High Availability

Building distributed environments with high availability included is fairly easy with Icinga 2.
The cluster feature is built-in and allows you to build many scenarios based on your requirements:

* [High Availability](13-distributed-monitoring-ha.md#cluster-scenarios-high-availability). All instances in the `Zone` run as Active/Active cluster.
* [Distributed Zones](13-distributed-monitoring-ha.md#cluster-scenarios-distributed-zones). A master zone and one or more satellites in their zones.
* [Load Distribution](13-distributed-monitoring-ha.md#cluster-scenarios-load-distribution). A configuration master and multiple checker satellites.

You can combine these scenarios into a global setup fitting your requirements.

Each instance got their own event scheduler, and does not depend on a centralized master
coordinating and distributing the events. In case of a cluster failure, all nodes
continue to run independently. Be alarmed when your cluster fails and a Split-Brain-scenario
is in effect -- all alive instances continue to do their job, and history will begin to differ.


## <a id="cluster-requirements"></a> Cluster Requirements

Before you start deploying, keep the following things in mind:

Your [SSL CA and certificates](13-distributed-monitoring-ha.md#manual-certificate-generation) are mandatory for secure communication.

Communication between zones requires one of these connection directions:

* The parent zone nodes are able to connect to the child zone nodes (`parent => child`).
* The child zone nodes are able to connect to the parent zone nodes (`parent <= child`).
* Both connnection directions work.

Update firewall rules and ACLs.

* Icinga 2 master, satellite and client instances communicate using the default tcp port `5665`.

Get pen and paper or a drawing board and design your nodes and zones!

* Keep the [naming convention](13-distributed-monitoring-ha.md#cluster-naming-convention) for nodes in mind.
* All nodes (endpoints) in a cluster zone provide high availability functionality and trust each other.
* Cluster zones can be built in a Top-Down-design where the child trusts the parent.

Decide whether to use the built-in [configuration syncronization](13-distributed-monitoring-ha.md#cluster-zone-config-sync) or use an external tool (Puppet, Ansible, Chef, Salt, etc.) to manage the configuration deployment.


> **Tip**
>
> If you're looking for troubleshooting cluster problems, check the general
> [troubleshooting](16-troubleshooting.md#troubleshooting-cluster) section.

## <a id="manual-certificate-generation"></a> Manual SSL Certificate Generation

Icinga 2 provides [CLI commands](8-cli-commands.md#cli-command-pki) assisting with CA
and node certificate creation for your Icinga 2 distributed setup.

You can also use the master and client setup wizards to install the cluster nodes
using CSR-Autosigning.

The manual steps are helpful if you want to use your own and/or existing CA (for example
Puppet CA).

You're free to use your own method to generated a valid ca and signed client
certificates.

The first step is the creation of the certificate authority (CA) by running the
following command:

    # icinga2 pki new-ca

Now create a certificate and key file for each node running the following command
(replace `icinga2a` with the required hostname):

    # icinga2 pki new-cert --cn icinga2a --key icinga2a.key --csr icinga2a.csr
    # icinga2 pki sign-csr --csr icinga2a.csr --cert icinga2a.crt

Repeat the step for all nodes in your cluster scenario.

Save the CA key in a secure location in case you want to set up certificates for
additional nodes at a later time.

Navigate to the location of your newly generated certificate files, and manually
copy/transfer them to `/etc/icinga2/pki` in your Icinga 2 configuration folder.

> **Note**
>
> The certificate files must be readable by the user Icinga 2 is running as. Also,
> the private key file must not be world-readable.

Each node requires the following files in `/etc/icinga2/pki` (replace `fqdn-nodename` with
the host's FQDN):

* ca.crt
* &lt;fqdn-nodename&gt;.crt
* &lt;fqdn-nodename&gt;.key

If you're planning to use your existing CA and certificates, please note that you *must not*
use wildcard certificates. The common name (CN) is mandatory for the cluster communication and
therefore must be unique for each connecting instance.

## <a id="cluster-naming-convention"></a> Cluster Naming Convention

The SSL certificate common name (CN) will be used by the [ApiListener](6-object-types.md#objecttype-apilistener)
object to determine the local authority. This name must match the local [Endpoint](6-object-types.md#objecttype-endpoint)
object name.

Certificate generation for host with the FQDN `icinga2a`:

    # icinga2 pki new-cert --cn icinga2a --key icinga2a.key --csr icinga2a.csr
    # icinga2 pki sign-csr --csr icinga2a.csr --cert icinga2a.crt

Add a new `Endpoint` object named `icinga2a`:

    # vim zones.conf

    object Endpoint "icinga2a" {
      host = "icinga2a.icinga.org"
    }

The [Endpoint](6-object-types.md#objecttype-endpoint) name is further referenced as `endpoints` attribute on the
[Zone](6-object-types.md#objecttype-zone) object.

    object Endpoint "icinga2b" {
      host = "icinga2b.icinga.org"
    }

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b" ]
    }

Specifying the local node name using the [NodeName](13-distributed-monitoring-ha.md#configure-nodename) variable requires
the same name as used for the endpoint name and common name above. If not set, the FQDN is used.

    const NodeName = "icinga2a"

If you're using the host's FQDN everywhere, you're on the safe side. The setup wizards
will do the very same.

## <a id="cluster-configuration"></a> Cluster Configuration

The following section describe which configuration must be updated/created
in order to get your cluster running with basic functionality.

* [configure the node name](13-distributed-monitoring-ha.md#configure-nodename)
* [configure the ApiListener object](13-distributed-monitoring-ha.md#configure-apilistener-object)
* [configure cluster endpoints](13-distributed-monitoring-ha.md#configure-cluster-endpoints)
* [configure cluster zones](13-distributed-monitoring-ha.md#configure-cluster-zones)

Once you're finished with the basic setup the following section will
describe how to use [zone configuration synchronisation](13-distributed-monitoring-ha.md#cluster-zone-config-sync)
and configure [cluster scenarios](13-distributed-monitoring-ha.md#cluster-scenarios).

### <a id="configure-nodename"></a> Configure the Icinga Node Name

Instead of using the default FQDN as node name you can optionally set
that value using the [NodeName](18-language-reference.md#constants) constant.

> ** Note **
>
> Skip this step if your FQDN already matches the default `NodeName` set
> in `/etc/icinga2/constants.conf`.

This setting must be unique for each node, and must also match
the name of the local [Endpoint](6-object-types.md#objecttype-endpoint) object and the
SSL certificate common name as described in the
[cluster naming convention](13-distributed-monitoring-ha.md#cluster-naming-convention).

    vim /etc/icinga2/constants.conf

    /* Our local instance name. By default this is the server's hostname as returned by `hostname --fqdn`.
     * This should be the common name from the API certificate.
     */
    const NodeName = "icinga2a"


Read further about additional [naming conventions](13-distributed-monitoring-ha.md#cluster-naming-convention).

Not specifying the node name will make Icinga 2 using the FQDN. Make sure that all
configured endpoint names and common names are in sync.

### <a id="configure-apilistener-object"></a> Configure the ApiListener Object

The [ApiListener](6-object-types.md#objecttype-apilistener) object needs to be configured on
every node in the cluster with the following settings:

A sample config looks like:

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_config = true
      accept_commands = true
    }

You can simply enable the `api` feature using

    # icinga2 feature enable api

Edit `/etc/icinga2/features-enabled/api.conf` if you require the configuration
synchronisation enabled for this node. Set the `accept_config` attribute to `true`.

If you want to use this node as [remote client for command execution](11-icinga2-client.md#icinga2-client-configuration-command-bridge),
set the `accept_commands` attribute to `true`.

> **Note**
>
> The certificate files must be readable by the user Icinga 2 is running as. Also,
> the private key file must not be world-readable.

### <a id="configure-cluster-endpoints"></a> Configure Cluster Endpoints

`Endpoint` objects specify the `host` and `port` settings for the cluster node
connection information.
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

If you don't want the local instance to connect to the remote instance, remove the
`host` attribute locally. Keep in mind that the configuration is now different amongst
all instances and point-of-view dependant.

### <a id="configure-cluster-zones"></a> Configure Cluster Zones

`Zone` objects specify the endpoints located in a zone. That way your distributed setup can be
seen as zones connected together instead of multiple instances in that specific zone.

Zones can be used for [high availability](13-distributed-monitoring-ha.md#cluster-scenarios-high-availability),
[distributed setups](13-distributed-monitoring-ha.md#cluster-scenarios-distributed-zones) and
[load distribution](13-distributed-monitoring-ha.md#cluster-scenarios-load-distribution).
Furthermore zones are used for the [Icinga 2 remote client](11-icinga2-client.md#icinga2-client).

Each Icinga 2 `Endpoint` must be put into its respective `Zone`. In this example, you will
define the zone `config-ha-master` where the `icinga2a` and `icinga2b` endpoints
are located. The `check-satellite` zone consists of `icinga2c` only, but more nodes could
be added.

The `config-ha-master` zone acts as High-Availability setup -- the Icinga 2 instances elect
one instance running a check, notification or feature (DB IDO), for example `icinga2a`. In case of
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


## <a id="cluster-zone-config-sync"></a> Zone Configuration Synchronisation

In case you are using the Icinga 2 API for creating, modifying and deleting objects
at runtime, please continue over [here](9-icinga2-api.md#icinga2-api-config-objects-cluster-sync).

By default all objects for specific zones should be organized in

    /etc/icinga2/zones.d/<zonename>

on the configuration master.

Your child zones and endpoint members **must not** have their config copied to `zones.d`.
The built-in configuration synchronisation takes care of that if your nodes accept
configuration from the parent zone. You can define that in the
[ApiListener](13-distributed-monitoring-ha.md#configure-apilistener-object) object by configuring the `accept_config`
attribute accordingly.

You should remove the sample config included in `conf.d` by commenting the `recursive_include`
statement in [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf):

    //include_recursive "conf.d"

This applies to any other non-used configuration directories as well (e.g. `repository.d`
if not used).

Better use a dedicated directory name for local configuration like `local` or similar, and
include that one if your nodes require local configuration not being synced to other nodes. That's
useful for local [health checks](13-distributed-monitoring-ha.md#cluster-health-check) for example.

> **Note**
>
> In a [high availability](13-distributed-monitoring-ha.md#cluster-scenarios-high-availability)
> setup only one assigned node can act as configuration master. All other zone
> member nodes **must not** have the `/etc/icinga2/zones.d` directory populated.


These zone packages are then distributed to all nodes in the same zone, and
to their respective target zone instances.

Each configured zone must exist with the same directory name. The parent zone
syncs the configuration to the child zones if allowed using the `accept_config`
attribute of the [ApiListener](13-distributed-monitoring-ha.md#configure-apilistener-object) object.

Config on node `icinga2a`:

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

Config on node `icinga2b`:

    object Zone "master" {
      endpoints = [ "icinga2a" ]
    }

    object Zone "checker" {
      endpoints = [ "icinga2b" ]
      parent = "master"
    }

    /etc/icinga2/zones.d
      EMPTY_IF_CONFIG_SYNC_ENABLED

If the local configuration is newer than the received update, Icinga 2 will skip the synchronisation
process.

> **Note**
>
> `zones.d` must not be included in [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf). Icinga 2 automatically
> determines the required include directory. This can be overridden using the
> [global constant](18-language-reference.md#constants) `ZonesDir`.

### <a id="zone-global-config-templates"></a> Global Configuration Zone for Templates

If your zone configuration setup shares the same templates, groups, commands, timeperiods, etc.,
you would have to duplicate quite a lot of configuration objects making the merged configuration
on your configuration master unique.

> ** Note **
>
> Only put templates, groups, etc. into this zone. DO NOT add checkable objects such as
> hosts or services here. If they are checked by all instances globally, this will lead
> into duplicated check results and unclear state history. Not easy to troubleshoot too -
> you have been warned.

That is not necessary by defining a global zone shipping all those templates. By setting
`global = true` you ensure that this zone serving common configuration templates will be
synchronized to all involved nodes (only if they accept configuration though).

Config on configuration master:

    /etc/icinga2/zones.d
      global-templates/
        templates.conf
        groups.conf
      master
        health.conf
      checker
        health.conf
        demo.conf

In this example, the global zone is called `global-templates` and must be defined in
your zone configuration visible to all nodes.

    object Zone "global-templates" {
      global = true
    }

If the remote node does not have this zone configured, it will ignore the configuration
update if it accepts synchronized configuration.

If you do not require any global configuration, skip this setting.

### <a id="zone-config-sync-permissions"></a> Zone Configuration Synchronisation Permissions

Each [ApiListener](6-object-types.md#objecttype-apilistener) object must have the `accept_config` attribute
set to `true` to receive configuration from the parent `Zone` members. Default value is `false`.

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_config = true
    }

If `accept_config` is set to `false`, this instance won't accept configuration from remote
master instances anymore.

> ** Tip **
>
> Look into the [troubleshooting guides](16-troubleshooting.md#troubleshooting-cluster-config-sync) for debugging
> problems with the configuration synchronisation.


### <a id="zone-config-sync-best-practice"></a> Zone Configuration Synchronisation Best Practice

The configuration synchronisation works with multiple hierarchies. The following example
illustrate a quite common setup where the master is reponsible for configuration deployment:

* [High-Availability master zone](13-distributed-monitoring-ha.md#distributed-monitoring-high-availability)
* [Distributed satellites](13-distributed-monitoring-ha.md#cluster-scenarios-distributed-zones)
* [Remote clients](11-icinga2-client.md#icinga2-client-scenarios) connected to the satellite

While you could use the clients with local configuration and service discovery on the satellite/master
**bottom up**, the configuration sync could be more reasonable working **top-down** in a cascaded scenario.

Take pen and paper and draw your network scenario including the involved zone and endpoint names.
Once you've added them to your zones.conf as connection and permission configuration, start over with
the actual configuration organization:

* Ensure that `command` object definitions are globally available. That way you can use the
`command_endpoint` configuration more easily on clients as [command execution bridge](11-icinga2-client.md#icinga2-client-configuration-command-bridge)
* Generic `Templates`, `timeperiods`, `downtimes` should be synchronized in a global zone as well.
* [Apply rules](3-monitoring-basics.md#using-apply) can be synchronized globally. Keep in mind that they are evaluated on each instance,
and might require additional filters (e.g. `match("icinga2*", NodeName) or similar based on the zone information.
* [Apply rules](3-monitoring-basics.md#using-apply) specified inside zone directories will only affect endpoints in the same zone or below.
* Host configuration must be put into the specific zone directory.
* Duplicated host and service objects (also generated by faulty apply rules) will generate a configuration error.
* Consider using custom constants in your host/service configuration. Each instance may set their local value, e.g. for `PluginDir`.

This example specifies the following hierarchy over three levels:

* `ha-master` zone with two child zones `dmz1-checker` and `dmz2-checker`
* `dmz1-checker` has two client child zones `dmz1-client1` and `dmz1-client2`
* `dmz2-checker` has one client child zone `dmz2-client9`

The configuration tree could look like this:

    # tree /etc/icinga2/zones.d
    /etc/icinga2/zones.d
    ├── dmz1-checker
    │   └── health.conf
    ├── dmz1-client1
    │   └── hosts.conf
    ├── dmz1-client2
    │   └── hosts.conf
    ├── dmz2-checker
    │   └── health.conf
    ├── dmz2-client9
    │   └── hosts.conf
    ├── global-templates
    │   ├── apply_notifications.conf
    │   ├── apply_services.conf
    │   ├── commands.conf
    │   ├── groups.conf
    │   ├── templates.conf
    │   └── users.conf
    ├── ha-master
    │   └── health.conf
    └── README
    
    7 directories, 13 files

If you prefer a different naming schema for directories or file names, go for it. If you
are unsure about the best method, join the [support channels](1-about.md#support) and discuss
with the community.

If you are planning to synchronize local service health checks inside a zone, look into the
[command endpoint](13-distributed-monitoring-ha.md#cluster-health-check-command-endpoint)
explainations.



## <a id="cluster-health-check"></a> Cluster Health Check

The Icinga 2 [ITL](7-icinga-template-library.md#icinga-template-library) provides
an internal check command checking all configured `EndPoints` in the cluster setup.
The check result will become critical if one or more configured nodes are not connected.

Example:

    object Host "icinga2a" {
      display_name = "Health Checks on icinga2a"

      address = "192.168.33.10"
      check_command = "hostalive"
    }

    object Service "cluster" {
        check_command = "cluster"
        check_interval = 5s
        retry_interval = 1s

        host_name = "icinga2a"
    }

Each cluster node should execute its own local cluster health check to
get an idea about network related connection problems from different
points of view.

Additionally you can monitor the connection from the local zone to the remote
connected zones.

Example for the `checker` zone checking the connection to the `master` zone:

    object Service "cluster-zone-master" {
      check_command = "cluster-zone"
      check_interval = 5s
      retry_interval = 1s
      vars.cluster_zone = "master"

      host_name = "icinga2b"
    }

## <a id="cluster-health-check-command-endpoint"></a> Cluster Health Check with Command Endpoints

If you are planning to sync the zone configuration inside a [High-Availability]()
cluster zone, you can also use the `command_endpoint` object attribute to
pin host/service checks to a specific endpoint inside the same zone.

This requires the `accept_commands` setting inside the [ApiListener](13-distributed-monitoring-ha.md#configure-apilistener-object)
object set to `true` similar to the [remote client command execution bridge](11-icinga2-client.md#icinga2-client-configuration-command-bridge)
setup.

Make sure to set `command_endpoint` to the correct endpoint instance.
The example below assumes that the endpoint name is the same as the
host name configured for health checks. If it differs, define a host
custom attribute providing [this information](11-icinga2-client.md#icinga2-client-configuration-command-bridge-master-config).

    apply Service "cluster-ha" {
      check_command = "cluster"
      check_interval = 5s
      retry_interval = 1s
      /* make sure host.name is the same as endpoint name */
      command_endpoint = host.name

      assign where regex("^icinga2[a|b]", host.name)
    }


## <a id="cluster-scenarios"></a> Cluster Scenarios

All cluster nodes are full-featured Icinga 2 instances. You only need to enabled
the features for their role (for example, a `Checker` node only requires the `checker`
feature enabled, but not `notification` or `ido-mysql` features).

> **Tip**
>
> There's a [Vagrant demo setup](https://github.com/Icinga/icinga-vagrant/tree/master/icinga2x-cluster)
> available featuring a two node cluster showcasing several aspects (config sync,
> remote command execution, etc.).

### <a id="cluster-scenarios-master-satellite-clients"></a> Cluster with Master, Satellites and Remote Clients

You can combine "classic" cluster scenarios from HA to Master-Checker with the
Icinga 2 Remote Client modes. Each instance plays a certain role in that picture.

Imagine the following scenario:

* The master zone acts as High-Availability zone
* Remote satellite zones execute local checks and report them to the master
* All satellites query remote clients and receive check results (which they also replay to the master)
* All involved nodes share the same configuration logic: zones, endpoints, apilisteners

You'll need to think about the following:

* Deploy the entire configuration from the master to satellites and cascading remote clients? ("top down")
* Use local client configuration instead and report the inventory to satellites and cascading to the master? ("bottom up")
* Combine that with command execution brdiges on remote clients and also satellites


### <a id="cluster-scenarios-security"></a> Security in Cluster Scenarios

While there are certain capabilities to ensure the safe communication between all
nodes (firewalls, policies, software hardening, etc.) the Icinga 2 cluster also provides
additional security itself:

* [SSL certificates](13-distributed-monitoring-ha.md#manual-certificate-generation) are mandatory for cluster communication.
* Child zones only receive event updates (check results, commands, etc.) for their configured updates.
* Zones cannot influence/interfere other zones. Each checked object is assigned to only one zone.
* All nodes in a zone trust each other.
* [Configuration sync](13-distributed-monitoring-ha.md#zone-config-sync-permissions) is disabled by default.

### <a id="cluster-scenarios-features"></a> Features in Cluster Zones

Each cluster zone may use all available features. If you have multiple locations
or departments, they may write to their local database, or populate graphite.
Even further all commands are distributed amongst connected nodes. For example, you could
re-schedule a check or acknowledge a problem on the master, and it gets replicated to the
actual slave checker node.

> **Note**
>
> All features must be same on all endpoints inside an [HA zone](13-distributed-monitoring-ha.md#cluster-scenarios-high-availability).
> There are additional [High-Availability-enabled features](13-distributed-monitoring-ha.md#high-availability-features) available.

### <a id="cluster-scenarios-distributed-zones"></a> Distributed Zones

That scenario fits if your instances are spread over the globe and they all report
to a master instance. Their network connection only works towards the master master
(or the master is able to connect, depending on firewall policies) which means
remote instances won't see each/connect to each other.

All events (check results, downtimes, comments, etc.) are synced to the master node,
but the remote nodes can still run local features such as a web interface, reporting,
graphing, etc. in their own specified zone.

Imagine the following example with a master node in Nuremberg, and two remote DMZ
based instances in Berlin and Vienna. Additonally you'll specify
[global templates](13-distributed-monitoring-ha.md#zone-global-config-templates) available in all zones.

The configuration tree on the master instance `nuremberg` could look like this:

    zones.d
      global-templates/
        templates.conf
        groups.conf
      nuremberg/
        local.conf
      berlin/
        hosts.conf
      vienna/
        hosts.conf

The configuration deployment will take care of automatically synchronising
the child zone configuration:

* The master node sends `zones.d/berlin` to the `berlin` child zone.
* The master node sends `zones.d/vienna` to the `vienna` child zone.
* The master node sends `zones.d/global-templates` to the `vienna` and `berlin` child zones.

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

    object Zone "global-templates" {
      global = true
    }

The `nuremberg-master` zone will only execute local checks, and receive
check results from the satellite nodes in the zones `berlin` and `vienna`.

> **Note**
>
> The child zones `berlin` and `vienna` will get their configuration synchronised
> from the configuration master 'nuremberg'. The endpoints in the child
> zones **must not** have their `zones.d` directory populated if this endpoint
> [accepts synced configuration](13-distributed-monitoring-ha.md#zone-config-sync-permissions).

### <a id="cluster-scenarios-load-distribution"></a> Load Distribution

If you are planning to off-load the checks to a defined set of remote workers,
you can achieve that by:

* Deploying the configuration on all nodes.
* Let Icinga 2 distribute the load amongst all available nodes.

That way all remote check instances will receive the same configuration
but only execute their part. The master instance located in the `master` zone
can also execute checks, but you may also disable the `Checker` feature.

Configuration on the master node:

    zones.d/
      global-templates/
      master/
      checker/

If you are planning to have some checks executed by a specific set of checker nodes,
you have to define additional zones and define these check objects there.

Endpoints:

    object Endpoint "master-node" {
      host = "master.icinga.org"
    }

    object Endpoint "checker1-node" {
      host = "checker1.icinga.org"
    }

    object Endpoint "checker2-node" {
      host = "checker2.icinga.org"
    }


Zones:

    object Zone "master" {
      endpoints = [ "master-node" ]
    }

    object Zone "checker" {
      endpoints = [ "checker1-node", "checker2-node" ]
      parent = "master"
    }

    object Zone "global-templates" {
      global = true
    }

> **Note**
>
> The child zones `checker` will get its configuration synchronised
> from the configuration master 'master'. The endpoints in the child
> zone **must not** have their `zones.d` directory populated if this endpoint
> [accepts synced configuration](13-distributed-monitoring-ha.md#zone-config-sync-permissions).

### <a id="cluster-scenarios-high-availability"></a> Cluster High Availability

High availability with Icinga 2 is possible by putting multiple nodes into
a dedicated [zone](13-distributed-monitoring-ha.md#configure-cluster-zones). All nodes will elect one
active master, and retry an election once the current active master is down.

Selected features provide advanced [HA functionality](13-distributed-monitoring-ha.md#high-availability-features).
Checks and notifications are load-balanced between nodes in the high availability
zone.

Connections from other zones will be accepted by all active and passive nodes
but all are forwarded to the current active master dealing with the check results,
commands, etc.

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b", "icinga2c" ]
    }

Two or more nodes in a high availability setup require an [initial cluster sync](13-distributed-monitoring-ha.md#initial-cluster-sync).

> **Note**
>
> Keep in mind that **only one node acts as configuration master** having the
> configuration files in the `zones.d` directory. All other nodes **must not**
> have that directory populated. Instead they are required to
> [accept synced configuration](13-distributed-monitoring-ha.md#zone-config-sync-permissions).
> Details in the [Configuration Sync Chapter](13-distributed-monitoring-ha.md#cluster-zone-config-sync).

### <a id="cluster-scenarios-multiple-hierarchies"></a> Multiple Hierarchies

Your master zone collects all check results for reporting and graphing and also
does some sort of additional notifications.
The customers got their own instances in their local DMZ zones. They are limited to read/write
only their services, but replicate all events back to the master instance.
Within each DMZ there are additional check instances also serving interfaces for local
departments. The customers instances will collect all results, but also send them back to
your master instance.
Additionally the customers instance on the second level in the middle prohibits you from
sending commands to the subjacent department nodes. You're only allowed to receive the
results, and a subset of each customers configuration too.

Your master zone will generate global reports, aggregate alert notifications, and check
additional dependencies (for example, the customers internet uplink and bandwidth usage).

The customers zone instances will only check a subset of local services and delegate the rest
to each department. Even though it acts as configuration master with a master dashboard
for all departments managing their configuration tree which is then deployed to all
department instances. Furthermore the master NOC is able to see what's going on.

The instances in the departments will serve a local interface, and allow the administrators
to reschedule checks or acknowledge problems for their services.


## <a id="high-availability-features"></a> High Availability for Icinga 2 features

All nodes in the same zone require the same features enabled for High Availability (HA)
amongst them.

By default the following features provide advanced HA functionality:

* [Checks](13-distributed-monitoring-ha.md#high-availability-checks) (load balanced, automated failover)
* [Notifications](13-distributed-monitoring-ha.md#high-availability-notifications) (load balanced, automated failover)
* [DB IDO](13-distributed-monitoring-ha.md#high-availability-db-ido) (Run-Once, automated failover)

### <a id="high-availability-checks"></a> High Availability with Checks

All instances within the same zone (e.g. the `master` zone as HA cluster) must
have the `checker` feature enabled.

Example:

    # icinga2 feature enable checker

All nodes in the same zone load-balance the check execution. When one instance shuts down
the other nodes will automatically take over the reamining checks.

### <a id="high-availability-notifications"></a> High Availability with Notifications

All instances within the same zone (e.g. the `master` zone as HA cluster) must
have the `notification` feature enabled.

Example:

    # icinga2 feature enable notification

Notifications are load balanced amongst all nodes in a zone. By default this functionality
is enabled.
If your nodes should notify independent from any other nodes (this will cause
duplicated notifications if not properly handled!), you can set `enable_ha = false`
in the [NotificationComponent](6-object-types.md#objecttype-notificationcomponent) feature.

### <a id="high-availability-db-ido"></a> High Availability with DB IDO

All instances within the same zone (e.g. the `master` zone as HA cluster) must
have the DB IDO feature enabled.

Example DB IDO MySQL:

    # icinga2 feature enable ido-mysql

By default the DB IDO feature only runs on one node. All other nodes in the same zone disable
the active IDO database connection at runtime. The node with the active DB IDO connection is
not necessarily the zone master.

> **Note**
>
> The DB IDO HA feature can be disabled by setting the `enable_ha` attribute to `false`
> for the [IdoMysqlConnection](6-object-types.md#objecttype-idomysqlconnection) or
> [IdoPgsqlConnection](6-object-types.md#objecttype-idopgsqlconnection) object on **all** nodes in the
> **same** zone.
>
> All endpoints will enable the DB IDO feature and connect to the configured
> database and dump configuration, status and historical data on their own.

If the instance with the active DB IDO connection dies, the HA functionality will
automatically elect a new DB IDO master.

The DB IDO feature will try to determine which cluster endpoint is currently writing
to the database and bail out if another endpoint is active. You can manually verify that
by running the following query:

    icinga=> SELECT status_update_time, endpoint_name FROM icinga_programstatus;
       status_update_time   | endpoint_name
    ------------------------+---------------
     2014-08-15 15:52:26+02 | icinga2a
    (1 Zeile)

This is useful when the cluster connection between endpoints breaks, and prevents
data duplication in split-brain-scenarios. The failover timeout can be set for the
`failover_timeout` attribute, but not lower than 60 seconds.


## <a id="cluster-add-node"></a> Add a new cluster endpoint

These steps are required for integrating a new cluster endpoint:

* generate a new [SSL client certificate](13-distributed-monitoring-ha.md#manual-certificate-generation)
* identify its location in the zones
* update the `zones.conf` file on each involved node ([endpoint](13-distributed-monitoring-ha.md#configure-cluster-endpoints), [zones](13-distributed-monitoring-ha.md#configure-cluster-zones))
    * a new slave zone node requires updates for the master and slave zones
    * verify if this endpoints requires [configuration synchronisation](13-distributed-monitoring-ha.md#cluster-zone-config-sync) enabled
* if the node requires the existing zone history: [initial cluster sync](13-distributed-monitoring-ha.md#initial-cluster-sync)
* add a [cluster health check](13-distributed-monitoring-ha.md#cluster-health-check)

### <a id="initial-cluster-sync"></a> Initial Cluster Sync

In order to make sure that all of your cluster nodes have the same state you will
have to pick one of the nodes as your initial "master" and copy its state file
to all the other nodes.

You can find the state file in `/var/lib/icinga2/icinga2.state`. Before copying
the state file you should make sure that all your cluster nodes are properly shut
down.


## <a id="host-multiple-cluster-nodes"></a> Host With Multiple Cluster Nodes

Special scenarios might require multiple cluster nodes running on a single host.
By default Icinga 2 and its features will place their runtime data below the prefix
`LocalStateDir`. By default packages will set that path to `/var`.
You can either set that variable as constant configuration
definition in [icinga2.conf](4-configuring-icinga-2.md#icinga2-conf) or pass it as runtime variable to
the Icinga 2 daemon.

    # icinga2 -c /etc/icinga2/node1/icinga2.conf -DLocalStateDir=/opt/node1/var
