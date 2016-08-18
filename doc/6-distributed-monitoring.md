# <a id="distributed-monitoring"></a> Distributed Monitoring with Masters, Satellites and Clients

This chapter will guide you through the setup of a distributed monitoring
environment. This includes High-Availability clustering and Icinga 2 client
setup details.

## <a id="distributed-monitoring-roles"></a> Roles: Master, Satellite and Clients

A `node` in the Icinga2 cluster is defined as an `endpoint` object, as found in the **zones.conf** file.
Icinga 2 nodes can be given names for easier understanding:

* A `master` node which is on top of the hierarchy (it *has no* parent)
* A `satellite` node which is a child of a `master` or `satelite` node 
    * It *has* a parent
    * Is able to schedule checks and thus requires a configuration
    * Continues to run checks even if the master is temporarily unavailable
* A `client` node which works as an `agent` connected to `master` and/or `satellite` nodes
    * It *has* a parent
    * Is not able to schedule checks and thus needs no configuration
    * It just runs commands in the moment they are provided by its parent. For that, the parent evaluates the 'command_endpoint' config attribute.
    * Will not run commands if the parent is temporarily unavailable.

Technically, it is possible to mix these role features freely, but above roles are the most used ones.

The following sections will refer to these roles and explain
their possibilities and differences in detail.

> **Tip**:
>
> If you just want to install a single master node with several hosts
> monitored with Icinga 2 clients continue reading, we'll start with
> simple examples.
> In case you are planning a huge cluster setup with multiple levels and
> lots of clients - read on, we'll deal with these examples later on.

The installation on each system is the same -- you need to install the
Icinga 2 package. The required configuration steps are mostly helped with CLI commands.

The first thing you need learn about a distributed setup -- the overall hierarchy.

## <a id="distributed-monitoring-zones"></a> Zones

The Icinga 2 hierarchy consists of so-called [Zone](9-object-types.md#objecttype-zone) objects.
Zones depend on a parent-child relationship for trusting each other.

![Icinga 2 Distributed Zones](images/distributed-monitoring/icinga2_distributed_zones.png)

Example for the satellite zones which have the `master` zone as parent zone:

    object Zone "master" {
       //...
    }

    object Zone "satellite region 1" {
      parent = "master"
      //...
    }

    object Zone "satellite region 2" {
      parent = "master"
      //...
    }

There are certain limitations for child zones - e.g. their members are not allowed
to send configuration to the parent zone members. Vice versa the
trust hierarchy allows for example the `master` zone to send
configuration files to the `satellite` zone. Read more about this
in the [security section](6-distributed-monitoring.md#distributed-monitoring-security).

`client` nodes also have their own unique zone. By convention you
can use the FQDN for the zone name.

## <a id="distributed-monitoring-endpoints"></a> Endpoints

Nodes which are a member of a zone are so-called [Endpoint](9-object-types.md#objecttype-endpoint) objects.

![Icinga 2 Distributed Endpoints](images/distributed-monitoring/icinga2_distributed_endpoints.png)

Example configuration for two endpoints in different zones:

    object Endpoint "icinga2-master1.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-satellite1.localdomain" {
      host = "192.168.56.105"
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain" ]
    }

    object Zone "satellite" {
      endpoints = [ "icinga2-satellite1.localdomain" ]
      parent = "master"
    }

All endpoints in the same zone work as High-Availability setup. If you have for
example two nodes in the `master` zone, they will load-balance the check execution.

Endpoint objects are important for specifying the connection
information e.g. if the master should actively try to connect to a client.

The zone membership is defined inside the `Zone` object definition using
the `endpoints` attribute with an array of `Endpoint` names.

## <a id="distributed-monitoring-apilistener"></a> ApiListener

In case you are using the CLI commands later you won't directly see
this configuration object. The [ApiListener](9-object-types.md#objecttype-apilistener)
object is used to load the SSL certificates and specify restrictions
for e.g. accepting configuration.

It is further used for the [Icinga 2 REST API](12-icinga2-api.md#icinga2-api) which shares
the same host and port with the Icinga 2 Cluster protocol.

The object configuration is stored as feature in `/etc/icinga2/features-enabled/api.conf`
by default. 
Depending on your needs, set `accept_config = true` and / or `accept_commands = true` in that file.

In order to use the `api` feature you need to enable it and restart Icinga 2.

    icinga2 feature enable api

## <a id="distributed-monitoring-conventions"></a> Conventions

By convention all nodes should be configured using their FQDN.

Furthermore you must ensure that the following configuration names
are exact the same:

* Host certificate common name (CN)
* Endpoint configuration object
* NodeName constant

The cli commands will help you here already and minimize the effort. Just keep in mind
-- use the FQDN for endpoints and common names when asked.

## <a id="distributed-monitoring-security"></a> Security

While there are certain capabilities to ensure the safe communication between all
nodes (firewalls, policies, software hardening, etc.) the Icinga 2 also provides
additional security itself:

* SSL certificates are mandatory for cluster communication. There are CLI commands helping with their creation.
* Child zones only receive updates (check results, commands, etc.) for the config objects defined in that zone.
* Zones cannot influence/interfere other zones. Each checked object is assigned to only one zone.
* All nodes in a zone trust each other.
* [Config sync]() and [remote command endpoint execution]() is disabled by default.

## <a id="distributed-monitoring-setup-master"></a> Master Setup

This section explains how to install a central single master node using
the `node wizard` command. If you prefer to do a manual installation please
refer to the [manual setup]() section.

> **Note**:
> Master setups are not supported on Microsoft Windows operating systems. 

Required information:

  Parameter           | Description
  --------------------|--------------------
  Common name (CN)    | **Required.** By convention this should be the host's FQDN. Defaults to the FQDN.
  API bind host       | **Optional.** Allows to specify the address where the ApiListener is bound to. For advanced usage only.
  API bind port       | **Optional.** Allows to specify the port where the ApiListener is bound to. For advanced usage only (requires changing the default port 5665 everywhere).

The setup wizard will ensure that the following steps are taken:

* Setup the `api` feature
* Generate a new certificate authority (CA) in `/var/lib/icinga2/ca` if not existing
* Create a certificate signing request (CSR) for the local node
* Sign the CSR with the local CA and copy all files into the `/etc/icinga2/pki` directory
* Update the `zones.conf` file with the new zone hierarchy
* Update `/etc/icinga2/features-enabled/api.conf` and `constants.conf`

Example master setup for the `icinga2-master1.localdomain` node on CentOS 7:

    [root@icinga2-master1.localdomain /]# icinga2 node wizard
    Welcome to the Icinga 2 Setup Wizard!
    
    We'll guide you through all required configuration details.
    
    Please specify if this is a satellite setup ('n' installs a master setup) [Y/n]: n
    Starting the Master setup routine...
    Please specifiy the common name (CN) [icinga2-master1.localdomain]: icinga2-master1.localdomain
    Checking for existing certificates for common name 'icinga2-master1.localdomain'...
    Certificates not yet generated. Running 'api setup' now.
    information/cli: Generating new CA.
    information/base: Writing private key to '/var/lib/icinga2/ca/ca.key'.
    information/base: Writing X509 certificate to '/var/lib/icinga2/ca/ca.crt'.
    information/cli: Generating new CSR in '/etc/icinga2/pki/icinga2-master1.localdomain.csr'.
    information/base: Writing private key to '/etc/icinga2/pki/icinga2-master1.localdomain.key'.
    information/base: Writing certificate signing request to '/etc/icinga2/pki/icinga2-master1.localdomain.csr'.
    information/cli: Signing CSR with CA and writing certificate to '/etc/icinga2/pki/icinga2-master1.localdomain.crt'.
    information/cli: Copying CA certificate to '/etc/icinga2/pki/ca.crt'.
    Generating master configuration for Icinga 2.
    information/cli: Adding new ApiUser 'root' in '/etc/icinga2/conf.d/api-users.conf'.
    information/cli: Enabling the 'api' feature.
    Enabling feature api. Make sure to restart Icinga 2 for these changes to take effect.
    information/cli: Dumping config items to file '/etc/icinga2/zones.conf'.
    information/cli: Created backup file '/etc/icinga2/zones.conf.orig'.
    Please specify the API bind host/port (optional):
    Bind Host []:
    Bind Port []:
    information/cli: Created backup file '/etc/icinga2/features-available/api.conf.orig'.
    information/cli: Updating constants.conf.
    information/cli: Created backup file '/etc/icinga2/constants.conf.orig'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    Done.
    
    Now restart your Icinga 2 daemon to finish the installation!

    [root@icinga2-master1.localdomain /]# systemctl restart icinga2

The CA public and private key are stored in the `/var/lib/icinga2/ca` directory. Keep this path secure and include
it in your backups.

Once the master setup is completed, you can also use this node as primary CSR auto-signing
master. The following chapter will explain how to use the cli commands fetching their
signed certificate from this master node.

## <a id="distributed-monitoring-setup-satellite-client"></a> Client/Satellite Setup

This section describes the setup of a satellite and/or client connected to an
existing master node setup. If you haven't done so already please [run the master setup](6-distributed-monitoring.md#distributed-monitoring-setup-master).

Icinga 2 on the master node must be running and accepting connections on port `5665`.

### <a id="distributed-monitoring-setup-csr-auto-signing"></a> CSR Auto-Signing

The `node wizard` cli command will setup a satellite/client using CSR auto-signing. This
involves that the setup wizard sends a certificate signing request (CSR) to the
master node.
There is a security mechanism in place which requires the client to send in a valid
ticket for CSR auto-signing.

This ticket must be generated beforehand. The `ticket_salt` attribute for the [ApiListener](9-object-types.md#objecttype-apilistener)
must be properly configured in order to make this work.

There are two possible ways to retrieve the ticket:

* [CLI command](11-cli-commands.md#cli-command-pki) executed on the master node
* [REST API](12-icinga2-api.md#icinga2-api) request against the master node

Required information:

  Parameter           | Description
  --------------------|--------------------
  Common name (CN)    | **Required.** The common name for the satellite/client. By convention this should be the FQDN.

Example for the client `icinga2-client1.localdomain` generating a ticket on the master node
`icinga2-master1.localdomain`:

    [root@icinga2-master1.localdomain /]# icinga2 pki ticket --cn icinga2-client1.localdomain

Querying the [Icinga 2 API](12-icinga2-api.md#icinga2-api) on the master requires an [ApiUser](12-icinga2-api.md#icinga2-api-authentication)
object with at least the `actions/generate-ticket`.

    [root@icinga2-master1.localdomain /]# vim /etc/icinga2/conf.d/api-users.conf

    object ApiUser "client-pki-ticket" {
      password = "bea11beb7b810ea9ce6ea" //change this
      permissions = [ "actions/generate-ticket" ]
    }

    [root@icinga2-master1.localdomain /]# systemctl restart icinga2

Retrieve the ticket on the client node `icinga2-client1.localdomain` with curl for example:

     [root@icinga2-client1.localdomain /]# curl -k -s -u client-pki-ticket:bea11beb7b810ea9ce6ea -H 'Accept: application/json' \
     -X POST 'https://icinga2-master1.localdomain:5665/v1/actions/generate-ticket' -d '{ "cn": "icinga2-client1.localdomain" }'

Store that ticket number for the satellite/client setup below.

### <a id="distributed-monitoring-setup-client-linux"></a> Client/Satellite Linux Setup

Please ensure that you've run all the steps mentioned in the [client/satellite chapter](6-distributed-monitoring.md#distributed-monitoring-setup-satellite-client).

Required information:

  Parameter           | Description
  --------------------|--------------------
  Common name (CN)    | **Required.** By convention this should be the host's FQDN. Defaults to the FQDN.
  Master common name  | **Required.** Use the common name you've specified for your master node before.
  Establish connection to the master | **Optional.** Whether the client should attempt to connect the to master or not. Defaults to `y`.
  Master endpoint host | **Required if the the client should connect to the master.** The master's IP address or FQDN. This information is written to the `Endpoint` object configuration in the `zones.conf` file.
  Master endpoint port | **Optional if the the client should connect to the master.** The master's listening port. This information is written to the `Endpoint` object configuration.
  Add more master endpoints | **Optional.** If you have multiple master nodes configured, add them here.
  Master connection for CSR auto-signing | **Required.** The master node's IP address or FQDN and port where the client should request a certificate from. Defaults to the master endpoint host.
  Certificate information | **Required.** Verify that the connecting host really is the requested master node.
  Request ticket      | **Required.** Paste the previously generated [ticket number](6-distributed-monitoring.md#distributed-monitoring-setup-csr-auto-signing).
  API bind host       | **Optional.** Allows to specify the address where the ApiListener is bound to. For advanced usage only.
  API bind port       | **Optional.** Allows to specify the port where the ApiListener is bound to. For advanced usage only (requires changing the default port 5665 everywhere).
  Accept config       | **Optional.** Whether this node accepts configuration sync from the master node (required for [config sync mode](6-distributed-monitoring.md#distributed-monitoring-top-down-config-sync). Defaults to 'n'.
  Accept commands     | **Optional.** Whether this node accepts command execution message from the master node (required for [command endpoint mode](6-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint). Defaults to 'n'.


Example for the client `icinga2-client1.localdomain` generating a ticket on the master node
`icinga2-master1.localdomain`:

    [root@icinga2-master1.localdomain /]# icinga2 pki ticket --cn icinga2-client1.localdomain
    4f75d2ecd253575fe9180938ebff7cbca262f96e

Example client setup for the `icinga2-client1.localdomain` node on CentOS 7. This client
is configured to receive configuration sync and also accept commands.

    [root@icinga2-client1.localdomain /]# icinga2 node wizard
    Welcome to the Icinga 2 Setup Wizard!
    
    We'll guide you through all required configuration details.
    
    Please specify if this is a satellite setup ('n' installs a master setup) [Y/n]:
    Starting the Node setup routine...
    Please specifiy the common name (CN) [icinga2-client1.localdomain]: icinga2-client1.localdomain
    Please specify the master endpoint(s) this node should connect to:
    Master Common Name (CN from your master setup): icinga2-master1.localdomain
    Do you want to establish a connection to the master from this node? [Y/n]:
    Please fill out the master connection information:
    Master endpoint host (Your master's IP address or FQDN): 192.168.56.101
    Master endpoint port [5665]:
    Add more master endpoints? [y/N]:
    Please specify the master connection for CSR auto-signing (defaults to master endpoint host):
    Host [192.168.56.101]: 192.168.2.101
    Port [5665]:
    information/base: Writing private key to '/etc/icinga2/pki/icinga2-client1.localdomain.key'.
    information/base: Writing X509 certificate to '/etc/icinga2/pki/icinga2-client1.localdomain.crt'.
    information/cli: Fetching public certificate from master (192.168.56.101, 5665):
    
    Certificate information:
    
     Subject:     CN = icinga2-master1.localdomain
     Issuer:      CN = Icinga CA
     Valid From:  Feb 23 14:45:32 2016 GMT
     Valid Until: Feb 19 14:45:32 2031 GMT
     Fingerprint: AC 99 8B 2B 3D B0 01 00 E5 21 FA 05 2E EC D5 A9 EF 9E AA E3
    
    Is this information correct? [y/N]: y
    information/cli: Received trusted master certificate.
    
    Please specify the request ticket generated on your Icinga 2 master.
     (Hint: # icinga2 pki ticket --cn 'icinga2-client1.localdomain'): 4f75d2ecd253575fe9180938ebff7cbca262f96e
    information/cli: Requesting certificate with ticket '4f75d2ecd253575fe9180938ebff7cbca262f96e'.
    
    information/cli: Created backup file '/etc/icinga2/pki/icinga2-client1.localdomain.crt.orig'.
    information/cli: Writing signed certificate to file '/etc/icinga2/pki/icinga2-client1.localdomain.crt'.
    information/cli: Writing CA certificate to file '/etc/icinga2/pki/ca.crt'.
    Please specify the API bind host/port (optional):
    Bind Host []:
    Bind Port []:
    Accept config from master? [y/N]: y
    Accept commands from master? [y/N]: y
    information/cli: Disabling the Notification feature.
    Disabling feature notification. Make sure to restart Icinga 2 for these changes to take effect.
    information/cli: Enabling the Apilistener feature.
    information/cli: Generating local zones.conf.
    information/cli: Dumping config items to file '/etc/icinga2/zones.conf'.
    information/cli: Updating constants.conf.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    Done.
    
    Now restart your Icinga 2 daemon to finish the installation!

Now that you've succesfully installed a satellite/client please proceed to the
[configuration modes](6-distributed-monitoring.md#distributed-monitoring-configuration-modes).


### <a id="distributed-monitoring-setup-client-windows"></a> Client/Satellite Windows Setup

Download the MSI-Installer package from [http://packages.icinga.org/windows/](http://packages.icinga.org/windows/).

Requirements:

* Windows Vista/Server 2008 or higher
* [Microsoft .NET Framework 2.0](http://www.microsoft.com/de-de/download/details.aspx?id=1639) if not already installed.

The installer package includes the [NSClient++](http://www.nsclient.org/) so Icinga 2 can
use its built-in plugins. You can use the [nscp-local commands from the ITL](10-icinga-template-library.md#nscp-plugin-check-commands)
for these plugins.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_installer_01.png)
![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_installer_02.png)
![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_installer_03.png)
![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_installer_04.png)
![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_installer_05.png)

The graphical installer will offer to run the Icinga 2 setup wizard after the installation.
You can also manually run the Icinga 2 setup wizard from the start menu.

On a fresh installation the setup wizard will guide you through the initial configuration
as well as the required details for SSL certificate generation using CSR-Autosigning.

You'll need the following configuration details:

* The client common name (CN). Defaults to FQDN.
* The request ticket number generated on your master for CSR Auto-Signing

Example for the client `icinga2-client2.localdomain` generating a ticket on the master node
`icinga2-master1.localdomain`:

    [root@icinga2-master1.localdomain /]# icinga2 pki ticket --cn DESKTOP-IHRPO96

Fill in the required information and click `Add` to add a new master connection.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_01.png)

Add the following details:

* The master endpoint name. Look into your master setup `zones.conf` file for the proper name.
* The master endpoint connection information. Your master's IP address and port (defaults to 5665)

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_02.png)

You can optionally enable the following settings:

* Accept config updates from master (client with [config sync mode](6-distributed-monitoring.md#distributed-monitoring-top-down-config-sync))
* Accept commands from master (client as [command endpoint](6-distributed-monitoring.md#distributed-monitoring-top-down-command-endpoint)).
* Install/Update NSClient++

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_03.png)

The next step allows you to verify the CA presented by the master.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_04.png)

If you have chosen to install/update the NSClient++ package, the Icinga 2 setup wizard will ask
you to do so.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_05.png)

Finish the setup wizard.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_06.png)

Once install and configuration is done, Icinga 2 is automatically started as a Windows service.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_running_service.png)

The Icinga 2 configuration is located inside the `C:\ProgramData\icinga2` directory.
If you click `Examine Config` in the setup wizard, it will open a new explorer window.

![Icinga 2 Windows Setup](images/distributed-monitoring/icinga2_windows_setup_wizard_examine_config.png)

The configuration files can be modified with your favorite editor.

Configuration validation is done similar to the linux pendant on the Windows shell:

    C:> icinga2.exe daemon -C

**Note**: You have to run this command in a shell with `administrator` permissions.

In case you want to restart the Icinga 2 service, run `services.msc` and restart the
`icinga2` service. Alternatively you can use the `net {start,stop}` CLI commands.

Now that you've succesfully installed a satellite/client please proceed to the
[configuration modes](6-distributed-monitoring.md#distributed-monitoring-configuration-modes).

## <a id="distributed-monitoring-configuration-modes"></a> Configuration Modes

There are different ways to ensure that the Icinga 2 cluster nodes execute
checks, send notifications, etc.

The following modes differ in the way the host/service object
configuration is synchronized among nodes and checks are executed.

* [Top down](6-distributed-monitoring.md#distributed-monitoring-top-down). This mode syncs the configuration and commands from the master into child zones.
* [Bottom up](6-distributed-monitoring.md#distributed-monitoring-bottom-up). This mode leaves the configuration on the child nodes and requires an import on the parent nodes.

Read the chapter carefully and decide upon your requirements which way fits
best for your environments. You should not mix them -- that will overly complicate
your setup.

Check results are synced all the way up from the child nodes to the parent nodes.
That happens automatically and is ensured by the cluster protocol.

### <a id="distributed-monitoring-top-down"></a> Top Down

This is the most commonly used mode gathered from community feedback.

There are two different behaviours with check execution:

* Send a command execution event remotely, the scheduler still runs on the parent node
* Sync the host/service objects directly to the child node, checks are executed locally

Again -- technically it does not matter whether this is a `client` or a `satellite`
which is receiving configuration or command execution events.
But following our [mode definition](#distributed-monitoring-roles), a `client` will just run commands and would not need configuration objects.


### <a id="distributed-monitoring-top-down-command-endpoint"></a> Top Down Command Endpoint (aka Client, Agent)

This mode will force the Icinga 2 node to execute commands remotely on a specified endpoint.
The host/service object configuration is located on the master/satellite and the client only
needs the CheckCommand object definitions used.

![Icinga 2 Distributed Top Down Command Endpoint](images/distributed-monitoring/icinga2_distributed_top_down_command_endpoint.png)

Advantages:

* No local checks defined on the child node (client)
* Light-weight remote check execution (asynchronous events)
* No replay log necessary on child node disconnect (ensure to set `log_duration=0` on the parent node)
* Pin checks to specific endpoints (if the child zone consists of 2 endpoints)

Disadvantages:

* If the child node is not connected, no more checks are executed
* Requires additional configuration attribute specified in host/service objects
* Requires local `CheckCommand` object configuration. Best practice is to use a [global config zone](6-distributed-monitoring.md#distributed-monitoring-global-zone-config-sync).

In order that all involved nodes will accept configuration and/or
commands you'll need to configure the `Zone` and `Endpoint` hierarchy
on all nodes.

* `icinga2-master1.localdomain` is the configuration master in this scenario.
* `icinga2-client2.localdomain` acts as client which receives command execution messages via command endpoint from the master. In addition it receives global check command configuration from the master.

Put the endpoint and zone configuration on **both** nodes into `/etc/icinga2/zones.conf`.

The endpoint configuration could look like this:

    object Endpoint "icinga2-master1.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-client2.localdomain" {
      host = "192.168.56.112"
    }

Then you'll need to define two zones. There is no naming convention but best practice
is to either use `master`, `satellite`/`client-fqdn` or go by region names.

> **Note**
>
> Each client requires its own zone and endpoint configuration. Best practice
> has been to use the client's FQDN for all object names.

The `master` zone is a parent of the `icinga2-client2.localdomain` zone.

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain" ] //array with endpoint names
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]

      parent = "master" //establish zone hierarchy
    }

In addition to that add a [global zone](6-distributed-monitoring.md#distributed-monitoring-global-zone-config-sync)
for syncing check commands later.

    object Zone "global-templates" {
      global = true
    }

Edit the `api` feature on the client `icinga2-client2.localdomain` in
the `/etc/icinga2/features-enabled/api.conf` file and ensure to set
`accept_commands` and `accept_config` to `true`.

    [root@icinga2-client1.localdomain /]# vim /etc/icinga2/features-enabled/api.conf

    object ApiListener "api" {
       //...
       accept_commands = true
       accept_config = true
    }

Now it is time to validate the configuration and restart the Icinga 2 daemon
on both nodes.

Example on CentOS 7:

    [root@icinga2-client2.localdomain /]# icinga2 daemon -C
    [root@icinga2-client2.localdomain /]# systemctl restart icinga2

    [root@icinga2-master1.localdomain /]# icinga2 daemon -C
    [root@icinga2-master1.localdomain /]# systemctl restart icinga2

Once the clients have connected you are ready for the next step - **execute
a remote check on the client using the command endpoint**.

Put the host and service object configuration into the `master` zone
-- this will help adding a secondary master for High-Availability later.

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/master

Add the host and service objects you want to monitor. There is
no limitation with files and directories -- best practice is to
keep things organised by type.

By convention a master/satellite/client host object should use the same name as the endpoint object.
You can also add multiple hosts which execute checks against remote services/clients.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/master
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim hosts.conf

    object Host "icinga2-client2.localdomain" {
      check_command = "hostalive" //check is executed on the master
      address = "192.168.56.112"

      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

Given that you are monitoring a Linux client we'll just add a remote [disk](10-icinga-template-library.md#plugin-check-command-disk)
check.

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim services.conf

    apply Service "disk" {
      check_command = "disk"

      //specify where the check is executed
      command_endpoint = host.vars.client_endpoint

      assign where host.vars.client_endpoint
    }

In case that you have your own custom CheckCommand add it to the global zone.

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/global-templates
    [root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/global-templates/commands.conf

    object CheckCommand "my-cmd" {
      import "plugin-check-command"

      //...
    }

Save and validate the configuration on the master node.

    [root@icinga2-master1.localdomain /]# icinga2 daemon -C

Restart the Icinga 2 daemon (example for CentOS 7):

    [root@icinga2-master1.localdomain /]# systemctl restart icinga2

Now the following happens:

* Icinga 2 validates the configuration on `icinga2-master1.localdomain` and restarts.
* The `icinga2-master1.localdomain` node schedules and executes the checks.
* The `icinga2-client2.localdomain` node receives the execute command event with additional command parameters.
* The `icinga2-client2.localdomain` node maps the command parameters onto the local check command, executes the check locally and sends back the check result message.

You'll see - no reload or any interaction required on the client
itself.

Now you have learned the basics about command endpoint checks. Proceed in
the [scenarios](6-distributed-monitoring.md#distributed-monitoring-scenarios)
chapter for more details on extending the setup.


### <a id="distributed-monitoring-top-down-config-sync"></a> Top Down Config Sync

This mode syncs the object configuration files within specified zones.
This comes in handy if you want to configure everything on the master node
and sync the satellite checks (disk, memory, etc.). The satellites run their
own local scheduler and will send the check result messages back to the master.

![Icinga 2 Distributed Top Down Config Sync](images/distributed-monitoring/icinga2_distributed_top_down_config_sync.png)

Advantages:

* Sync the configuration files from the parent zone to the child zones.
* No manual restart required on the child nodes - sync, validation and restarts happen automatically.
* Execute checks directly on the child node's scheduler.
* Replay log if the connection drops (important for keeping the check history in sync, e.g. for SLA reports).
* Use a global zone for syncing templates, groups, etc.

Disadvantages:

* Requires a config directory on the master node with the zone name underneath `/etc/icinga2/zones.d`.
* Additional zone and endpoint configuration.
* Replay log is replicated on reconnect. This might generate an overload on the used connection.

In order that all involved nodes will accept configuration and/or
commands you'll need to configure the `Zone` and `Endpoint` hierarchy
on all nodes.

* `icinga2-master1.localdomain` is the configuration master in this scenario.
* `icinga2-client1.localdomain` acts as client which receives configuration from the master.

Put the endpoint and zone configuration on **both** nodes into `/etc/icinga2/zones.conf`.

The endpoint configuration could look like this:

    object Endpoint "icinga2-master1.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-client1.localdomain" {
      host = "192.168.56.111"
    }

Then you'll need to define two zones. There is no naming convention but best practice
is to either use `master`, `satellite`/`client-fqdn` or go by region names.

> **Note**
>
> Each client requires its own zone and endpoint configuration. Best practice
> has been to use the client's FQDN for all object names.

The `master` zone is a parent of the `icinga2-client1.localdomain` zone.

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain" ] //array with endpoint names
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]

      parent = "master" //establish zone hierarchy
    }

Edit the `api` feature on the client `icinga2-client1.localdomain` in
the `/etc/icinga2/features-enabled/api.conf` file and ensure to set
`accept_config` to `true`.

    [root@icinga2-client1.localdomain /]# vim /etc/icinga2/features-enabled/api.conf

    object ApiListener "api" {
       //...
       accept_config = true
    }

Now it is time to validate the configuration and restart the Icinga 2 daemon
on both nodes.

Example on CentOS 7:

    [root@icinga2-client1.localdomain /]# icinga2 daemon -C
    [root@icinga2-client1.localdomain /]# systemctl restart icinga2

    [root@icinga2-master1.localdomain /]# icinga2 daemon -C
    [root@icinga2-master1.localdomain /]# systemctl restart icinga2


> **Tip**
>
> Best practice is to use a [global zone](6-distributed-monitoring.md#distributed-monitoring-global-zone-config-sync)
> for common configuration items (check commands, templates, groups, etc.).

Once the clients have connected you are ready for the next step - **execute
a local check on the client using the configuration sync**.

Therefore navigate into `/etc/icinga2/zones.d` on your config master
`icinga2-master1.localdomain` and create a new directory with the same
name as your satellite/client zone name.

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/icinga2-client1.localdomain

Add the host and service objects you want to monitor. There is
no limitation with files and directories -- best practice is to
keep things organised by type.

By convention a master/satellite/client host object should use the same name as the endpoint object.
You can also add multiple hosts which execute checks against remote services/clients.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/icinga2-client1.localdomain
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/icinga2-client1.localdomain]# vim hosts.conf

    object Host "icinga2-client1.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.111"
      zone = "master" //optional trick: sync the required host object to the client, but enforce the "master" zone to execute the check
    }

Given that you are monitoring a Linux client we'll just add a local [disk](10-icinga-template-library.md#plugin-check-command-disk)
check.

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/icinga2-client1.localdomain]# vim services.conf

    object Service "disk" {
      host_name = "icinga2-client1.localdomain"

      check_command = "disk"
    }

Save and validate the configuration on the master node.

    [root@icinga2-master1.localdomain /]# icinga2 daemon -C

Restart the Icinga 2 daemon (example for CentOS 7):

    [root@icinga2-master1.localdomain /]# systemctl restart icinga2

Now the following happens:

* Icinga 2 validates the configuration on `icinga2-master1.localdomain`
* Icinga 2 copies the configuration into its zone config store in `/var/lib/icinga2/api/zones`
* The `icinga2-master1.localdomain` node sends a config update event to all endpoints in the same or direct child zones.
* The `icinga2-client1.localdomain` node accepts config and populates the local zone config store with the received config files.
* The `icinga2-client1.localdomain` node validates the configuration and automatically restarts.

You'll see - no reload or any interaction required on the client
itself.

You can also use the config sync inside a High-Availability zone to
ensure that all config objects are synced among zone members.

> **Note**
>
> You can only have one so-called "config master" in a zone which stores
> configuration in `zones.d`. Everything else breaks and is not supported.

Now you have learned the basics about the configuration sync. Proceed in
the [scenarios](6-distributed-monitoring.md#distributed-monitoring-scenarios)
chapter for more details on extending the setup.


### <a id="distributed-monitoring-bottom-up"></a> Bottom Up Import

This mode requires you to manage the configuration on the client itself.
Edit the configuration files underneath `/etc/icinga2/conf.d` or any other
directory included in the `icinga2.conf` file.

The client will send the configured objects to the parent zone members
where they can generate configuration objects gathered from that information.

![Icinga 2 Distributed Bottom Up](images/distributed-monitoring/icinga2_distributed_bottom_up.png)

Advantages:

* Each child node comes preconfigured with the most common local checks.
* Central repository for zones, endpoints, hosts and services with configuration repository import.

Disadvantages:

* No object attribute sync. Parent nodes cannot filter for specific attributes in assign expressions.
* Does not reliably work with a HA parent zone (single master preferred).
* Configuration management of many client nodes is hard or impossible if you don't have access to them.

You can list and import the configuration sent from clients on the master
node. Example for listing all client services on the master node `icinga2-master1.localdomain`:

    [root@icinga2-master1.localdomain /]# icinga2 node list
    Node 'icinga2-client1.localdomain' (last seen: Sun Aug 14 11:19:14 2016)
        * Host 'icinga2-client1.localdomain'
            * Service 'disk'
            * Service 'disk /'
            * Service 'http'
            * Service 'icinga'
            * Service 'load'
            * Service 'ping4'
            * Service 'ping6'
            * Service 'procs'
            * Service 'ssh'
            * Service 'swap'
            * Service 'users'
    
    Node 'DESKTOP-IHRPO96' (last seen: Sun Aug 14 11:19:14 2016)
        * Host 'DESKTOP-IHRPO96'
            * Service 'disk'
            * Service 'disk C:'
            * Service 'icinga'
            * Service 'load'
            * Service 'ping4'
            * Service 'ping6'
            * Service 'procs'
            * Service 'swap'
            * Service 'users'

The object configuration must exist on the master node as well
in order to receive check results from the clients. Therefore
you'll need to invoke the `node update-config` cli command.

    [root@icinga2-master1.localdomain /]# icinga2 node update-config
    information/cli: Updating node configuration for
    ...

The generated configuration objects are located in `/etc/icinga2/repository.d`.
If you have accidentally added specific hosts or services you can safely purge
them from this directory and restart icinga 2.

In case you want to blacklist or whitelist several hosts and/or services
to not generate configuration on the master, use the `icinga2 node {black,white}list`
cli commands.

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

There are certain limitations with this mode. Currently the repository
does not sync object attributes (custom attributes, group memberships)
from the client to the master.

You can manually edit the configuration in `/etc/icinga2/repository.d`
and fix it. That will help with additional notification apply rules
or group memberships required for Icinga Web 2 and addons.


## <a id="distributed-monitoring-scenarios"></a> Scenarios

These examples should give you an idea how you can build your own
distributed monitoring environment. We've seen them all in production
environments and received feedback from our [community](https://www.icinga.org/community/get-help/)
and [partner support](https://www.icinga.org/services/support/) channels.

* Single master with clients
* HA master with clients as command endpoint
* Three level cluster with config HA masters, satellites receiving config sync and clients checked using command_endpoint

### <a id="distributed-monitoring-master-clients"></a> Master with Clients

![Icinga 2 Distributed Master with Clients](images/distributed-monitoring/icinga2_distributed_scenarios_master_clients.png)

* `icinga2-master1.localdomain` is the primary master node
* `icinga2-client1.localdomain` and `icinga2-client2.localdomain` are two child nodes as clients

Setup requirements:

* Install `icinga2-master1.localdomain` as [master setup](6-distributed-monitoring.md#distributed-monitoring-setup-master)
* Install `icinga2-client1.localdomain` and `icinga2-client2.localdomain` as [client setup](6-distributed-monitoring.md#distributed-monitoring-setup-satellite-client)

Edit the `zones.conf` configuration file on the master:

    [root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
    }

    object Endpoint "icinga2-client1.localdomain" {
      host = "192.168.33.111" //the master actively tries to connect to the client
    }

    object Endpoint "icinga2-client2.localdomain" {
      host = "192.168.33.112" //the master actively tries to connect to the client
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain" ]
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

The two client nodes do not necessarily need to know about each other. The only important thing
is that they know about the parent zone and their endpoint members and optional the global zone.

If you specify the `host` attribute in the `icinga2-master1.localdomain` endpoint object
the client will actively try to connect to the master node. Since we've specified the client
endpoint's attribute on the master node already, we don't want the clients to connect to the
master. Choose one connection direction.

    [root@icinga2-client1.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
      //do not actively connect to the master by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-client1.localdomain" {
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain" ]
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

    [root@icinga2-client2.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
      //do not actively connect to the master by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-client2.localdomain" {
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain" ]
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

Now it is time to define the two client hosts and apply service checks using
the command endpoint execution method to them. Note: You can also use the
config sync mode here.

Create a new configuration directory on the master node.

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/master

Add the two client nodes as host objects.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/master
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim hosts.conf

    object Host "icinga2-client1.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.111"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

    object Host "icinga2-client2.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.112"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

Add services using command endpoint checks.

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim services.conf

    apply Service "ping4" {
      check_command = "ping4"
      //check is executed on the master node
      assign where host.address
    }

    apply Service "disk" {
      check_command = "disk"

      //specify where the check is executed
      command_endpoint = host.vars.client_endpoint

      assign where host.vars.client_endpoint
    }

Validate the configuration and restart Icinga 2 on the master node `icinga2-master1.localdomain`.

Open Icinga Web 2 and check the 2 newly created clients hosts with two new services
-- one executed locally (`ping4`) and one using command endpoint (`disk`).

### <a id="distributed-monitoring-scenarios-ha-master-clients"></a> High-Availability Master with Clients

![Icinga 2 Distributed High Availability Master with Clients](images/distributed-monitoring/icinga2_distributed_scenarios_ha_master_clients.png)

This scenario is quite the same as you have already found in the [chapter before](6-distributed-monitoring.md#distributed-monitoring-master-clients).

The real difference is that we will now setup two master nodes in a High-Availablity setup.
These nodes must be configured into zone and endpoints objects.

This scenario uses the capabilities of the Icinga 2 cluster. All zone members
replicate cluster events amongst each other. In addition to that several Icinga 2
features can enable HA functionality.

> **Notes**
> All nodes in the same zone require the same features enabled for High Availability (HA)
> amongst them.

Overview:

* `icinga2-master1.localdomain` is the config master master node
* `icinga2-master2.localdomain` is the secondary master master node without config in `zones.d`
* `icinga2-client1.localdomain` and `icinga2-client2.localdomain` are two child nodes as clients

Setup requirements:

* Install `icinga2-master1.localdomain` as [master setup](6-distributed-monitoring.md#distributed-monitoring-setup-master)
* Install `icinga2-master2.localdomain` as [client setup](6-distributed-monitoring.md#distributed-monitoring-setup-satellite-client) (we will modify the generated configuration)
* Install `icinga2-client1.localdomain` and `icinga2-client2.localdomain` as [client setup](6-distributed-monitoring.md#distributed-monitoring-setup-satellite-client) (when asked for adding multiple masters, tick 'y' and add the secondary master `icinga2-master2.localdomain`).

In case you not want to use the cli commands you can also manually create and sync the
required SSL certificates. We will modify and discuss the generated configuration here
in detail.

Since there are now two nodes in the same zone we must consider the
[high-availability features](6-distributed-monitoring.md#distributed-monitoring-high-availability-features).

* Checks and notifiations are balanced between the two master nodes. That's fine but requires check plugins and notification scripts to exist on both nodes.
* The IDO feature will only be active on one node by default. Since all events are replicated between both nodes it is easier to just have one central database.

Decide whether you want to use a dedicated MySQL cluster VIP (external application cluster)
and leave the IDO feature with enabled HA capabilities. Or you'll configure the feature to
disable HA and write to a local installed database on each node. Both implementation methods
require you to configure Icinga Web 2 accordingly (Monitoring backend - IDO database, used transports).

The zone hierarchy could look like this. It involves putting the two master nodes
`icinga2-master1.localdomain` and `icinga2-master2.localdomain` into the `master` zone.

    [root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-master2.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-client1.localdomain" {
      host = "192.168.33.111" //the master actively tries to connect to the client
    }

    object Endpoint "icinga2-client2.localdomain" {
      host = "192.168.33.112" //the master actively tries to connect to the client
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain", "icinga2-master1.localdomain" ]
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

The two client nodes do not necessarily need to know about each other. The only important thing
is that they know about the parent zone and their endpoint members and optional the global zone.

If you specify the `host` attribute in the `icinga2-master1.localdomain` and `icinga2-master2.localdomain`
endpoint objects the client will actively try to connect to the master node. Since we've specified the client
endpoint's attribute on the master node already, we don't want the clients to connect to the
master nodes. Choose one connection direction.

    [root@icinga2-client1.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
      //do not actively connect to the master by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-master2.localdomain" {
      //do not actively connect to the master by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-client1.localdomain" {
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain", "icinga2-master2.localdomain" ]
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

    [root@icinga2-client2.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
      //do not actively connect to the master by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-master2.localdomain" {
      //do not actively connect to the master by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-client2.localdomain" {
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain", "icinga2-master2.localdomain" ]
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

Now it is time to define the two client hosts and apply service checks using
the command endpoint execution method to them. Note: You can also use the
config sync mode here.

Create a new configuration directory on the master node `icinga2-master1.localdomain`.
**Note**: The secondary master node `icinga2-master2.localdomain` receives the
configuration using the [config sync mode](6-distributed-monitoring.md#distributed-monitoring-top-down-config-sync).

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/master

Add the two client nodes as host objects.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/master
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim hosts.conf

    object Host "icinga2-client1.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.111"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

    object Host "icinga2-client2.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.112"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

Add services using command endpoint checks.

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim services.conf

    apply Service "ping4" {
      check_command = "ping4"
      //check is executed on the master node
      assign where host.address
    }

    apply Service "disk" {
      check_command = "disk"

      //specify where the check is executed
      command_endpoint = host.vars.client_endpoint

      assign where host.vars.client_endpoint
    }

Validate the configuration and restart Icinga 2 on the master node `icinga2-master1.localdomain`.

Open Icinga Web 2 and check the 2 newly created clients hosts with two new services
-- one executed locally (`ping4`) and one using command endpoint (`disk`).

In addition to that you should add [health checks](6-distributed-monitoring.md#distributed-monitoring-health-checks)
ensuring that your cluster notifies you in case of failure.


### <a id="distributed-monitoring-scenarios-master-satellite-client"></a> Three Levels with Master, Satellites and Clients

![Icinga 2 Distributed Master and Satellites with Clients](images/distributed-monitoring/icinga2_distributed_scenarios_master_satellite_client.png)

This scenario combines everything you've learned so far. High-availability masters,
satellites receiving their config from the master zone, clients checked via command
endpoint from the satellite zones.

> **Tip**
>
> It can get complicated so take pen and paper and bring your thoughts to life.
> Play around with a test setup before putting such a thing into production too!

Overview:

* `icinga2-master1.localdomain` is the config master master node
* `icinga2-master2.localdomain` is the secondary master master node without config in `zones.d`
* `icinga2-satellite1.localdomain` and `icinga2-satellite2.localdomain` are satellite nodes in a `master` child zone
* `icinga2-client1.localdomain` and `icinga2-client2.localdomain` are two child nodes as clients

Setup requirements:

* Install `icinga2-master1.localdomain` as [master setup](6-distributed-monitoring.md#distributed-monitoring-setup-master)
* Install `icinga2-master2.localdomain`, `icinga2-satellite1.localdomain` and `icinga2-satellite2.localdomain` as [client setup](6-distributed-monitoring.md#distributed-monitoring-setup-satellite-client) (we will modify the generated configuration)
* Install `icinga2-client1.localdomain` and `icinga2-client2.localdomain` as [client setup](6-distributed-monitoring.md#distributed-monitoring-setup-satellite-client)

Once you are asked for the master endpoint providing CSR auto-signing capabilities
please add the master node which holds the CA and has the ApiListener feature configured.
The parent endpoint must still remain the satellite endpoint name.

Example for `icinga2-client1.localdomain`:

    Please specify the master endpoint(s) this node should connect to:

"master" is the first satellite `icinga2-satellite1.localdomain`.

    Master Common Name (CN from your master setup): icinga2-satellite1.localdomain
    Do you want to establish a connection to the master from this node? [Y/n]: y
    Please fill out the master connection information:
    Master endpoint host (Your master's IP address or FQDN): 192.168.56.105
    Master endpoint port [5665]:

Add more "masters", the second satellite `icinga2-satellite2.localdomain`.

    Add more master endpoints? [y/N]: y
    Master Common Name (CN from your master setup): icinga2-satellite2.localdomain
    Do you want to establish a connection to the master from this node? [Y/n]: y
    Please fill out the master connection information:
    Master endpoint host (Your master's IP address or FQDN): 192.168.56.106
    Master endpoint port [5665]:
    Add more master endpoints? [y/N]: n

Specify the master node `icinga2-master2.localdomain`with the CA private key and ticket salt configured.

    Please specify the master connection for CSR auto-signing (defaults to master endpoint host):
    Host [192.168.56.106]: icinga2-master1.localdomain
    Port [5665]:

In case you cannot connect to the master node from your clients, you'll manually need
to [generate the SSL certificates](6-distributed-monitoring.md#distributed-monitoring-advanced-hints-certificates)
and modify the configuration.

We'll discuss the required configuration in detail below.

The zone hierarchy can look like this. We'll define only the directly connected zones here.

You can safely deploy this configuration onto all master and satellite zone
members. You should keep in mind to control the endpoint connection direction
using the `host` attribute.

    [root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-master1.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-master2.localdomain" {
      host = "192.168.56.101"
    }

    object Endpoint "icinga2-satellite1.localdomain" {
      host = "192.168.56.105"
    }

    object Endpoint "icinga2-satellite2.localdomain" {
      host = "192.168.56.106"
    }

    object Zone "master" {
      endpoints = [ "icinga2-master1.localdomain", "icinga2-master1.localdomain" ]
    }

    object Zone "satellite" {
      endpoints = [ "icinga2-satellite1.localdomain", "icinga2-satellite1.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

Note: The master nodes do not need to know about the indirectly connected clients
for connection reasons. But since we want to use command endpoint check configuration,
we'll need them. In order to maximize the effort, we'll sync the client zone and endpoint
config to the satellites where the connection information is needed as well.

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/{master,satellite,global-templates}
    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/satellite

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/satellite]# vim icinga2-client1.localdomain.conf

    object Endpoint "icinga2-client1.localdomain" {
      host = "192.168.33.111" //the satellite actively tries to connect to the client
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]
    }

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/satellite]# vim icinga2-client2.localdomain.conf

    object Endpoint "icinga2-client2.localdomain" {
      host = "192.168.33.112" //the satellite actively tries to connect to the client
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]
    }

The two client nodes themselves do not necessarily need to know about each other. The only important thing
is that they know about the parent zone and their endpoint members and optional the global zone.

If you specify the `host` attribute in the `icinga2-master1.localdomain` and `icinga2-master2.localdomain`
endpoint objects the client will actively try to connect to the master node. Since we've specified the client
endpoint's attribute on the master node already, we don't want the clients to connect to the
master nodes. Choose one connection direction.

    [root@icinga2-client1.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-satellite1.localdomain" {
      //do not actively connect to the satellite by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-satellite2.localdomain" {
      //do not actively connect to the satellite by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-client1.localdomain" {
    }

    object Zone "satellite" {
      endpoints = [ "icinga2-satellite1.localdomain", "icinga2-satellite2.localdomain" ]
    }

    object Zone "icinga2-client1.localdomain" {
      endpoints = [ "icinga2-client1.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

    [root@icinga2-client2.localdomain /]# vim /etc/icinga2/zones.conf

    object Endpoint "icinga2-satellite1.localdomain" {
      //do not actively connect to the satellite by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-satellite2.localdomain" {
      //do not actively connect to the satellite by leaving out the 'host' attribute
    }

    object Endpoint "icinga2-client2.localdomain" {
    }

    object Zone "satellite" {
      endpoints = [ "icinga2-satellite1.localdomain", "icinga2-satellite2.localdomain" ]
    }

    object Zone "icinga2-client2.localdomain" {
      endpoints = [ "icinga2-client2.localdomain" ]
    }

    /* sync global commands */
    object Zone "global-templates" {
      global = true
    }

Now it is time to define the two client hosts on the master, sync them to the satellites
 and apply service checks using the command endpoint execution method to them.

Add the two client nodes as host objects into the `satellite` zone.
**Note**: We've previously created the directories in `zones.d` and files for the
zone and endpoint configuration for the clients already.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/satellite
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/satellite]# vim icinga2-client1.localdomain.conf

    object Host "icinga2-client1.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.111"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/satellite]# vim icinga2-client2.localdomain.conf

    object Host "icinga2-client2.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.112"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

Add services using command endpoint checks. Pin the apply rules to the `satellite` zone only.

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/satellite]# vim services.conf

    apply Service "ping4" {
      check_command = "ping4"
      //check is executed on the satellite node
      assign where host.zone == "satellite" && host.address
    }

    apply Service "disk" {
      check_command = "disk"

      //specify where the check is executed
      command_endpoint = host.vars.client_endpoint

      assign where host.zone == "satellite" && host.vars.client_endpoint
    }

Validate the configuration and restart Icinga 2 on the master node `icinga2-master1.localdomain`.

Open Icinga Web 2 and check the 2 newly created clients hosts with two new services
-- one executed locally (`ping4`) and one using command endpoint (`disk`).

In addition to that you should add [health checks](6-distributed-monitoring.md#distributed-monitoring-health-checks)
ensuring that your cluster notifies you in case of failure.


## <a id="distributed-monitoring-best-practice"></a> Best Practice

A collection of best practices we've learned from the community.

Join the [community channels](https://www.icinga.org/community/get-help/)
and share your tips and tricks with us!

### <a id="distributed-monitoring-global-zone-config-sync"></a> Global Zone for Config Sync

The idea behind a global zone is not to add endpoints to it. That would
not work with the implemented cluster hierarchy.

It was rather designed with the problem in mind - the configuration synced
to each node must be valid. What if my templates and check commands are
only available on the master node?

Therefore it is possible to use the config sync mode with a global zone.

The zone object configuration must be deployed on all nodes which should receive
the global configuration files.

    object Zone "global-templates" {
      global = true
    }

Similar to the zone configuration sync you'll need to create a new directory in
`/etc/icinga2/zones.d`

    [root@icinga2-master1.localdomain /]# mkdir -p /etc/icinga2/zones.d/global-templates

Then add a new check command for example.

    [root@icinga2-master1.localdomain /]# vim /etc/icinga2/zones.d/global-templates/commands.conf

    object CheckCommand "my-cmd" {
      import "plugin-check-command"

      //...
    }

Restart the client(s) which should receive the global zone first.

Then validate the configuration on the master node and restart Icinga 2.

**Note**: Host/Service objects must not be put into a global zone. The configuration
validation will throw an error.

### <a id="distributed-monitoring-health-checks"></a> Health Checks

In case of network failures or any other problem your monitoring could
either have late check results or just send out mass alarms for unknown
checks.

In order to minimize the problems caused by this you should configure
additional health checks.

The `cluster` check will check if all endpoints in the current zone and the directly
connected zones are working properly.

    object Service "cluster" {
        check_command = "cluster"
        check_interval = 5s
        retry_interval = 1s

        host_name = "icinga2-master1.localdomain"
    }

The `cluster-zone` check will test whether the configured target zone is currently
connected or not.

    apply Service "child-health" {
      check_command = "cluster-zone"

      /* This follows the convention that the client zone name is the FQDN which is the same as the host object name. */
      vars.cluster_zone = host.name

      assign where host.vars.has_client
    }

In case you cannot assign the `cluster_zone` attribute that generic add specific
checks to your cluster.

    object Service "cluster-zone-satellite" {
      check_command = "cluster-zone"
      check_interval = 5s
      retry_interval = 1s
      vars.cluster_zone = "satellite"

      host_name = "icinga2-master1.localdomain"
    }


In case you are using top down checks with command endpoint configuration you can
for example add a dependency which prevents notifications for all other failing services:

    apply Dependency "health-check" to Service {
      parent_service_name = "child-health"

      states = [ OK ]
      disable_notifications = true

      assign where host.vars.has_client
      ignore where service.name == "child-health"
   }

### <a id="distributed-monitoring-windows-plugins"></a> Windows Client and Plugins

The Icinga 2 package on Windows already provides several plugins. There is
a detailed documentation for all available check command definitions over [here](10-icinga-template-library.md#windows-plugins).

Add the following inclusion on all your nodes (master, satellite, client):

    vim /etc/icinga2/icinga2.conf

    include <windows-plugins>

Based on the [master with clients](6-distributed-monitoring.md#distributed-monitoring-master-clients)
scenario we'll now add a local disk check.

Add the client node as host object.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/master
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim hosts.conf

    object Host "icinga2-client1.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.111"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
      vars.os_type = "windows"
    }

Add the disk check using command endpoint checks (details in the
[disk-windows](10-icinga-template-library.md#windows-plugins-disk-windows) documentation).

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim services.conf

    apply Service "disk C:" {
      check_command = "disk-windows"

      vars.disk_win_path = "C:"

      //specify where the check is executed
      command_endpoint = host.vars.client_endpoint

      assign where host.vars.os_type == "windows" && host.vars.client_endpoint
    }


### <a id="distributed-monitoring-windows-nscp"></a> Windows Client and NSClient++

The [Windows setup](6-distributed-monitoring.md#distributed-monitoring-setup-client-windows) already allows
you to install the NSClient++ package. In addition to the Windows plugins you can
also use the [nscp-local commands](10-icinga-template-library.md#nscp-plugin-check-commands)
provided by the Icinga Template Library (ITL).

![Icinga 2 Distributed Monitoring Windows Client with NSClient++](images/distributed-monitoring/icinga2_distributed_windows_nscp.png)

Add the following inclusion on all your nodes (master, satellite, client):

    vim /etc/icinga2/icinga2.conf

    include <nscp-local>

The CheckCommand definitions will automatically determine the installed path
to the `nscp.exe` binary.

Based on the [master with clients](6-distributed-monitoring.md#distributed-monitoring-master-clients)
scenario we'll now add a local nscp check querying a given performance counter.

Add the client node as host object.

    [root@icinga2-master1.localdomain /]# cd /etc/icinga2/zones.d/master
    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim hosts.conf

    object Host "icinga2-client1.localdomain" {
      check_command = "hostalive"
      address = "192.168.56.111"
      vars.client_endpoint = host.name //follows the convention host name == endpoint name
    }

Add performance counter check using command endpoint checks (details in the
[nscp-local-counter](10-icinga-template-library.md#nscp-check-local-counter) documentation).

    [root@icinga2-master1.localdomain /etc/icinga2/zones.d/master]# vim services.conf

    apply Service "perf-counter-cpu" {
      check_command = "nscp-local-counter"

      vars.nscp_local_counter = "\\Processor(_total)\\% Processor Time"
      vars.nscp_local_perfsyntax = "Total Processor Time"
      vars.nscp_local_warning = 1
      vars.nscp_local_critical = 5

      //specify where the check is executed
      command_endpoint = host.vars.client_endpoint

      assign where host.vars.client_endpoint
    }


## <a id="distributed-monitoring-advanced-hints"></a> Advanced Hints

You can find additional hints in this section if you prefer to go your own route
with automating setups (setup, certificates, configuration).

### <a id="distributed-monitoring-high-availability-features"></a> High Availability for Icinga 2 features

All nodes in the same zone require the same features enabled for High Availability (HA)
amongst them.

By default the following features provide advanced HA functionality:

* [Checks](6-distributed-monitoring.md#distributed-monitoring-high-availability-checks) (load balanced, automated failover)
* [Notifications](6-distributed-monitoring.md#distributed-monitoring-high-availability-notifications) (load balanced, automated failover)
* [DB IDO](6-distributed-monitoring.md#distributed-monitoring-high-availability-db-ido) (Run-Once, automated failover)

#### <a id="distributed-monitoring-high-availability-checks"></a> High Availability with Checks

All instances within the same zone (e.g. the `master` zone as HA cluster) must
have the `checker` feature enabled.

Example:

    # icinga2 feature enable checker

All nodes in the same zone load-balance the check execution. When one instance shuts down
the other nodes will automatically take over the remaining checks.

#### <a id="distributed-monitoring-high-availability-notifications"></a> High Availability with Notifications

All instances within the same zone (e.g. the `master` zone as HA cluster) must
have the `notification` feature enabled.

Example:

    # icinga2 feature enable notification

Notifications are load balanced amongst all nodes in a zone. By default this functionality
is enabled.
If your nodes should notify independent from any other nodes (this will cause
duplicated notifications if not properly handled!), you can set `enable_ha = false`
in the [NotificationComponent](9-object-types.md#objecttype-notificationcomponent) feature.

#### <a id="distributed-monitoring-high-availability-db-ido"></a> High Availability with DB IDO

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
> for the [IdoMysqlConnection](9-object-types.md#objecttype-idomysqlconnection) or
> [IdoPgsqlConnection](9-object-types.md#objecttype-idopgsqlconnection) object on **all** nodes in the
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


### <a id="distributed-monitoring-advanced-hints-windows-silent"></a> Silent Windows Setup

If you want to install the client silently/unattended, use the `/qn` modifier. The
installation should not trigger a restart but if you want to be completly sure you can use the `/norestart` modifier.

    C:> msiexec /i C:\Icinga2-v2.5.0-x86.msi /qn /norestart

### <a id="distributed-monitoring-advanced-hints-certificates"></a> Manual Certificate Creation

Choose the host which should store the certificate authority (one of the master nodes).

The first step is the creation of the certificate authority (CA) by running the following command
as root user:

    icinga2 pki new-ca

Create a certificate signing request (CSR) for each node.

    icinga2 pki new-cert --cn icinga2-master1.localdomain --key icinga2-master1.localdomain.key --csr icinga2-master1.localdomain.csr

Sign the CSR with the previously created CA.

    icinga2 pki sign-csr --csr icinga2-master1.localdomain.csr --cert icinga2-master1.localdomain

Copy the host's certificate files and the public CA certificate to `/etc/icinga2/pki`.

    mkdir -p /etc/icinga2/pki
    cp icinga2-master1.localdomain.{crt,key} /etc/icinga2/pki
    cp /var/lib/icinga2/ca/ca.crt /etc/icinga2/pki

Ensure that proper permissions are set (replace `icinga` with the Icinga 2 daemon user).

    chown -R icinga:icinga /etc/icinga2/pki
    chmod 600 /etc/icinga2/pki/*.key
    chmod 644 /etc/icinga2/pki/*.crt

The CA public and private key are stored in the `/var/lib/icinga2/ca` directory. Keep this path secure and include
it in your backups.

Example for creating multiple certificates at once:

    # for node in icinga2-master1.localdomain icinga2-master2.localdomain icinga2-satellite1.localdomain; do sudo icinga2 pki new-cert --cn $node --csr $node.csr --key $node.key; done
    information/base: Writing private key to 'icinga2-master1.localdomain.key'.
    information/base: Writing certificate signing request to 'icinga2-master1.localdomain.csr'.
    information/base: Writing private key to 'icinga2-master2.localdomain.key'.
    information/base: Writing certificate signing request to 'icinga2-master2.localdomain.csr'.
    information/base: Writing private key to 'icinga2-satellite1.localdomain.key'.
    information/base: Writing certificate signing request to 'icinga2-satellite1.localdomain.csr'.

    # for node in icinga2-master1.localdomain icinga2-master2.localdomain icinga2-satellite1.localdomain; do sudo icinga2 pki sign-csr --csr $node.csr --cert $node.crt; done
    information/pki: Writing certificate to file 'icinga2-master1.localdomain.crt'.
    information/pki: Writing certificate to file 'icinga2-master2.localdomain.crt'.
    information/pki: Writing certificate to file 'icinga2-satellite1.localdomain.crt'.

### <a id="distributed-monitoring-advanced-hints-cli-node-setup"></a> Node Setup Cli Command

Instead of using the `node wizard` cli command, there is an alternative `node setup`
cli command available which has some pre-requisites. Make sure that the
`/etc/icinga2/pki` exists and is owned by the `icinga` user (or the user Icinga 2 is
running as).

Required information:

* The client common name (CN). Use the FQDN, e.g. `icinga2-node2.localdomain`.
* The master host and zone name. Pass that to `pki save-cert` as `--host` parameter for example.
 * Optional: Master endpoint host and port information for the `--endpoint` parameter.
* The client [ticket number](6-distributed-monitoring.md#distributed-monitoring-setup-csr-auto-signing)

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

In case the client should connect to the master node, you'll
need to modify the `--endpoint` parameter using the format `cn,host,port`.

    --endpoint icinga2-node1.localdomain,192.168.56.101,5665

Restart Icinga 2 once complete.

    # service icinga2 restart

