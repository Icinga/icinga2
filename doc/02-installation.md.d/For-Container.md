# Installing Icinga 2 in Containers

To be able to run Icinga 2 in a containerized environment, you'll need to set up a few things.
This guide will help you get started with running Icinga 2 in a container using our official images.

## Prerequisites

- A container runtime such as [Docker](https://www.docker.com) or [Podman](https://podman.io) installed on your system.
- Basic knowledge of how to use Docker or Podman commands.
- A basic understanding of Icinga 2 and its configuration.

## Getting Started

First, create a dedicated docker network for Icinga 2 to ensure that all containers can communicate with each other:

```
docker network create icinga
```

Next, start an Icinga 2 master container using the official Icinga 2 image. By default, all Icinga 2 containers will
listen on port `5665` from within the docker network, but you can expose this port on a different port on your host
system if needed. The following command will start an Icinga 2 master container with the necessary configurations:

```
docker run --detach \
	--network icinga \
	--name icinga-master \
	--hostname icinga-master \
	--publish 5665:5665 \
	--volume icinga-master:/data \
	--env ICINGA_MASTER=1 \
	icinga/icinga2
```

This command will run the Icinga 2 master container in detached mode, exposing port 5665 for communication and mounting
the `/data` directory to a persistent volume named `icinga-master`. You can adjust the volume name and other parameters
as needed. You can also set additional environment variables to configure the Icinga 2 instance,
see [Environment Variables](#environment-variables) for a list of available options.

Alternatively, if you're used to using the `icinga2 node wizard` command to set up Icinga 2 nodes, you can still run
the `icinga2 node wizard` command to set up the containers interactively.

```
docker run --rm -it \
	--network icinga \
	--hostname icinga-master \
	--volume icinga-master:/data \
	icinga/icinga2 icinga2 node wizard
```

This command will run the Icinga 2 master container in interactive mode, allowing you to answer the prompts from the
`icinga2 node wizard` command.

Another option is to mount all the necessary configuration files from your host system into the container.
This way, you can use your existing Icinga 2 configuration files without needing any additional setup steps.
By default, the container will be populated with the default Icinga 2 configuration files, but you can override
them by creating bind mounts from your host system to the respective directories in the container. For example, to
replace the default `api-users.conf` file with your own one, you can start the Icinga 2 master container with the
following command:

```
docker run --detach \
	--network icinga \
	--name icinga-master \
	--hostname icinga-master \
	--publish 5665:5665 \
	--volume icinga-master:/data \
	--mount=type=bind,source=/absolute/path/to/your/api-users.conf,target=/data/etc/icinga2/conf.d/api-users.conf \
	--env ICINGA_MASTER=1 \
	icinga/icinga2
```

> **Note**
>
> If you [mount an empty](https://docs.docker.com/engine/storage/bind-mounts/#mount-into-a-non-empty-directory-on-the-container)
> directory from your host into the container's `/data` directory using `--volume /path/to/empty-directory:/data`,
> all files in `/data` inside the container will be obscured. The container may not start correctly because its
> important files are no longer visible. This happens because Docker replaces the container's `/data` directory with
> your empty host directory. To avoid this, either use `--mount` to bind only specific files or subdirectories, or
> use `--volume` with a named volume (like `icinga-master:/data`) so the container's default files are preserved and
> only your specified files are replaced.

## Adding Icinga 2 Agents

To add Icinga 2 agents to your setup, you can run additional containers for each agent. In order your agents be able
to successfully connect to the master, they need to have a copy of the master's `ca.crt` file created during the master
setup. You can first copy this file from the master container to your host system using the following command:

```
docker cp icinga-master:/var/lib/icinga2/certs/ca.crt icinga-ca.crt
```

If you didn't use `icinga-master` as the name of your master container, replace it with the actual name you used.
For easier setup, you may want to also obtain a `ticket` from the master container, which will allow the agent to
authenticate itself without needing you to manually sign a certificate signing request (CSR).

You can create a ticket for the agent by running the following command on your host system:

```
docker exec icinga-master icinga2 pki ticket --cn icinga-agent > icinga-agent.ticket
```

Again, replace `icinga-master` with the actual name of your master container if necessary. Additionally, you may want
to adjust the `--cn` parameter to match the hostname of your agent containers. For non-ticket based setups, the required
steps are described in the [On-Demand CSR Signing](https://icinga.com/docs/icinga-2/latest/doc/06-distributed-monitoring/#on-demand-csr-signing)
section of the Icinga 2 documentation. So, we won't cover that here.

Now, you can start an Icinga 2 agent container using the following command:

```
docker run --detach \
	--network icinga \
	--name icinga-agent \
	--hostname icinga-agent \
	--volume icinga-agent:/data \
	--env ICINGA_ZONE=icinga-agent \
	--env ICINGA_ENDPOINT=icinga-master,icinga-master,5665 \
	--env ICINGA_CACERT="$(< icinga-ca.crt)" \
	--env ICINGA_TICKET="$(< icinga-agent.ticket)" \
	icinga/icinga2
```

This command will run the Icinga 2 agent container in detached mode, mounting the `/data` directory to a persistent
volume named `icinga-agent`. As with the master container, you can adjust the volume name and other parameters as
needed. The environment variables will be processed by the container's entrypoint script and perform a `icinga2 node setup`
on your behalf, configuring the agent to connect to the master.

You can repeat this step for each additional agent you want to add, ensuring that each agent has a unique hostname and
zone name.

## Icinga 2 API

By default, if the `icinga2 node setup` command is run when starting the container, the Icinga 2 API will be enabled,
and it will use a default API user named `root` with a randomly generated password. If you want to use your own API
users and passwords, you can bind mount your api-users file from your host system into the
`/data/etc/icinga2/conf.d/api-users.conf` in the container as described in the [Getting Started](#getting-started)
section.

## Notifications

By default, Icinga 2 does not send notifications when running in a containerized environment. However, it is possible
to enable mail notifications by configuring [msmtp](https://wiki.archlinux.org/title/Msmtp) client in the container.
The binary is already included in the official Icinga 2 container image, so it just needs to be configured. In order
to do this, you can either mount the `/etc/msmtprc` file from your host system into the container or provide the
necessary configuration for the `~icinga/.msmtprc` file via the `MSMTPRC` environment variable.

## Environment Variables

Most of the environment variables are used as parameters for the `icinga2 node setup` command. If you set any of these
variables, and the `node setup` has not been run yet, the entrypoint script will automatically run the command for you.

The following environment variables are available:

| Variable                                                 | Node setup CLI                                                |
|----------------------------------------------------------|---------------------------------------------------------------|
| `ICINGA_ACCEPT_COMMANDS=1`                               | `--accept-commands`                                           |
| `ICINGA_ACCEPT_CONFIG=1`                                 | `--accept-config`                                             |
| `ICINGA_DISABLE_CONFD=1`                                 | `--disable-confd`                                             |
| `ICINGA_MASTER=1`                                        | `--master`                                                    |
| `ICINGA_CN=icinga-master`                                | `--cn icinga-master`                                          |
| `ICINGA_ENDPOINT=icinga-master,2001:db8::192.0.2.9,5665` | `--endpoint icinga-master,2001:db8::192.0.2.9,5665`           |
| `ICINGA_GLOBAL_ZONES=global-config,global-commands`      | `--global_zones global-config --global_zones global-commands` |
| `ICINGA_LISTEN=::,5665`                                  | `--listen ::,5665`                                            |
| `ICINGA_PARENT_HOST=2001:db8::192.0.2.9,5665`            | `--parent_host 2001:db8::192.0.2.9,5665`                      |
| `ICINGA_PARENT_ZONE=master`                              | `--parent_zone master`                                        |
| `ICINGA_TICKET=0123456789abcdef0123456789abcdef01234567` | `--ticket 0123456789abcdef0123456789abcdef01234567`           |
| `ICINGA_ZONE=master`                                     | `--zone master`                                               |

Special variables:

* `ICINGA_TRUSTEDCERT`'s value is written to a temporary file in the container, which is then used
  as the `--trustedcert` parameter for the `icinga2 node setup` command.
* `ICINGA_CACERT`'s value is directly written to the `/var/lib/icinga2/certs/ca.crt` file in the container.

## Build Your Own Image

If you want to build your own Icinga 2 container image, you can clone the Icinga 2 repository and build the image
using the provided `Containerfile`.

```
git clone https://github.com/Icinga/icinga2.git
cd icinga2
docker build --tag icinga/icinga2:test --file Containerfile .
```
