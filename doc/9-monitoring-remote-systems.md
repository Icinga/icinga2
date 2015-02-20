# <a id="monitoring-remote-systems"></a> Monitoring Remote Systems

There are multiple ways you can monitor remote clients. Be it using [agent-less](9-monitoring-remote-systems.md#agent-less-checks)
or [agent-based](agent-based-checks-addons) using additional addons & tools.

Icinga 2 uses its own unique and secure communitication protol amongst instances.
Be it an High-Availability cluster setup, distributed load-balanced setup or just a single
agent [monitoring a remote client](9-monitoring-remote-systems.md#icinga2-remote-client-monitoring).

All communication is secured by TLS with certificates, and fully supports IPv4 and IPv6.

If you are planning to use the native Icinga 2 cluster feature for distributed
monitoring and high-availability, please continue reading in
[this chapter](9-monitoring-remote-systems.md#distributed-monitoring-high-availability).

> **Tip**
>
> Don't panic - there are CLI commands available, including setup wizards for easy installation
> with SSL certificates.
> If you prefer to use your own CA (for example Puppet) you can do that as well.

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

## <a id="icinga2-remote-client-monitoring"></a> Monitoring Icinga 2 Remote Clients

First, you should decide which role the remote client has:

* a single host with local checks and configuration
* a remote satellite checking other hosts (for example in your DMZ)
* a remote command execution client (similar to NRPE, NSClient++, etc)

Later on, you will be asked again and told how to proceed with these
different [roles](9-monitoring-remote-systems.md#icinga2-remote-monitoring-client-roles).

> **Note**
>
> If you are planning to build an Icinga 2 distributed setup using the cluster feature, please skip
> the following instructions and jump directly to the
> [cluster setup instructions](9-monitoring-remote-systems.md#distributed-monitoring-high-availability).

> **Note**
>
> Remote instances are independent Icinga 2 instances which schedule
> their checks and just synchronize them back to the defined master zone.

## <a id="icinga2-remote-monitoring-master"></a> Master Setup for Remote Monitoring

If you are planning to use the [remote Icinga 2 clients](9-monitoring-remote-systems.md#icinga2-remote-monitoring-client)
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

    # icinga2 node wizard

    Welcome to the Icinga 2 Setup Wizard!

    We'll guide you through all required configuration details.

    If you have questions, please consult the documentation at http://docs.icinga.org
    or join the community support channels at https://support.icinga.org


    Please specify if this is a satellite setup ('n' installs a master setup) [Y/n]: n
    Starting the Master setup routine...
    Please specifiy the common name (CN) [icinga2m]:
    information/base: Writing private key to '/var/lib/icinga2/ca/ca.key'.
    information/base: Writing X509 certificate to '/var/lib/icinga2/ca/ca.crt'.
    information/cli: Initializing serial file in '/var/lib/icinga2/ca/serial.txt'.
    information/cli: Generating new CSR in '/etc/icinga2/pki/icinga2m.csr'.
    information/base: Writing private key to '/etc/icinga2/pki/icinga2m.key'.
    information/base: Writing certificate signing request to '/etc/icinga2/pki/icinga2m.csr'.
    information/cli: Signing CSR with CA and writing certificate to '/etc/icinga2/pki/icinga2m.crt'.
    information/cli: Copying CA certificate to '/etc/icinga2/pki/ca.crt'.
    information/cli: Dumping config items to file '/etc/icinga2/zones.conf'.
    Please specify the API bind host/port (optional):
    Bind Host []:
    Bind Port []:
    information/cli: Enabling the APIlistener feature.
    information/cli: Updating constants.conf.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    information/cli: Edit the constants.conf file '/etc/icinga2/constants.conf' and set a secure 'TicketSalt' constant.
    Done.

    Now restart your Icinga 2 daemon to finish the installation!

    If you encounter problems or bugs, please do not hesitate to
    get in touch with the community at https://support.icinga.org


The setup wizard will do the following:

* Generate a local CA in `/var/lib/icinga2/ca` or use the existing one
* Generate a new CSR, sign it with the local CA and copying it into `/etc/icinga2/pki`
* Generate a local zone and endpoint configuration for this master based on FQDN
* Enabling the API feature, and setting optional `bind_host` and `bind_port`
* Setting the `NodeName` and `TicketSalt` constants in [constants.conf](5-configuring-icinga-2.md#constants-conf)

The setup wizard does not automatically restart Icinga 2.


> **Note**
>
> This setup wizard will install a standalone master, HA cluster scenarios are currently
> not supported.



## <a id="icinga2-remote-monitoring-client"></a> Client Setup for Remote Monitoring

Icinga 2 can be installed on Linux/Unix and Windows. While
[Linux/Unix](9-monitoring-remote-systems.md#icinga2-remote-monitoring-client-linux) will be using the [CLI command](8-cli-commands.md#cli-command-node)
`node wizard` for a guided setup, you will need to use the
graphical installer for Windows based client setup.

Your client setup requires the following

* A ready configured and installed [master node](9-monitoring-remote-systems.md#icinga2-remote-monitoring-master)
* SSL signed certificate for communication with the master (Use [CSR auto-signing](certifiates-csr-autosigning)).
* Enabled API feature, and a local Endpoint and Zone object configuration
* Firewall ACLs for the communication port (default 5665)



### <a id="icinga2-remote-monitoring-client-linux"></a> Linux Client Setup for Remote Monitoring

#### <a id="csr-autosigning-requirements"></a> Requirements for CSR Auto-Signing

If your remote clients are capable of connecting to the central master, Icinga 2
supports CSR auto-signing.

First you'll need to define a secure ticket salt in the [constants.conf](5-configuring-icinga-2.md#constants-conf).
The [setup wizard for the master setup](9-monitoring-remote-systems.md#icinga2-remote-monitoring-master) will create
one for you already.

    # grep TicketSalt /etc/icinga2/constants.conf

The client setup wizard will ask you to generate a valid ticket number using its CN.
If you already know your remote client's Common Names (CNs) - usually the FQDN - you
can generate all ticket numbers on-demand.

This is also reasonable if you are not capable of installing the remote client, but
a colleague of yours, or a customer.

Example for a client notebook:

    # icinga2 pki ticket --cn nbmif.int.netways.de

> **Note**
>
> You can omit the `--salt` parameter using the `TicketSalt` constant from
> [constants.conf](5-configuring-icinga-2.md#constants-conf) if already defined and Icinga 2 was
> reloaded after the master setup.

#### <a id="certificates-manual-creation"></a> Manual SSL Certificate Generation

This is described separately in the [cluster setup chapter](9-monitoring-remote-systems.md#manual-certificate-generation).

> **Note**
>
> If you're using [CSR Auto-Signing](9-monitoring-remote-systems.md#csr-autosigning-requirements), skip this step.


#### <a id="icinga2-remote-monitoring-client-linux-setup"></a> Linux Client Setup Wizard for Remote Monitoring

Install Icinga 2 from your distribution's package repository as described in the
general [installation instructions](2-getting-started.md#setting-up-icinga2).

Please make sure that either [CSR Auto-Signing](9-monitoring-remote-systems.md#csr-autosigning-requirements) requirements
are fulfilled, or that you're using [manual SSL certificate generation](9-monitoring-remote-systems.md#manual-certificate-generation).

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
* The master endpoint connection information. Your master's IP address and port (defaults to 5665)
* The [request ticket number](9-monitoring-remote-systems.md#csr-autosigning-requirements) generated on your master
for CSR Auto-Signing
* Bind host/port for the Api feature (optional)

The command must be run as root, all Icinga 2 specific files will be updated to the icinga
user the daemon is running as (certificate files for example).


    # icinga2 node wizard

    Welcome to the Icinga 2 Setup Wizard!

    We'll guide you through all required configuration details.

    If you have questions, please consult the documentation at http://docs.icinga.org
    or join the community support channels at https://support.icinga.org


    Please specify if this is a satellite setup ('n' installs a master setup) [Y/n]:
    Starting the Node setup routine...
    Please specifiy the common name (CN) [nbmif.int.netways.de]:
    Please specifiy the local zone name [nbmif.int.netways.de]:
    Please specify the master endpoint(s) this node should connect to:
    Master Common Name (CN from your master setup, defaults to FQDN): icinga2m
    Please fill out the master connection information:
    Master endpoint host (required, your master's IP address or FQDN): 192.168.33.100
    Master endpoint port (optional) []:
    Add more master endpoints? [y/N]
    Please specify the master connection for CSR auto-signing (defaults to master endpoint host):
    Host [192.168.33.100]:
    Port [5665]:
    information/base: Writing private key to '/var/lib/icinga2/ca/ca.key'.
    information/base: Writing X509 certificate to '/var/lib/icinga2/ca/ca.crt'.
    information/cli: Initializing serial file in '/var/lib/icinga2/ca/serial.txt'.
    information/base: Writing private key to '/etc/icinga2/pki/nbmif.int.netways.de.key'.
    information/base: Writing X509 certificate to '/etc/icinga2/pki/nbmif.int.netways.de.crt'.
    information/cli: Generating self-signed certifiate:
    information/cli: Fetching public certificate from master (192.168.33.100, 5665):

    information/cli: Writing trusted certificate to file '/etc/icinga2/pki/trusted-master.crt'.
    information/cli: Stored trusted master certificate in '/etc/icinga2/pki/trusted-master.crt'.

    Please specify the request ticket generated on your Icinga 2 master.
     (Hint: '# icinga2 pki ticket --cn nbmif.int.netways.de'):
    2e070405fe28f311a455b53a61614afd718596a1
    information/cli: Processing self-signed certificate request. Ticket '2e070405fe28f311a455b53a61614afd718596a1'.

    information/cli: Writing signed certificate to file '/etc/icinga2/pki/nbmif.int.netways.de.crt'.
    information/cli: Writing CA certificate to file '/var/lib/icinga2/ca/ca.crt'.
    Please specify the API bind host/port (optional):
    Bind Host []:
    Bind Port []:
    information/cli: Disabling the Notification feature.
    Disabling feature notification. Make sure to restart Icinga 2 for these changes to take effect.
    information/cli: Enabling the Apilistener feature.
    information/cli: Generating local zones.conf.
    information/cli: Dumping config items to file '/etc/icinga2/zones.conf'.
    information/cli: Updating constants.conf.
    information/cli: Updating constants file '/etc/icinga2/constants.conf'.
    Done.

    Now restart your Icinga 2 daemon to finish the installation!

    If you encounter problems or bugs, please do not hesitate to
    get in touch with the community at https://support.icinga.org


The setup wizard will do the following:

* Generate a new self-signed certificate and copy it into `/etc/icinga2/pki`
* Store the master's certificate as trusted certificate for requesting a new signed certificate
(manual step when using `node setup`).
* Request a new signed certificate from the master and store updated certificate and master CA in `/etc/icinga2/pki`
* Generate a local zone and endpoint configuration for this client and the provided master information
(based on FQDN)
* Disabling the notification feature for this client
* Enabling the API feature, and setting optional `bind_host` and `bind_port`
* Setting the `NodeName` constant in [constants.conf](5-configuring-icinga-2.md#constants-conf)

The setup wizard does not automatically restart Icinga 2.

If you are getting an error when requesting the ticket number, please check the following:

* Is the CN the same (from pki ticket on the master and setup node on the client)
* Is the ticket expired


### <a id="icinga2-remote-monitoring-client-windows"></a> Windows Client Setup for Remote Monitoring

Download the MSI-Installer package from [http://packages.icinga.org/windows/](http://packages.icinga.org/windows/).

Requirements:
* [Microsoft .NET Framework 2.0](http://www.microsoft.com/de-de/download/details.aspx?id=1639) if not already installed.

The setup wizard will install Icinga 2 and then continue with SSL certificate generation,
CSR-Autosigning and configuration setup.

You'll need the following configuration details:

* The client common name (CN). Defaults to FQDN.
* The client's local zone name. Defaults to FQDN.
* The master endpoint name. Look into your master setup `zones.conf` file for the proper name.
* The master endpoint connection information. Your master's IP address and port (defaults to 5665)
* The [request ticket number](9-monitoring-remote-systems.md#csr-autosigning-requirements) generated on your master
for CSR Auto-Signing
* Bind host/port for the Api feature (optional)

Once install is done, Icinga 2 is automatically started as a Windows service.

### <a id="icinga2-remote-monitoring-client-roles"></a> Remote Monitoring Client Roles

Icinga 2 allows you to use two separate ways of defining a client (or: `agent`) role:

* execute commands remotely, but host/service configuration happens on the master.
* schedule remote checks on remote satellites with their local configuration.

Depending on your scenario, either one or both combined with a cluster setup
could be build and put together.


### <a id="icinga2-remote-monitoring-client-command-execution"></a> Remote Client for Command Execution

This scenario allows you to configure the checkable objects (hosts, services) on
your Icinga 2 master or satellite, and only send commands remotely.

Requirements:
* Exact same [CheckCommand](6-object-types.md#objecttype-checkcommand) (and
[EventCommand](6-object-types.md#objecttype-eventcommand)) configuration objects
on the master and the remote client(s).
* Installed plugin scripts on the remote client (`PluginDir` constant can be locally modified)
* `Zone` and `Endpoint` configuration for the client on the master
* `command_endpoint` attribute configured for host/service objects pointing to the configured
endpoint

`CheckCommand` objects are already shipped with the Icinga 2 ITL
as [plugin check commands](7-icinga-template-library.md#plugin-check-commands). If you are
using your own configuration definitions for example in
[commands.conf](5-configuring-icinga-2.md#commands-conf) make sure to copy/sync it
on your remote client.

#### <a id="icinga2-remote-monitoring-client-command-execution-client"></a> Client Configuration Remote Client for Command Execution

> **Note**
>
> Remote clients must explicitely accept commands in a similar
> fashion as cluster nodes [accept configuration]#i(cluster-zone-config-sync).
> This is due to security reasons.

Edit the `api` feature configuration in `/etc/icinga2/features-enabled/api.conf`
and set `accept_commands` to `true`.

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_commands = true
    }

#### <a id="icinga2-remote-monitoring-client-command-execution-master"></a> Master Configuration Remote Client for Command Execution

Add an `Endpoint` and `Zone` configuration object for the remote client
in [zones.conf](#zones-conf) and define a trusted master zone as `parent`.

    object Endpoint "remote-client1" {
      host = "192.168.33.20"
    }

    object Zone "remote-client1" {
      endpoints = [ "remote-client1" ]
      parent = "master"
    }

More details here:
* [configure endpoints](9-monitoring-remote-systems.md#configure-cluster-endpoints)
* [configure zones](9-monitoring-remote-systems.md#configure-cluster-zones)


Configuration example for host and service objects running commands on the remote endpoint `remote-client1`:

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


That way you can also execute the `icinga` check remotely
thus verifying the health of your remote client(s). As a bonus
you'll also get the running Icinga 2 version and may
schedule client updates in your management tool (e.g. Puppet).

> **Tip**
>
> [Event commands](3-monitoring-basics.md#event-commands) are executed on the
> remote command endpoint as well. You do not need
> an additional transport layer such as SSH or similar.

> **Note**
> You cannot add any Icinga 2 features like DB IDO on the remote
> clients. There are no local configured objects available.
>
> If you require this, please install a full-featured
> [local client](9-monitoring-remote-systems.md#icinga2-remote-monitoring-client-local-config).

### <a id="icinga2-remote-monitoring-client-local-config"></a> Remote Client with Local Configuration

This is considered as independant satellite using a local scheduler, configuration
and the possibility to add Icinga 2 features on demand.

Local configured checks are transferred to the central master and helped
with discovery CLI commands.

Please follow the instructions closely in order to deploy your fully featured
client, or `agent` as others might call it.

#### <a id="icinga2-remote-monitoring-client-configuration"></a> Client Configuration for Remote Monitoring

There is no difference in the configuration syntax on clients to any other Icinga 2 installation.

The following convention applies to remote clients:

* The hostname in the default host object should be the same as the Common Name (CN) used for SSL setup
* Add new services and check commands locally

The default setup routine will install a new host based on your FQDN in `repository.d/hosts` with all
services in separate configuration files a directory underneath.

The repository can be managed using the CLI command `repository`.

> **Note**
>
> The CLI command `repository` only supports basic configuration manipulation (add, remove). Future
> versions will support more options (set, etc.). Please check the Icinga 2 development roadmap
> for that.

You can also use additional features like notifications directly on the remote client, if you are
required to. Basically everything a single Icinga 2 instance provides by default.


#### <a id="icinga2-remote-monitoring-master-discovery"></a> Discover Client Services on the Master

Icinga 2 clients will sync their locally defined objects to the defined master node. That way you can
list, add, filter and remove nodes based on their `node`, `zone`, `host` or `service` name.

List all discovered nodes (satellites, agents) and their hosts/services:

    # icinga2 node list


#### <a id="icinga2-remote-monitoring-master-discovery-manual"></a> Manually Discover Clients on the Master

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
> Better use [blacklists and/or whitelists](9-monitoring-remote-systems.md#icinga2-remote-monitoring-master-discovery-blacklist-whitelist)
> to control which clients and hosts/services are integrated into your master configuration repository.

### <a id="icinga2-remote-monitoring-master-discovery-generate-config"></a> Generate Icinga 2 Configuration for Client Services on the Master

There is a dedicated Icinga 2 CLI command for updating the client services on the master,
generating all required configuration.

    # icinga2 node update-config

The generated configuration of all nodes is stored in the `repository.d/` directory.

By default, the following additional configuration is generated:
* add `Endpoint` and `Zone` objects for the newly added node
* add `cluster-zone` health check for the master host detecting if the remote node died
* use the default templates `satellite-host` and `satellite-service` defined in `/etc/icinga2/conf.d/satellite.conf`
* apply a dependency for all other hosts on the remote satellite prevening failure checks/notifications

> **Note**
>
> If there are existing hosts/services defined or modified, the CLI command will not overwrite these (modified)
> configuration files.
>
> If hosts or services disappeared from the client discovery, it will remove the existing configuration objects
> from the config repository.

The `update-config` CLI command will fail, if there are uncommitted changes for the
configuration repository.
Please review these changes manually, or clear the commit and try again. This is a
safety hook to prevent unwanted manual changes to be committed by a updating the
client discovered objects only.

    # icinga2 repository commit --simulate

    # icinga2 repository clear-changes

    # icinga2 repository commit

After updating the configuration repository, make sure to reload Icinga 2.

    # service icinga2 reload

Using systemd:
    # systemctl reload icinga2


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
> The `--zone` and `--host` arguments are required. A zone is always where the remote client is in.
> If you are unsure about it, set a wildcard (`*`) for them and filter only by host/services.


### <a id="icinga2-remote-monitoring-master-manual-add-endpoint-zone"></a> Manually add Client Endpoint and Zone Objects on the Master

Define a [Zone](6-object-types.md#objecttype-zone) with a new [Endpoint](6-object-types.md#objecttype-endpoint) similar to the cluster setup.

* [configure the node name](9-monitoring-remote-systems.md#configure-nodename)
* [configure the ApiListener object](9-monitoring-remote-systems.md#configure-apilistener-object)
* [configure cluster endpoints](9-monitoring-remote-systems.md#configure-cluster-endpoints)
* [configure cluster zones](9-monitoring-remote-systems.md#configure-cluster-zones)

on a per remote client basis. If you prefer to synchronize the configuration to remote
clients, you can also use the cluster provided [configuration sync](9-monitoring-remote-systems.md#cluster-zone-config-sync)
in `zones.d`.


### <a id="agent-based-checks-addon"></a> Agent-based Checks using additional Software

If the remote services are not directly accessible through the network, a
local agent installation exposing the results to check queries can
become handy.

### <a id="agent-based-checks-snmp"></a> SNMP

The SNMP daemon runs on the remote system and answers SNMP queries by plugin
binaries. The [Monitoring Plugins package](2-getting-started.md#setting-up-check-plugins) ships
the `check_snmp` plugin binary, but there are plenty of [existing plugins](10-addons-plugins.md#plugins)
for specific use cases already around, for example monitoring Cisco routers.

The following example uses the [SNMP ITL](7-icinga-template-library.md#plugin-check-command-snmp) `CheckCommand` and just
overrides the `snmp_oid` custom attribute. A service is created for all hosts which
have the `snmp-community` custom attribute.

    apply Service "uptime" {
      import "generic-service"

      check_command = "snmp"
      vars.snmp_oid = "1.3.6.1.2.1.1.3.0"
      vars.snmp_miblist = "DISMAN-EVENT-MIB"

      assign where host.vars.snmp_community != ""
    }

Additional SNMP plugins are available using the [Manubulon SNMP Plugins](7-icinga-template-library.md#snmp-manubulon-plugin-check-commands).

If no `snmp_miblist` is specified the plugin will default to `ALL`. As the number of available MIB files 
on the system increases so will the load generated by this plugin if no `MIB` is specified.
As such, it is recommended to always specify at least one `MIB`.

### <a id="agent-based-checks-ssh"></a> SSH

Calling a plugin using the SSH protocol to execute a plugin on the remote server fetching
its return code and output. The `by_ssh` command object is part of the built-in templates and
requires the `check_by_ssh` check plugin which is available in the [Monitoring Plugins package](2-getting-started.md#setting-up-check-plugins).

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

> **Note**
>
> The NRPE protocol is considered insecure and has multiple flaws in its
> design. Upstream is not willing to fix these issues.
>
> In order to stay safe, please use the native [Icinga 2 client](9-monitoring-remote-systems.md#icinga2-remote-monitoring-master)
> instead.

The NRPE daemon uses its own configuration format in nrpe.cfg while `check_nrpe`
can be embedded into the Icinga 2 `CheckCommand` configuration syntax.

You can use the `check_nrpe` plugin from the NRPE project to query the NRPE daemon.
Icinga 2 provides the [nrpe check command](7-icinga-template-library.md#plugin-check-command-nrpe) for this:

Example:

    object Service "users" {
      import "generic-service"

      host_name = "remote-nrpe-host"

      check_command = "nrpe"
      vars.nrpe_command = "check_users"
    }

nrpe.cfg:

    command[check_users]=/usr/local/icinga/libexec/check_users -w 5 -c 10

If you are planning to pass arguments to NRPE using the `-a`
command line parameter, make sure that your NRPE daemon has them
supported and enabled.

> **Note**
>
> Enabling command arguments in NRPE is considered harmful
> and exposes a security risk allowing attackers to execute
> commands remotely. Details at [seclists.org](http://seclists.org/fulldisclosure/2014/Apr/240).

The plugin check command `nrpe` provides the `nrpe_arguments` custom
attribute which expects either a single value or an array of values.

Example:

    object Service "nrpe-disk-/" {
      import "generic-service"

      host_name = "remote-nrpe-host"

      check_command = "nrpe"
      vars.nrpe_command = "check_disk"
      vars.nrpe_arguments = [ "20%", "10%", "/" ]
    }

Icinga 2 will execute the nrpe plugin like this:

    /usr/lib/nagios/plugins/check_nrpe -H <remote-nrpe-host> -c 'check_disk' -a '20%' '10%' '/'

NRPE expects all additional arguments in an ordered fashion
and interprets the first value as `$ARG1$` macro, the second
value as `$ARG2$`, and so on.

nrpe.cfg:

    command[check_disk]=/usr/local/icinga/libexec/check_disk -w $ARG1$ -c $ARG2$ -p $ARG3$

Using the above example with `nrpe_arguments` the command
executed by the NRPE daemon looks similar to that:

    /usr/local/icinga/libexec/check_disk -w 20% -c 10% -p /

You can pass arguments in a similar manner to [NSClient++](9-monitoring-remote-systems.md#agent-based-checks-nsclient)
when using its NRPE supported check method.

### <a id="agent-based-checks-nsclient"></a> NSClient++

[NSClient++](http://nsclient.org) works on both Windows and Linux platforms and is well
known for its magnificent Windows support. There are alternatives like the WMI interface,
but using `NSClient++` will allow you to run local scripts similar to check plugins fetching
the required output and performance counters.

You can use the `check_nt` plugin from the Monitoring Plugins project to query NSClient++.
Icinga 2 provides the [nscp check command](7-icinga-template-library.md#plugin-check-command-nscp) for this:

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

### <a id="agent-based-checks-nsca-ng"></a> NSCA-NG

[NSCA-ng](http://www.nsca-ng.org) provides a client-server pair that allows the
remote sender to push check results into the Icinga 2 `ExternalCommandListener`
feature.

> **Note**
>
> This addon works in a similar fashion like the Icinga 1.x distributed model. If you
> are looking for a real distributed architecture with Icinga 2, scroll down.

### <a id="agent-based-checks-snmp-traps"></a> Passive Check Results and SNMP Traps

SNMP Traps can be received and filtered by using [SNMPTT](http://snmptt.sourceforge.net/)
and specific trap handlers passing the check results to Icinga 2.

Following the SNMPTT [Format](http://snmptt.sourceforge.net/docs/snmptt.shtml#SNMPTT.CONF-FORMAT)
documentation and the Icinga external command syntax found [here](19-appendix.md#external-commands-list-detail)
we can create generic services that can accommodate any number of hosts for a given scenario.

#### <a id="simple-traps"></a> Simple SNMP Traps

A simple example might be monitoring host reboots indicated by an SNMP agent reset.
Building the event to auto reset after dispatching a notification is important.
Setup the manual check parameters to reset the event from an initial unhandled
state or from a missed reset event.

Add a directive in `snmptt.conf`

    EVENT coldStart .1.3.6.1.6.3.1.1.5.1 "Status Events" Normal
    FORMAT Device reinitialized (coldStart)
    EXEC echo "[$@] PROCESS_SERVICE_CHECK_RESULT;$A;Coldstart;2;The snmp agent has reinitialized." >> /var/run/icinga2/cmd/icinga2.cmd
    SDESC
    A coldStart trap signifies that the SNMPv2 entity, acting
    in an agent role, is reinitializing itself and that its
    configuration may have been altered.
    EDESC

1. Define the `EVENT` as per your need.
2. Construct the `EXEC` statement with the service name matching your template
applied to your _n_ hosts. The host address inferred by SNMPTT will be the
correlating factor. You can have snmptt provide host names or ip addresses to
match your Icinga convention.

Add an `EventCommand` configuration object for the passive service auto reset event.

    object EventCommand "coldstart-reset-event" {
      import "plugin-event-command"

      command = [ SysconfDir + "/icinga2/conf.d/custom/scripts/coldstart_reset_event.sh" ]

      arguments = {
        "-i" = "$service.state_id$"
        "-n" = "$host.name$"
        "-s" = "$service.name$"
      }
    }

Create the `coldstart_reset_event.sh` shell script to pass the expanded variable
data in. The `$service.state_id$` is important in order to prevent an endless loop
of event firing after the service has been reset.

    #!/bin/bash

    SERVICE_STATE_ID=""
    HOST_NAME=""
    SERVICE_NAME=""

    show_help()
    {
    cat <<-EOF
    	Usage: ${0##*/} [-h] -n HOST_NAME -s SERVICE_NAME
    	Writes a coldstart reset event to the Icinga command pipe.

    	  -h                  Display this help and exit.
    	  -i SERVICE_STATE_ID The associated service state id.
    	  -n HOST_NAME        The associated host name.
    	  -s SERVICE_NAME     The associated service name.
    EOF
    }

    while getopts "hi:n:s:" opt; do
        case "$opt" in
          h)
              show_help
              exit 0
              ;;
          i)
              SERVICE_STATE_ID=$OPTARG
              ;;
          n)
              HOST_NAME=$OPTARG
              ;;
          s)
              SERVICE_NAME=$OPTARG
              ;;
          '?')
              show_help
              exit 0
              ;;
          esac
    done

    if [ -z "$SERVICE_STATE_ID" ]; then
        show_help
        printf "\n  Error: -i required.\n"
        exit 1
    fi

    if [ -z "$HOST_NAME" ]; then
        show_help
        printf "\n  Error: -n required.\n"
        exit 1
    fi

    if [ -z "$SERVICE_NAME" ]; then
        show_help
        printf "\n  Error: -s required.\n"
        exit 1
    fi

    if [ "$SERVICE_STATE_ID" -gt 0 ]; then
        echo "[`date +%s`] PROCESS_SERVICE_CHECK_RESULT;$HOST_NAME;$SERVICE_NAME;0;Auto-reset (`date +"%m-%d-%Y %T"`)." >> /var/run/icinga2/cmd/icinga2.cmd
    fi

Finally create the `Service` and assign it:

    apply Service "Coldstart" {
      import "generic-service-custom"

      check_command         = "dummy"
      event_command         = "coldstart-reset-event"

      enable_notifications  = 1
      enable_active_checks  = 0
      enable_passive_checks = 1
      enable_flapping       = 0
      volatile              = 1
      enable_perfdata       = 0

      vars.dummy_state      = 0
      vars.dummy_text       = "Manual reset."

      vars.sla              = "24x7"

      assign where (host.vars.os == "Linux" || host.vars.os == "Windows")
    }

#### <a id="complex-traps"></a> Complex SNMP Traps

A more complex example might be passing dynamic data from a traps varbind list
for a backup scenario where the backup software dispatches status updates. By
utilizing active and passive checks, the older freshness concept can be leveraged.

By defining the active check as a hard failed state, a missed backup can be reported.
As long as the most recent passive update has occurred, the active check is bypassed.

Add a directive in `snmptt.conf`

    EVENT enterpriseSpecific <YOUR OID> "Status Events" Normal
    FORMAT Enterprise specific trap
    EXEC echo "[$@] PROCESS_SERVICE_CHECK_RESULT;$A;$1;$2;$3" >> /var/run/icinga2/cmd/icinga2.cmd
    SDESC
    An enterprise specific trap.
    The varbinds in order denote the Icinga service name, state and text.
    EDESC

1. Define the `EVENT` as per your need using your actual oid.
2. The service name, state and text are extracted from the first three varbinds.
This has the advantage of accommodating an unlimited set of use cases.

Create a `Service` for the specific use case associated to the host. If the host
matches and the first varbind value is `Backup`, SNMPTT will submit the corresponding
passive update with the state and text from the second and third varbind:

    object Service "Backup" {
      import "generic-service-custom"

      host_name             = "host.domain.com"
      check_command         = "dummy"

      enable_notifications  = 1
      enable_active_checks  = 1
      enable_passive_checks = 1
      enable_flapping       = 0
      volatile              = 1
      max_check_attempts    = 1
      check_interval        = 87000
      enable_perfdata       = 0

      vars.sla              = "24x7"
      vars.dummy_state      = 2
      vars.dummy_text       = "No passive check result received."
    }


## <a id="distributed-monitoring-high-availability"></a> Distributed Monitoring and High Availability

Building distributed environments with high availability included is fairly easy with Icinga 2.
The cluster feature is built-in and allows you to build many scenarios based on your requirements:

* [High Availability](9-monitoring-remote-systems.md#cluster-scenarios-high-availability). All instances in the `Zone` elect one active master and run as Active/Active cluster.
* [Distributed Zones](9-monitoring-remote-systems.md#cluster-scenarios-distributed-zones). A master zone and one or more satellites in their zones.
* [Load Distribution](9-monitoring-remote-systems.md#cluster-scenarios-load-distribution). A configuration master and multiple checker satellites.

You can combine these scenarios into a global setup fitting your requirements.

Each instance got their own event scheduler, and does not depend on a centralized master
coordinating and distributing the events. In case of a cluster failure, all nodes
continue to run independently. Be alarmed when your cluster fails and a Split-Brain-scenario
is in effect - all alive instances continue to do their job, and history will begin to differ.

> ** Note **
>
> Before you start, make sure to read the [requirements](#distributed-monitoring-requirements).


### <a id="cluster-requirements"></a> Cluster Requirements

Before you start deploying, keep the following things in mind:

* Your [SSL CA and certificates](#certificate-authority-certificates) are mandatory for secure communication
* Get pen and paper or a drawing board and design your nodes and zones!
    * all nodes in a cluster zone are providing high availability functionality and trust each other
    * cluster zones can be built in a Top-Down-design where the child trusts the parent
    * communication between zones happens bi-directional which means that a DMZ-located node can still reach the master node, or vice versa
* Update firewall rules and ACLs
* Decide whether to use the built-in [configuration syncronization](9-monitoring-remote-systems.md#cluster-zone-config-sync) or use an external tool (Puppet, Ansible, Chef, Salt, etc) to manage the configuration deployment


> **Tip**
>
> If you're looking for troubleshooting cluster problems, check the general
> [troubleshooting](13-troubleshooting.md#troubleshooting-cluster) section.


### <a id="manual-certificate-generation"></a> Manual SSL Certificate Generation

Icinga 2 ships [CLI commands](8-cli-commands.md#cli-command-pki) assisting with CA and node certificate creation
for your Icinga 2 distributed setup.

> **Note**
>
> You're free to use your own method to generated a valid ca and signed client
> certificates.

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



#### <a id="cluster-naming-convention"></a> Cluster Naming Convention

The SSL certificate common name (CN) will be used by the [ApiListener](6-object-types.md#objecttype-apilistener)
object to determine the local authority. This name must match the local [Endpoint](6-object-types.md#objecttype-endpoint)
object name.

Example:

    # icinga2 pki new-cert --cn icinga2a --key icinga2a.key --csr icinga2a.csr
    # icinga2 pki sign-csr --csr icinga2a.csr --cert icinga2a.crt

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

Specifying the local node name using the [NodeName](9-monitoring-remote-systems.md#configure-nodename) variable requires
the same name as used for the endpoint name and common name above. If not set, the FQDN is used.

    const NodeName = "icinga2a"


### <a id="cluster-configuration"></a> Cluster Configuration

The following section describe which configuration must be updated/created
in order to get your cluster running with basic functionality.

* [configure the node name](9-monitoring-remote-systems.md#configure-nodename)
* [configure the ApiListener object](9-monitoring-remote-systems.md#configure-apilistener-object)
* [configure cluster endpoints](9-monitoring-remote-systems.md#configure-cluster-endpoints)
* [configure cluster zones](9-monitoring-remote-systems.md#configure-cluster-zones)

Once you're finished with the basic setup the following section will
describe how to use [zone configuration synchronisation](9-monitoring-remote-systems.md#cluster-zone-config-sync)
and configure [cluster scenarios](9-monitoring-remote-systems.md#cluster-scenarios).

#### <a id="configure-nodename"></a> Configure the Icinga Node Name

Instead of using the default FQDN as node name you can optionally set
that value using the [NodeName](16-language-reference.md#constants) constant.

> ** Note **
>
> Skip this step if your FQDN already matches the default `NodeName` set
> in `/etc/icinga2/constants.conf`.

This setting must be unique for each node, and must also match
the name of the local [Endpoint](6-object-types.md#objecttype-endpoint) object and the
SSL certificate common name as described in the
[cluster naming convention](9-monitoring-remote-systems.md#cluster-naming-convention).

    vim /etc/icinga2/constants.conf

    /* Our local instance name. By default this is the server's hostname as returned by `hostname --fqdn`.
     * This should be the common name from the API certificate.
     */
    const NodeName = "icinga2a"


Read further about additional [naming conventions](9-monitoring-remote-systems.md#cluster-naming-convention).

Not specifying the node name will make Icinga 2 using the FQDN. Make sure that all
configured endpoint names and common names are in sync.

#### <a id="configure-apilistener-object"></a> Configure the ApiListener Object

The [ApiListener](6-object-types.md#objecttype-apilistener) object needs to be configured on
every node in the cluster with the following settings:

A sample config looks like:

    object ApiListener "api" {
      cert_path = SysconfDir + "/icinga2/pki/" + NodeName + ".crt"
      key_path = SysconfDir + "/icinga2/pki/" + NodeName + ".key"
      ca_path = SysconfDir + "/icinga2/pki/ca.crt"
      accept_config = true
    }

You can simply enable the `api` feature using

    # icinga2 feature enable api

Edit `/etc/icinga2/features-enabled/api.conf` if you require the configuration
synchronisation enabled for this node. Set the `accept_config` attribute to `true`.

> **Note**
>
> The certificate files must be readable by the user Icinga 2 is running as. Also,
> the private key file must not be world-readable.

#### <a id="configure-cluster-endpoints"></a> Configure Cluster Endpoints

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

#### <a id="configure-cluster-zones"></a> Configure Cluster Zones

`Zone` objects specify the endpoints located in a zone. That way your distributed setup can be
seen as zones connected together instead of multiple instances in that specific zone.

Zones can be used for [high availability](9-monitoring-remote-systems.md#cluster-scenarios-high-availability),
[distributed setups](9-monitoring-remote-systems.md#cluster-scenarios-distributed-zones) and
[load distribution](9-monitoring-remote-systems.md#cluster-scenarios-load-distribution).

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


### <a id="cluster-zone-config-sync"></a> Zone Configuration Synchronisation

By default all objects for specific zones should be organized in

    /etc/icinga2/zones.d/<zonename>

on the configuration master.

Your child zones and endpoint members **must not** have their config copied to `zones.d`.
The built-in configuration synchronisation takes care of that if your nodes accept
configuration from the parent zone. You can define that in the
[ApiListener](9-monitoring-remote-systems.md#configure-apilistener-object) object by configuring the `accept_config`
attribute accordingly.

You should remove the sample config included in `conf.d` by commenting the `recursive_include`
statement in [icinga2.conf](5-configuring-icinga-2.md#icinga2-conf):

    //include_recursive "conf.d"

Better use a dedicated directory name like `cluster` or similar, and include that
one if your nodes require local configuration not being synced to other nodes. That's
useful for local [health checks](9-monitoring-remote-systems.md#cluster-health-check) for example.

> **Note**
>
> In a [high availability](9-monitoring-remote-systems.md#cluster-scenarios-high-availability)
> setup only one assigned node can act as configuration master. All other zone
> member nodes **must not** have the `/etc/icinga2/zones.d` directory populated.

These zone packages are then distributed to all nodes in the same zone, and
to their respective target zone instances.

Each configured zone must exist with the same directory name. The parent zone
syncs the configuration to the child zones, if allowed using the `accept_config`
attribute of the [ApiListener](9-monitoring-remote-systems.md#configure-apilistener-object) object.

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

If the local configuration is newer than the received update Icinga 2 will skip the synchronisation
process.

> **Note**
>
> `zones.d` must not be included in [icinga2.conf](5-configuring-icinga-2.md#icinga2-conf). Icinga 2 automatically
> determines the required include directory. This can be overridden using the
> [global constant](16-language-reference.md#constants) `ZonesDir`.

#### <a id="zone-global-config-templates"></a> Global Configuration Zone for Templates

If your zone configuration setup shares the same templates, groups, commands, timeperiods, etc.
you would have to duplicate quite a lot of configuration objects making the merged configuration
on your configuration master unique.

> ** Note **
>
> Only put templates, groups, etc into this zone. DO NOT add checkable objects such as
> hosts or services here. If they are checked by all instances globally, this will lead
> into duplicated check results and unclear state history. Not easy to troubleshoot too -
> you've been warned.

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

> **Note**
>
> If the remote node does not have this zone configured, it will ignore the configuration
> update, if it accepts synchronized configuration.

If you don't require any global configuration, skip this setting.

#### <a id="zone-config-sync-permissions"></a> Zone Configuration Synchronisation Permissions

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
> Look into the [troubleshooting guides](13-troubleshooting.md#troubleshooting-cluster-config-sync) for debugging
> problems with the configuration synchronisation.


### <a id="cluster-health-check"></a> Cluster Health Check

The Icinga 2 [ITL](7-icinga-template-library.md#icinga-template-library) ships an internal check command checking all configured
`EndPoints` in the cluster setup. The check result will become critical if
one or more configured nodes are not connected.

Example:

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


### <a id="cluster-scenarios"></a> Cluster Scenarios

All cluster nodes are full-featured Icinga 2 instances. You only need to enabled
the features for their role (for example, a `Checker` node only requires the `checker`
feature enabled, but not `notification` or `ido-mysql` features).

#### <a id="cluster-scenarios-security"></a> Security in Cluster Scenarios

While there are certain capabilities to ensure the safe communication between all
nodes (firewalls, policies, software hardening, etc) the Icinga 2 cluster also provides
additional security itself:

* [SSL certificates](#certificate-authority-certificates) are mandatory for cluster communication.
* Child zones only receive event updates (check results, commands, etc) for their configured updates.
* Zones cannot influence/interfere other zones. Each checked object is assigned to only one zone.
* All nodes in a zone trust each other.
* [Configuration sync](9-monitoring-remote-systems.md#zone-config-sync-permissions) is disabled by default.

#### <a id="cluster-scenarios-features"></a> Features in Cluster Zones

Each cluster zone may use all available features. If you have multiple locations
or departments, they may write to their local database, or populate graphite.
Even further all commands are distributed amongst connected nodes. For example, you could
re-schedule a check or acknowledge a problem on the master, and it gets replicated to the
actual slave checker node.

DB IDO on the left, graphite on the right side - works (if you disable
[DB IDO HA](9-monitoring-remote-systems.md#high-availability-db-ido)).
Icinga Web 2 on the left, checker and notifications on the right side - works too.
Everything on the left and on the right side - make sure to deal with
[load-balanced notifications and checks](9-monitoring-remote-systems.md#high-availability-features) in a
[HA zone](9-monitoring-remote-systems.md#cluster-scenarios-high-availability).
configure-cluster-zones
#### <a id="cluster-scenarios-distributed-zones"></a> Distributed Zones

That scenario fits if your instances are spread over the globe and they all report
to a master instance. Their network connection only works towards the master master
(or the master is able to connect, depending on firewall policies) which means
remote instances won't see each/connect to each other.

All events (check results, downtimes, comments, etc) are synced to the master node,
but the remote nodes can still run local features such as a web interface, reporting,
graphing, etc. in their own specified zone.

Imagine the following example with a master node in Nuremberg, and two remote DMZ
based instances in Berlin and Vienna. Additonally you'll specify
[global templates](9-monitoring-remote-systems.md#zone-global-config-templates) available in all zones.

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
> [accepts synced configuration](9-monitoring-remote-systems.md#zone-config-sync-permissions).

#### <a id="cluster-scenarios-load-distribution"></a> Load Distribution

If you are planning to off-load the checks to a defined set of remote workers
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

If you are planning to have some checks executed by a specific set of checker nodes
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
> [accepts synced configuration](9-monitoring-remote-systems.md#zone-config-sync-permissions).

#### <a id="cluster-scenarios-high-availability"></a> Cluster High Availability

High availability with Icinga 2 is possible by putting multiple nodes into
a dedicated [zone](9-monitoring-remote-systems.md#configure-cluster-zones). All nodes will elect one
active master, and retry an election once the current active master is down.

Selected features provide advanced [HA functionality](9-monitoring-remote-systems.md#high-availability-features).
Checks and notifications are load-balanced between nodes in the high availability
zone.

Connections from other zones will be accepted by all active and passive nodes
but all are forwarded to the current active master dealing with the check results,
commands, etc.

    object Zone "config-ha-master" {
      endpoints = [ "icinga2a", "icinga2b", "icinga2c" ]
    }

Two or more nodes in a high availability setup require an [initial cluster sync](9-monitoring-remote-systems.md#initial-cluster-sync).

> **Note**
>
> Keep in mind that **only one node acts as configuration master** having the
> configuration files in the `zones.d` directory. All other nodes **must not**
> have that directory populated. Instead they are required to
> [accept synced configuration](9-monitoring-remote-systems.md#zone-config-sync-permissions).
> Details in the [Configuration Sync Chapter](9-monitoring-remote-systems.md#cluster-zone-config-sync).

#### <a id="cluster-scenarios-multiple-hierarchies"></a> Multiple Hierarchies

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


### <a id="high-availability-features"></a> High Availability for Icinga 2 features

All nodes in the same zone require the same features enabled for High Availability (HA)
amongst them.

By default the following features provide advanced HA functionality:

* [Checks](9-monitoring-remote-systems.md#high-availability-checks) (load balanced, automated failover)
* [Notifications](9-monitoring-remote-systems.md#high-availability-notifications) (load balanced, automated failover)
* [DB IDO](9-monitoring-remote-systems.md#high-availability-db-ido) (Run-Once, automated failover)

#### <a id="high-availability-checks"></a> High Availability with Checks

All nodes in the same zone load-balance the check execution. When one instance
fails the other nodes will automatically take over the reamining checks.

> **Note**
>
> If a node should not check anything, disable the `checker` feature explicitely and
> reload Icinga 2.

    # icinga2 feature disable checker
    # service icinga2 reload

#### <a id="high-availability-notifications"></a> High Availability with Notifications

Notifications are load balanced amongst all nodes in a zone. By default this functionality
is enabled.
If your nodes should notify independent from any other nodes (this will cause
duplicated notifications if not properly handled!), you can set `enable_ha = false`
in the [NotificationComponent](6-object-types.md#objecttype-notificationcomponent) feature.

#### <a id="high-availability-db-ido"></a> High Availability with DB IDO

All instances within the same zone (e.g. the `master` zone as HA cluster) must
have the DB IDO feature enabled.

Example DB IDO MySQL:

    # icinga2 feature enable ido-mysql
    The feature 'ido-mysql' is already enabled.

By default the DB IDO feature only runs on the elected zone master. All other passive
nodes disable the active IDO database connection at runtime.

> **Note**
>
> The DB IDO HA feature can be disabled by setting the `enable_ha` attribute to `false`
> for the [IdoMysqlConnection](6-object-types.md#objecttype-idomysqlconnection) or
> [IdoPgsqlConnection](6-object-types.md#objecttype-idopgsqlconnection) object on all nodes in the
> same zone.
>
> All endpoints will enable the DB IDO feature then, connect to the configured
> database and dump configuration, status and historical data on their own.

If the instance with the active DB IDO connection dies, the HA functionality will
re-enable the DB IDO connection on the newly elected zone master.

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


### <a id="cluster-add-node"></a> Add a new cluster endpoint

These steps are required for integrating a new cluster endpoint:

* generate a new [SSL client certificate](#certificate-authority-certificates)
* identify its location in the zones
* update the `zones.conf` file on each involved node ([endpoint](9-monitoring-remote-systems.md#configure-cluster-endpoints), [zones](9-monitoring-remote-systems.md#configure-cluster-zones))
    * a new slave zone node requires updates for the master and slave zones
    * verify if this endpoints requires [configuration synchronisation](9-monitoring-remote-systems.md#cluster-zone-config-sync) enabled
* if the node requires the existing zone history: [initial cluster sync](9-monitoring-remote-systems.md#initial-cluster-sync)
* add a [cluster health check](9-monitoring-remote-systems.md#cluster-health-check)

#### <a id="initial-cluster-sync"></a> Initial Cluster Sync

In order to make sure that all of your cluster nodes have the same state you will
have to pick one of the nodes as your initial "master" and copy its state file
to all the other nodes.

You can find the state file in `/var/lib/icinga2/icinga2.state`. Before copying
the state file you should make sure that all your cluster nodes are properly shut
down.


### <a id="host-multiple-cluster-nodes"></a> Host With Multiple Cluster Nodes

Special scenarios might require multiple cluster nodes running on a single host.
By default Icinga 2 and its features will place their runtime data below the prefix
`LocalStateDir`. By default packages will set that path to `/var`.
You can either set that variable as constant configuration
definition in [icinga2.conf](5-configuring-icinga-2.md#icinga2-conf) or pass it as runtime variable to
the Icinga 2 daemon.

    # icinga2 -c /etc/icinga2/node1/icinga2.conf -DLocalStateDir=/opt/node1/var
