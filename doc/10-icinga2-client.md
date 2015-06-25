# <a id="icinga2-client"></a> Icinga 2 Client

## <a id="icinga2-client-introduction"></a> Introduction

Icinga 2 uses its own unique and secure communitication protol amongst instances.
Be it an High-Availability cluster setup, distributed load-balanced setup or just a single
agent [monitoring a remote client](10-icinga2-client.md#icinga2-client).

All communication is secured by TLS with certificates, and fully supports IPv4 and IPv6.

If you are planning to use the native Icinga 2 cluster feature for distributed
monitoring and high-availability, please continue reading in
[this chapter](distributed-monitoring-high-availability).

> **Tip**
>
> Don't panic - there are CLI commands available, including setup wizards for easy installation
> with SSL certificates.
> If you prefer to use your own CA (for example Puppet) you can do that as well.


## <a id="icinga2-client-scenarios"></a> Client Scenarios

* Clients with [local configuration](10-icinga2-client.md#icinga2-client-configuration-local), sending their inventory to the master
* Clients as [command execution bridge](10-icinga2-client.md#icinga2-client-configuration-command-bridge) without local configuration
* Clients receive their configuration from the master ([Cluster config sync](10-icinga2-client.md#icinga2-client-configuration-master-config-sync))

### <a id="icinga2-client-configuration-combined-scenarios"></a> Combined Client Scenarios

If your setup consists of remote clients with local configuration but also command execution bridges
and probably syncing global templates through the cluster config sync, you should take a deep
breath and take pen and paper to draw your design before starting over.

Keep the following hints in mind:

* You can blacklist remote nodes entirely. They are then ignored on `node update-config`
on the master.
* Your remote instance can have local configuration **and** act as remote command execution bridge.
* You can use the `global` cluster zones to sync check commands, templates, etc to your remote clients.
Be it just for command execution or for helping the local configuration.
* If your remote clients shouldn't have local configuration, remove `conf.d` inclusion from `icinga2`
and simply use the cluster configuration sync.
* `accept_config` and `accept_commands` are disabled by default in the `api` feature

If you are planning to use the Icinga 2 client inside a distributed setup, refer to
[this chapter](12-distributed-monitoring-ha.md#cluster-scenarios-master-satellite-clients) with detailed instructions.


## <a id="icinga2-client-installation"></a> Installation

### <a id="icinga2-client-installation-firewall"></a> Configure the Firewall

Icinga 2 master, satellite and client instances communicate using the default tcp
port `5665`. The communication is bi-directional and the first node opening the
connection "wins" if there are both connection ways enabled in your firewall policies.

If you are going to use CSR-Autosigning, you must (temporarly) allow the client
connecting to the master instance and open the firewall port. Once the client install is done,
you can close the port and use a different communication direction (master-to-client).

### <a id="icinga2-client-installation-master-setup"></a> Setup the Master for Remote Clients

If you are planning to use the [remote Icinga 2 clients](10-icinga2-client.md#icinga2-client)
you'll first need to update your master setup.

Your master setup requires the following

* SSL CA and signed certificate for the master
* Enabled API feature, and a local Endpoint and Zone object configuration
* Firewall ACLs for the communication port (default 5665)

You can use the [CLI command](8-cli-commands.md#cli-command-node) `node wizard` for setting up a new node
on the master. The command must be run as root, all Icinga 2 specific files
will be updated to the icinga user the daemon is running as (certificate files
for example).

Make sure to answer the first question with `n` (no).

    nbmif /etc/icinga2 # icinga2 node wizard
    Welcome to the Icinga 2 Setup Wizard!

    We'll guide you through all required configuration details.

    Please specify if this is a satellite setup ('n' installs a master setup) [Y/n]: n
    Starting the Master setup routine...
    Please specifiy the common name (CN) [icinga2-node1.localdomain]: 
    Checking the 'api' feature...
    'api' feature not enabled, running 'api setup' now.
    information/cli: Generating new CA.

    information/base: Writing private key to '/var/lib/icinga2/ca/ca.key'.
    information/base: Writing X509 certificate to '/var/lib/icinga2/ca/ca.crt'.
    information/cli: Initializing serial file in '/var/lib/icinga2/ca/serial.txt'.
    information/cli: Generating new CSR in '/etc/icinga2/pki/icinga2-node1.localdomain.csr'.

    information/base: Writing private key to '/etc/icinga2/pki/icinga2-node1.localdomain.key'.
    information/base: Writing certificate signing request to '/etc/icinga2/pki/icinga2-node1.localdomain.csr'.
    information/cli: Signing CSR with CA and writing certificate to '/etc/icinga2/pki/icinga2-node1.localdomain.crt'.

    information/cli: Copying CA certificate to '/etc/icinga2/pki/ca.crt'.

    information/cli: Adding new ApiUser 'root' in '/etc/icinga2/conf.d/api-users.conf'.

    information/cli: Enabling the ApiListener feature.

    Enabling feature api. Make sure to restart Icinga 2 for these changes to take effect.
    information/cli: Dumping config items to file '/etc/icinga2/zones.conf'.
    Please specify the API bind host/port (optional):
    Bind Host []: 
    Bind Port []: 
    information/cli: Updating constants.conf.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    Done.

    Now restart your Icinga 2 daemon to finish the installation!


The setup wizard will do the following:

* Check if the `api` feature is already enabled, and if not:
 * Generate a local CA in `/var/lib/icinga2/ca` or use the existing one
 * Generate a new CSR, sign it with the local CA and copying it into `/etc/icinga2/pki`
 * Enabling the API feature, and setting optional `bind_host` and `bind_port`
* Generate a local zone and endpoint configuration for this master based on FQDN
* Setting the `NodeName` and `TicketSalt` constants in [constants.conf](4-configuring-icinga-2.md#constants-conf)

The setup wizard does not automatically restart Icinga 2.

Verify the modified configuration:

    # egrep 'NodeName|TicketSalt' /etc/icinga2/constants.conf

    # cat /etc/icinga2/zones.conf
    /*
     * Generated by Icinga 2 node setup commands
     * on 2015-02-09 15:21:49 +0100
     */

    object Endpoint "icinga2-node1.localdomain" {
    }

    object Zone "master" {
    	//this is the local node master named  = "master"
    	endpoints = [ "icinga2-node1.localdomain" ]
    }

Validate the configuration and restart Icinga 2.


> **Note**
>
> This setup wizard will install a standalone master, HA cluster scenarios are currently
> not supported and require manual modifications afterwards.

## <a id="icinga2-client-setup"></a> Client Setup for Remote Monitoring

Icinga 2 can be installed on Linux/Unix and Windows. While
[Linux/Unix](10-icinga2-client.md#icinga2-client-installation-client-setup-linux) will be using the [CLI command](8-cli-commands.md#cli-command-node)
`node wizard` for a guided setup, you will need to use the
graphical installer for Windows based client setup.

Your client setup requires the following

* A ready configured and installed [master node](10-icinga2-client.md#icinga2-client-installation-master-setup)
* SSL signed certificate for communication with the master (Use [CSR auto-signing](10-icinga2-client.md#csr-autosigning-requirements)).
* Enabled API feature, and a local Endpoint and Zone object configuration
* Firewall ACLs for the communication port (default 5665)



### <a id="csr-autosigning-requirements"></a> Requirements for CSR Auto-Signing

If your remote clients are capable of connecting to the central master, Icinga 2
supports CSR auto-signing.

First you'll need to define a secure ticket salt in the [constants.conf](4-configuring-icinga-2.md#constants-conf).
The [setup wizard for the master setup](10-icinga2-client.md#icinga2-client-installation-master-setup) will create
one for you already.

    # grep TicketSalt /etc/icinga2/constants.conf

The client setup wizard will ask you to generate a valid ticket number using its CN.
If you already know your remote client's Common Names (CNs) - usually the FQDN - you
can generate all ticket numbers on-demand.

This is also reasonable if you are not capable of installing the remote client, but
a colleague of yours, or a customer.

Example for a client:

    # icinga2 pki ticket --cn icinga2-node2.localdomain


> **Note**
>
> You can omit the `--salt` parameter using the `TicketSalt` constant from
> [constants.conf](4-configuring-icinga-2.md#constants-conf) if already defined and Icinga 2 was
> reloaded after the master setup.

### <a id="certificates-manual-creation"></a> Manual SSL Certificate Generation

This is described separately in the [cluster setup chapter](12-distributed-monitoring-ha.md#manual-certificate-generation).

> **Note**
>
> If you're using [CSR Auto-Signing](10-icinga2-client.md#csr-autosigning-requirements), skip this step.


### <a id="icinga2-client-installation-client-setup-linux"></a> Setup the Client on Linux

There is no extra client binary or package required. Install Icinga 2 from your distribution's package
repository as described in the general [installation instructions](2-getting-started.md#setting-up-icinga2).

Please make sure that either [CSR Auto-Signing](10-icinga2-client.md#csr-autosigning-requirements) requirements
are fulfilled, or that you're using [manual SSL certificate generation](12-distributed-monitoring-ha.md#manual-certificate-generation).

> **Note**
>
> You don't need any features (DB IDO, Livestatus) or user interfaces on the remote client.
> Install them only if you're planning to use them.

Once the package installation succeeded, use the `node wizard` CLI command to install
a new Icinga 2 node as client setup.

You'll need the following configuration details:

* The client common name (CN). Defaults to FQDN.
* The client's local zone name. Defaults to FQDN.
* The master endpoint name. Look into your master setup `zones.conf` file for the proper name.
* The master endpoint connection information. Your master's IP address and port (port defaults to 5665)
* The [request ticket number](10-icinga2-client.md#csr-autosigning-requirements) generated on your master
for CSR Auto-Signing
* Bind host/port for the Api feature (optional)

The command must be run as root, all Icinga 2 specific files will be updated to the icinga
user the daemon is running as (certificate files for example). The wizard creates backups
of configuration and certificate files if already existing.

Capitalized options in square brackets (e.g. `[Y/n]`) signal the default value and
allow you to continue pressing `Enter` instead of entering a value.

    # icinga2 node wizard
    Welcome to the Icinga 2 Setup Wizard!
    We'll guide you through all required configuration details.

    Please specify if this is a satellite setup ('n' installs a master setup) [Y/n]:
    Starting the Node setup routine...
    Please specifiy the common name (CN) [icinga2-node2.localdomain]:
    Please specifiy the local zone name [icinga2-node2.localdomain]:
    Please specify the master endpoint(s) this node should connect to:
    Master Common Name (CN from your master setup): icinga2-node1.localdomain
    Please fill out the master connection information:
    Master endpoint host (optional, your master's IP address or FQDN): 192.168.56.101
    Master endpoint port (optional) []:
    Add more master endpoints? [y/N]
    Please specify the master connection for CSR auto-signing (defaults to master endpoint host):
    Host [192.168.56.101]:
    Port [5665]:
    information/base: Writing private key to '/etc/icinga2/pki/icinga2-node2.localdomain.key'.
    information/base: Writing X509 certificate to '/etc/icinga2/pki/icinga2-node2.localdomain.crt'.
    information/cli: Generating self-signed certifiate:
    information/cli: Fetching public certificate from master (192.168.56.101, 5665):

    information/cli: Writing trusted certificate to file '/etc/icinga2/pki/trusted-master.crt'.
    information/cli: Stored trusted master certificate in '/etc/icinga2/pki/trusted-master.crt'.

    Please specify the request ticket generated on your Icinga 2 master.
     (Hint: # icinga2 pki ticket --cn 'icinga2-node2.localdomain'): ead2d570e18c78abf285d6b85524970a0f69c22d
    information/cli: Processing self-signed certificate request. Ticket 'ead2d570e18c78abf285d6b85524970a0f69c22d'.

    information/cli: Writing signed certificate to file '/etc/icinga2/pki/icinga2-node2.localdomain.crt'.
    information/cli: Writing CA certificate to file '/etc/icinga2/pki/ca.crt'.
    Please specify the API bind host/port (optional):
    Bind Host []:
    Bind Port []:
    information/cli: Disabling the Notification feature.
    Disabling feature notification. Make sure to restart Icinga 2 for these changes to take effect.
    information/cli: Enabling the Apilistener feature.
    Enabling feature api. Make sure to restart Icinga 2 for these changes to take effect.
    information/cli: Created backup file '/etc/icinga2/features-available/api.conf.orig'.
    information/cli: Generating local zones.conf.
    information/cli: Dumping config items to file '/etc/icinga2/zones.conf'.
    information/cli: Created backup file '/etc/icinga2/zones.conf.orig'.
    information/cli: Updating constants.conf.
    information/cli: Created backup file '/etc/icinga2/constants.conf.orig'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    Done.

    Now restart your Icinga 2 daemon to finish the installation!


The setup wizard will do the following:

* Generate a new self-signed certificate and copy it into `/etc/icinga2/pki`
* Store the master's certificate as trusted certificate for requesting a new signed certificate
(manual step when using `node setup`).
* Request a new signed certificate from the master and store updated certificate and master CA in `/etc/icinga2/pki`
* Generate a local zone and endpoint configuration for this client and the provided master information
(based on FQDN)
* Disabling the `notification` feature for this client
* Enabling the `api` feature, and setting optional `bind_host` and `bind_port`
* Setting the `NodeName` constant in [constants.conf](4-configuring-icinga-2.md#constants-conf)

The setup wizard does not automatically restart Icinga 2.

Verify the modified configuration:

    # grep 'NodeName' /etc/icinga2/constants.conf

    # cat /etc/icinga2/zones.conf
    /*
     * Generated by Icinga 2 node setup commands
     * on 2015-02-09 16:56:10 +0100
     */

    object Endpoint "icinga2-node1.localdomain" {
    	host = "192.168.56.101"
    }

    object Zone "master" {
    	endpoints = [ "icinga2-node1.localdomain" ]
    }

    object Endpoint "icinga2-node2.localdomain" {
    }

    object Zone "icinga2-node2.localdomain" {
    	//this is the local node = "icinga2-node2.localdomain"
    	endpoints = [ "icinga2-node2.localdomain" ]
    	parent = "master"
    }

Validate the configuration and restart Icinga 2.

If you are getting an error when requesting the ticket number, please check the following:

* Can your client connect to the master instance?
* Is the CN the same (from pki ticket on the master and setup node on the client)?
* Is the ticket expired?



#### <a id="icinga2-client-installation-client-setup-linux-manual"></a> Manual Setup without Wizard

Instead of using the `node wizard` cli command, there is an alternative `node setup`
cli command available which has some pre-requisites. Make sure that the
`/etc/icinga2/pki` exists and is owned by the `icinga` user (or the user Icinga 2 is
running as).

`icinga2-node1.localdomain` is the already installed master instance while
`icinga2-node2.localdomain` is the instance where the installation cli commands
are executed.

Required information:

* The client common name (CN). Use the FQDN, e.g. `icinga2-node2.localdomain`.
* The master host and zone name. Pass that to `pki save-cert` as `--host` parameter for example.
* The client ticket number generated on the master (`icinga2 pki ticket --cn icinga2-node2.localdomain`)

Generate a new local self-signed certificate.

    # icinga2 pki new-cert --cn icinga2-node2.localdomain \
    --key /etc/icinga2/pki/icinga2-node2.localdomain.key \
    --cert /etc/icinga2/pki/icinga2-node2.localdomain.crt

Request the master certificate from the master host (`icinga2-node1.localdomain`)
and store it as `trusted-master.crt`. Review it and continue.

    # icinga2 pki save-cert --key /etc/icinga2/pki/icinga2-node2.localdomain.key \
    --cert /etc/icinga2/pki/icinga2-node2.localdomain.crt \
    --trustedcert /etc/icinga2/pki/trusted-master.crt \
    --host icinga2-node1.localdomain

Send the self-signed certificate to the master host using the ticket number and
receive a CA signed certificate and the master's `ca.crt` certificate.
Specify the path to the previously stored trusted master certificate.

    # icinga2 pki request --host icinga2-node1.localdomain \
    --port 5665 \
    --ticket ead2d570e18c78abf285d6b85524970a0f69c22d \
    --key /etc/icinga2/pki/icinga2-node2.localdomain.key \
    --cert /etc/icinga2/pki/icinga2-node2.localdomain.crt \
    --trustedcert /etc/icinga2/pki/trusted-master.crt \
    --ca /etc/icinga2/pki/ca.crt

Continue with the additional node setup steps. Specify a local endpoint and zone name (`icinga2-node2.localdomain`)
and set the master host (`icinga2-node1.localdomain`) as parent zone configuration. Specify the path to
the previously stored trusted master certificate.

    # icinga2 node setup --ticket ead2d570e18c78abf285d6b85524970a0f69c22d \
    --endpoint icinga2-node1.localdomain \
    --zone icinga2-node2.localdomain \
    --master_host icinga2-node1.localdomain \
    --trustedcert /etc/icinga2/pki/trusted-master.crt

Restart Icinga 2 once complete.

    # service icinga2 restart


### <a id="icinga2-client-installation-client-setup-windows"></a> Setup the Client on Windows

Download the MSI-Installer package from [http://packages.icinga.org/windows/](http://packages.icinga.org/windows/).

Requirements:
* Windows Vista/Server 2008 or higher
* [Microsoft .NET Framework 2.0](http://www.microsoft.com/de-de/download/details.aspx?id=1639) if not already installed.

The setup wizard will install Icinga 2 and then continue with SSL certificate generation,
CSR-Autosigning and configuration setup.

You'll need the following configuration details:

* The client common name (CN). Defaults to FQDN.
* The client's local zone name. Defaults to FQDN.
* The master endpoint name. Look into your master setup `zones.conf` file for the proper name.
* The master endpoint connection information. Your master's IP address and port (defaults to 5665)
* The [request ticket number](10-icinga2-client.md#csr-autosigning-requirements) generated on your master
for CSR Auto-Signing
* Bind host/port for the Api feature (optional)

Once install is done, Icinga 2 is automatically started as a Windows service.

The Icinga 2 configuration is located inside the installation path and can be edited with
your favorite editor.

Configuration validation is done similar to the linux pendant on the Windows shell:

    C:> icinga2.exe daemon -C




## <a id="icinga2-client-configuration-modes"></a> Client Configuration Modes

* Clients with [local configuration](10-icinga2-client.md#icinga2-client-configuration-local), sending their inventory to the master
* Clients as [command execution bridge](10-icinga2-client.md#icinga2-client-configuration-command-bridge) without local configuration
* Clients receive their configuration from the master ([Cluster config sync](10-icinga2-client.md#icinga2-client-configuration-master-config-sync))

### <a id="icinga2-client-configuration-local"></a> Clients with Local Configuration

This is considered as independant satellite using a local scheduler, configuration
and the possibility to add Icinga 2 features on demand.

There is no difference in the configuration syntax on clients to any other Icinga 2 installation.
You can also use additional features like notifications directly on the remote client, if you are
required to. Basically everything a single Icinga 2 instance provides by default.

The following convention applies to remote clients:

* The hostname in the default host object should be the same as the Common Name (CN) used for SSL setup
* Add new services and check commands locally

Local configured checks are transferred to the central master. There are additional `node`
cli commands available which allow you to list/add/remove/blacklist remote clients and
generate the configuration on the master.


#### <a id="icinga2-remote-monitoring-master-discovery"></a> Discover Client Services on the Master

Icinga 2 clients will sync their locally defined objects to the defined master node. That way you can
list, add, filter and remove nodes based on their `node`, `zone`, `host` or `service` name.

List all discovered nodes (satellites, agents) and their hosts/services:

    # icinga2 node list
    Node 'icinga2-node2.localdomain' (last seen: Mon Feb  9 16:58:21 2015)
        * Host 'icinga2-node2.localdomain'
            * Service 'ping4'
            * Service 'ping6'
            * Service 'ssh'
            * Service 'http'
            * Service 'disk'
            * Service 'disk /'
            * Service 'icinga'
            * Service 'load'
            * Service 'procs'
            * Service 'swap'
            * Service 'users'

Listing the node and its host(s) and service(s) does not modify the master configuration yet. You
meed to generate the configuration in the next step.


### <a id="icinga2-client-master-discovery-generate-config"></a> Generate Configuration for Client Services on the Master

There is a dedicated Icinga 2 CLI command for updating the client services on the master,
generating all required configuration.

    # icinga2 node update-config

The generated configuration of all nodes is stored in the `repository.d/` directory.

By default, the following additional configuration is generated:
* add `Endpoint` and `Zone` objects for the newly added node
* add `cluster-zone` health check for the master host for reachability and dependencies
* use the default templates `satellite-host` and `satellite-service` defined in `/etc/icinga2/conf.d/satellite.conf`
* apply a dependency for all other hosts on the remote satellite prevening failure checks/notifications

If hosts or services disappeared from the client discovery, it will remove the existing configuration objects
from the config repository. If there are existing hosts/services defined or modified, the CLI command will not
overwrite these (modified) configuration files.

After updating the configuration repository, make sure to reload Icinga 2.

    # service icinga2 reload

Using systemd:
    # systemctl reload icinga2


The `update-config` CLI command will fail, if there are uncommitted changes for the
configuration repository.
Please review these changes manually, or clear the commit and try again. This is a
safety hook to prevent unwanted manual changes to be committed by a updating the
client discovered objects only.

    # icinga2 repository commit --simulate
    # icinga2 repository clear-changes
    # icinga2 repository commit


### <a id="icinga2-client-configuration-command-bridge"></a> Clients as Command Execution Bridge

Similar to other addons (NRPE, NSClient++, etc) the remote Icinga 2 client will only
execute commands the master instance is sending. There are no local host or service
objects configured, only the check command definitions must be configured.

> **Note**
>
> Remote clients must explicitely accept commands in a similar
> fashion as cluster nodes [accept configuration](12-distributed-monitoring-ha.md#cluster-zone-config-sync).
> This is due to security reasons.

Edit the `api` feature configuration in `/etc/icinga2/features-enabled/api.conf` on your client
and set `accept_commands` to `true`.

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_commands = true
    }

Icinga 2 on the remote client does not schedule checks locally, or keep checking
hosts/services on connection loss. This mode also does not allow to use features
for backend data writing (DB IDO, Perfdata, etc.) as the client does not have
local objects configured.

Icinga 2 already provides a variety of `CheckCommand` definitions using the Plugin
Check Commands, but you should also modify the local configuration inside `commands.conf`
for example.

If you're wondering why you need to keep the same command configuration on the master and
remote client: Icinga 2 calculates all required runtime macros used as command arguments on
the master and sends that information to the client.
In case you want to limit command arguments or handles values in a different manner, you
can modify the check command configuration on the remote client only. See [this issue](https://dev.icinga.org/issues/8221#note-3)
for more details.

### <a id="icinga2-client-configuration-command-bridge-master-config"></a> Master Configuration for Clients as Command Execution Bridge

This step involves little knowledge about the way the Icinga 2 nodes communication and trust
each other. Each client is configured as `Endpoint` object providing connection information.
As a matter of trust the client `Endpoint` is a member of its own `Zone` object which gets
the master zone configured as parent. That way the master knows how to connect to the client
and where to execute the check commands.

Add an `Endpoint` and `Zone` configuration object for the remote client
in `/etc/icinga2/zones.conf` and define a trusted master zone as `parent`.

    object Endpoint "icinga2-node2.localdomain" {
      host = "192.168.56.102"
    }

    object Zone "icinga2-node2.localdomain" {
      parent = "master"
      endpoints = [ "icinga2-node2.localdomain" ]
    }

More details here:
* [configure endpoints](12-distributed-monitoring-ha.md#configure-cluster-endpoints)
* [configure zones](12-distributed-monitoring-ha.md#configure-cluster-zones)


Once you have configured the required `Endpoint` and `Zone` object definition, you can start
configuring your host and service objects. The configuration is simple: If the `command_endpoint`
attribute is set, Icinga 2 calculcates all required runtime macros and sends that over to the
defined endpoint. The check result is then received asynchronously through the cluster protocol.

    object Host "host-remote" {
      import "generic-host"

      address = "127.0.0.1"
      address6 = "::1"

      vars.os = "Linux"
    }

    apply Service "users-remote" {
      import "generic-service"

      check_command = "users"
      command_endpoint = "remote-client1"

      vars.users_wgreater = 10
      vars.users_cgreater = 20

      /* assign where a remote client pattern is matched */
      assign where match("*-remote", host.name)
    }


If there is a failure on execution (for example, the local check command configuration or the plugin
is missing), the check will return `UNKNOWN` and populate the check output with the error message.
This will happen in a similar fashion if you forgot to enable the `accept_commands` attribute
inside the `api` feature.

If you don't want to define the endpoint name inside the service apply rule everytime, you can
also easily inherit this from a host's custom attribute like shown in the example below.

    object Host "host-remote" {
      import "generic-host"

      address = "127.0.0.1"
      address6 = "::1"

      vars.os = "Linux"

      vars.remote_client = "remote-client1"

      /* host specific check arguments */
      vars.users_wgreater = 10
      vars.users_cgreater = 20
    }

    apply Service "users-remote" {
      import "generic-service"

      check_command = "users"
      command_endpoint = host.vars.remote_client

      /* override (remote) command arguments with host settings */
      vars.users_wgreater = host.vars.users_wgreater
      vars.users_cgreater = host.vars.users_cgreater

      /* assign where a remote client is set */
      assign where host.vars.remote_client
    }

That way your generated host object is the information provider and the service apply
rules must only be configured once.

> **Tip**
>
> [Event commands](3-monitoring-basics.md#event-commands) are executed on the
> remote command endpoint as well. You do not need
> an additional transport layer such as SSH or similar.


### <a id="icinga2-client-configuration-master-config-sync"></a> Clients with Master Config Sync

This is an advanced configuration mode which requires knowledge about the Icinga 2
cluster configuration and its object relation (Zones, Endpoints, etc) and the way you
will be able to sync the configuration from the master to the remote satellite or client.

Please continue reading in the [distributed monitoring chapter](12-distributed-monitoring-ha.md#distributed-monitoring-high-availability),
especially the [configuration synchronisation](12-distributed-monitoring-ha.md#cluster-zone-config-sync)
and [best practices](12-distributed-monitoring-ha.md#zone-config-sync-best-practice).




### <a id="icinga2-client-cli-node"></a> Advanced Node Cli Actions

#### <a id="icinga2-remote-monitoring-master-discovery-blacklist-whitelist"></a> Blacklist/Whitelist for Clients on the Master

It's sometimes necessary to `blacklist` an entire remote client, or specific hosts or services
provided by this client. While it's reasonable for the local admin to configure for example an
additional ping check, you're not interested in that on the master sending out notifications
and presenting the dashboard to your support team.

Blacklisting an entire set might not be sufficient for excluding several objects, be it a
specific remote client with one ping servie you're interested in. Therefore you can `whitelist`
clients, hosts, services in a similar manner

Example for blacklisting all `ping*` services, but allowing only `probe` host with `ping4`:

    # icinga2 node blacklist add --zone "*" --host "*" --service "ping*"
    # icinga2 node whitelist add --zone "*" --host "probe" --service "ping*"

You can `list` and `remove` existing blacklists:

    # icinga2 node blacklist list
    Listing all blacklist entries:
    blacklist filter for Node: '*' Host: '*' Service: 'ping*'.

    # icinga2 node whitelist list
    Listing all whitelist entries:
    whitelist filter for Node: '*' Host: 'probe' Service: 'ping*'.


> **Note**
>
> The blacklist feature only prevents future updates from creating and removing objects, but it does not remove already existing objects.
> The `--zone` and `--host` arguments are required. A zone is always where the remote client is in.
> If you are unsure about it, set a wildcard (`*`) for them and filter only by host/services.



#### <a id="icinga2-client-master-discovery-manual"></a> Manually Discover Clients on the Master

Add a to-be-discovered client to the master:

    # icinga2 node add my-remote-client

Set the connection details, and the Icinga 2 master will attempt to connect to this node and sync its
object repository.

    # icinga2 node set my-remote-client --host 192.168.33.101 --port 5665

You can control that by calling the `node list` command:

    # icinga2 node list
    Node 'my-remote-client' (host: 192.168.33.101, port: 5665, log duration: 1 day, last seen: Sun Nov  2 17:46:29 2014)

#### <a id="icinga2-remote-monitoring-master-discovery-remove"></a> Remove Discovered Clients

If you don't require a connected agent, you can manually remove it and its discovered hosts and services
using the following CLI command:

    # icinga2 node remove my-discovered-agent

> **Note**
>
> Better use [blacklists and/or whitelists](10-icinga2-client.md#icinga2-remote-monitoring-master-discovery-blacklist-whitelist)
> to control which clients and hosts/services are integrated into your master configuration repository.
