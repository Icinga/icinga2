# Advanced Topics

## Downtimes

TODO (move to basics?)

## Comments

TODO (move to basics?)

## Acknowledgements

TODO (move to basics?)

## Cluster

An Icinga 2 cluster consists of two or more nodes and can resist on multiple architectures. The base concept of Icinga 2 is the possibility to add additional features using components. In case of a cluster setup you have to add the cluster feature to all involved nodes. Before you start configuring the diffent nodes its necessary to setup the underlaying communication layer based on SSL.

### Certificate authority and Certificates

If you have no other way, we would suggest to use easy-rsa for certificate creation. You can get easy-rsa using your distribution package manager or the following git clone

	$ git clone https://github.com/OpenVPN/easy-rsa.git

Before you create your CA please add your minium local variables to /easy-rsa/vars

* KEY_COUNTRY
* KEY_PROVINCE
* KEY_CITY
* KEY_ORG
* KEY_EMAIL
* KEY_OU

After that you have to export the defined var and clean-up all previously created files

	source ./vars
	./clean-all

Then you can start CA creation using

	./build-ca

After that you can find your ca.crt and ca.key file in the keys directory and can create a server certificate for every node in the cluster using

	./build-key-server <node-name> 

Please don't use a passphrase during the certificate creation process.

Icinga 2 needs all certification information in one file which could be easily achieved using

	cat <node-name>.crt <node-name>.key > <node-name>.pem

Please create a key-file for every node in the Icinga 2 Cluster and save the CA-Key for additional nodes at a later date

### Enable the cluster configuration

Until the cluster-component is moved into an independent feature you have to enable the required libraries in the icinga2.conf

	library "cluster"

### Configure the ClusterListener Object

The ClusterListener needs to be configured on every node in the cluster with the following settings:

  Configuration Setting    |Value
  -------------------------|------------------------------------
  ca_path                  | path to ca.crt file
  cert_path                | path to server certificate
  bind_port                | port for incoming and outgoing conns
  peers                    | array of all reachable nodes
  ------------------------- ------------------------------------

A sample config part can look like this:

	/**
 	* Load cluster-library and configure Cluster-Listener using CA-files
 	*/
	library "cluster"
	object ClusterListener "cluster" {
    		ca_path = "/etc/icinga2/ca/ca.crt",
    		cert_path = "/etc/icinga2/ca/icinga-node-1.pem",
    		bind_port = 8888,
    		peers = [ "icinga-node-1", "icinga-node-2" ]
	}

Peers configures the direction used to connect multipe nodes together. If have a three node cluster consisting of 

* node-1
* node-2
* node-3

and node-3 is only reachable from node-2, you have to consider this in your peer configuration

### Configure Cluster Endpoints

In addition to the configured port and hostname every endpoint can have specific abilities to send configfiles to other nodes and limit the hosts allowed to send config-files.

  Configuration Setting    |Value
  -------------------------|------------------------------------
  host                     | hostname
  port                     | port
  accept_config            | defines all nodes allowed to send configs
  config_files             | defines all files to be send to other nodes
  ------------------------- ------------------------------------

A sample config part can look like this:

	/**
 	* Configure endpoints for cluster configuration
 	*/
	
	object Endpoint "icinga-node-1" {
    	host = "icinga-node-1.localdomain",
    	port = 8888
	}



## Dependencies

TODO

## Check Result Freshness

In Icinga 2 active check freshness is enabled by default. It is determined by the
`check_interval` attribute and no incoming check results in that period of time.

    threshold = last check execution time + check interval

Passive check freshness is calculated from the `check_interval` attribute if set.

    threshold = last check result time + check interval

If the freshness checks are invalid, a new check is executed defined by the
`check_command` attribute.

## Check Flapping

TODO

## Volatile Services

TODO

## Modified Attributes

TODO

## List of External Commands

TODO

## Plugin API

TODO

### Nagios Plugins

