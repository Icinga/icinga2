# <a id="icinga2-api"></a> Icinga 2 API

## <a id="icinga2-api-introduction"></a> Introduction

The Icinga 2 API allows you to manage configuration objects
and resources in a simple, programmatic way using HTTP requests.

The endpoints are logically separated allowing you to easily
make calls to

* [retrieve information](9-icinga2-api.md#icinga2-api-objects) (status, config)
* run [actions](9-icinga2-api.md#icinga2-api-actions) (reschedule checks, etc.)
* [create/update/delete configuration objects](9-icinga2-api.md#icinga2-api-objects)
* [manage configuration packages](9-icinga2-api.md#icinga2-api-config-management)
* subscribe to [event streams](9-icinga2-api.md#icinga2-api-event-streams)

This chapter will start with a general overview followed by
detailed information about specific endpoints.

### <a id="icinga2-api-requests"></a> Requests

Any tool capable of making HTTP requests can communicate with
the API, for example [curl](http://curl.haxx.se).

Requests are only allowed to use the HTTPS protocol so that
traffic remains encrypted.

By default the Icinga 2 API listens on port `5665` sharing this
port with the cluster communication protocol. This can be changed
by setting the `bind_port` attribute in the [ApiListener](6-object-types.md#objecttype-apilistener)
configuration object in the `/etc/icinga2/features-available/api.conf`
file.

Supported request methods:

  Method	| Usage
  --------------|------------------------------------------------------
  GET		| Retrieve information about configuration objects. Any request using the GET method is read-only and does not affect any objects.
  POST		| Update attributes of a specified configuration object.
  PUT		| Create a new object. The PUT request must include all attributes required to create a new object.
  DELETE	| Remove an object created by the API. The DELETE method is idempotent and does not require any check if the object actually exists.

### <a id="icinga2-api-http-statuses"></a> HTTP Statuses

The API will return standard [HTTP statuses](https://www.ietf.org/rfc/rfc2616.txt)
including error codes.

When an error occurs, the response body will contain additional information
about the problem and its source.

A status in the range of 200 generally means that the request was succesful
and no error was encountered.

Return codes within the 400 range indicate that there was a problem with the
request. Either you did not authenticate correctly, you are missing the authorization
for your requested action, the requested object does not exist or the request
was malformed.

A status in the range of 500 generally means that there was a server-side problem
and Icinga 2 is unable to process your request currently.

Ask your Icinga 2 system administrator to check the `icinga2.log` file for further
troubleshooting.


### <a id="icinga2-api-responses"></a> Responses

Succesful requests will send back a response body containing a `results`
list. Depending on the number of affected objects in your request, the
results may contain one or more entries.

The [output](9-icinga2-api.md#icinga2-api-output) will be sent back as JSON object:


    {
        "results": [
            {
                "code": 200.0,
                "status": "Object was created."
            }
        ]
    }


### <a id="icinga2-api-authentication"></a> Authentication

There are two different ways for authenticating against the Icinga 2 API:

* username and password using HTTP basic auth
* X.509 certificate with client CN

In order to configure a new API user you'll need to add a new [ApiUser](6-object-types.md#objecttype-apiuser)
configuration object. In this example `root` will be the basic auth username
and the `password` attribute contains the basic auth password.

    vim /etc/icinga2/conf.d/api-users.conf

    object ApiUser "root" {
      password = icinga"
    }

Alternatively you can use X.509 client certificates by specifying the `client_cn`
the API should trust.

    vim /etc/icinga2/conf.d/api-users.conf

    object ApiUser "api-clientcn" {
      password = "CertificateCommonName"
    }

An `ApiUser` object can have both methods configured. Sensitive information
such as the password will not be exposed through the API itself.

New installations of Icinga 2 will automatically generate a new `ApiUser`
named `root` with a generated password in the `/etc/icinga2/conf.d/api-users.conf`
file.
You can manually invoke the cli command `icinga2 api setup` which will generate
a new local CA, self-signed certificate and a new API user configuration.

Once the API user is configured make sure to restart Icinga 2:

    # service icinga2 restart

Now pass the basic auth information to curl and send a GET request to the API:

    $ curl -u root:icinga -k -s 'https://nbmif.int.netways.de:5665/v1'

In case you will get `Unauthorized` make sure to check the API user credentials.

### <a id="icinga2-api-permissions"></a> Permissions

**TODO** https://dev.icinga.org/issues/9088

### <a id="icinga2-api-parameters"></a> Parameters

Depending on the request method there are two ways of
passing parameters to the request:

* JSON body (`POST`, `PUT`)
* Query string (`GET`, `DELETE`)

Reserved characters by the HTTP protocol must be passed url-encoded as query string, e.g. a
space becomes `%20`.

Example for query string:

    /v1/hosts?filter=match(%22nbmif*%22,host.name)&attrs=host.name&attrs=host.state

Example for JSON body:

    { "attrs": { "address": "8.8.4.4", "vars.os" : "Windows" } }

**TODO**

#### <a id="icinga2-api-filters"></a> Filters

Use the same syntax as for apply rule expressions
for filtering specific objects.

Example for all services in NOT-OK state:

    https://localhost:5665/v1/services?filter=service.state!=0

Example for matching all hosts by name (**Note**: `"` are url-encoded as `%22`):

    https://localhost:5665/v1/hosts?filter=match(%22nbmif*%22,host.name)

**TODO**



### <a id="icinga2-api-output-format"></a>Output Format

The request and reponse body contain a JSON encoded string.

### <a id="icinga2-api-version"></a>Version

Each url contains the version string as prefix (currently "/v1").

### <a id="icinga2-api-url-overview"></a>Url Overview


The Icinga 2 API provides multiple url endpoints

  Url Endpoints	| Description
  --------------|----------------------------------------------------
  /v1/actions	| Endpoint for running specific [API actions](9-icinga2-api.md#icinga2-api-actions).
  /v1/config    | Endpoint for [managing configuration modules](9-icinga2-api.md#icinga2-api-config-management).
  /v1/events	| Endpoint for subscribing to [API events](9-icinga2-api.md#icinga2-api-actions).
  /v1/types	| Endpoint for listing Icinga 2 configuration object types and their attributes.

Additionally there are endpoints for each [config object type](6-object-types.md#object-types):

**TODO** Update

  Url Endpoints		| Description
  ----------------------|----------------------------------------------------
  /v1/hosts		| Endpoint for retreiving and updating [Host](6-object-types.md#objecttype-host) objects.
  /v1/services		| Endpoint for retreiving and updating [Service](6-object-types.md#objecttype-service) objects.
  /v1/notifications	| Endpoint for retreiving and updating [Notification](6-object-types.md#objecttype-notification) objects.
  /v1/dependencies	| Endpoint for retreiving and updating [Dependency](6-object-types.md#objecttype-dependency) objects.
  /v1/users		| Endpoint for retreiving and updating [User](6-object-types.md#objecttype-user) objects.
  /v1/checkcommands	| Endpoint for retreiving and updating [CheckCommand](6-object-types.md#objecttype-checkcommand) objects.
  /v1/eventcommands	| Endpoint for retreiving and updating [EventCommand](6-object-types.md#objecttype-eventcommand) objects.
  /v1/notificationcommands | Endpoint for retreiving and updating [NotificationCommand](6-object-types.md#objecttype-notificationcommand) objects.
  /v1/hostgroups	| Endpoint for retreiving and updating [HostGroup](6-object-types.md#objecttype-hostgroup) objects.
  /v1/servicegroups	| Endpoint for retreiving and updating [ServiceGroup](6-object-types.md#objecttype-servicegroup) objects.
  /v1/usergroups	| Endpoint for retreiving and updating [UserGroup](6-object-types.md#objecttype-usergroup) objects.
  /v1/zones		| Endpoint for retreiving and updating [Zone](6-object-types.md#objecttype-zone) objects.
  /v1/endpoints		| Endpoint for retreiving and updating [Endpoint](6-object-types.md#objecttype-endpoint) objects.
  /v1/timeperiods	| Endpoint for retreiving and updating [TimePeriod](6-object-types.md#objecttype-timeperiod) objects.



## <a id="icinga2-api-actions"></a> Actions

There are several actions available for Icinga 2 provided by the `actions` url endpoint.

In case you have been using the [external commands](5-advanced-topics.md#external-commands)
in the past, the API actions provide a yet more powerful interface with
filters and even more functionality.

Actions require specific target types (e.g. `type=Host`) and a [filter](9-icinga2-api.md#)

**TODO**

Action name                            | Parameters                        | Target types             | Notes
---------------------------------------|-----------------------------------|--------------------------|-----------------------
process-check-result                   | exit_status; plugin_output; check_source; performance_data[]; check_command[]; execution_end; execution_start; schedule_end; schedule_start | Service; Host | -
reschedule-check                       | {next_check}; {(force_check)} | Service; Host | -
acknowledge-problem                    | author; comment; {timestamp}; {(sticky)}; {(notify)} | Service; Host | -
remove-acknowledgement                 | - | Service; Host | -
add-comment                            | author; comment | Service; Host | -
remove-comment                         | - | Service;Host | -
remove-comment-by-id                   | comment_id | - | -
delay-notifications                    | timestamp | Service;Host | -
add-downtime                           | start_time; end_time; duration; author; comment; {trigger_id}; {(fixed)} | Service; Host; ServiceGroup; HostGroup | Downtime for all services on host x?
remove-downtime                        | - | Service; Host | -
remove-downtime-by-id                  | downtime_id | - | -
send-custom-notification               | options[]; author; comment | Service; Host | -

enable-passive-checks                  | - | Service; Host; ServiceGroup; HostGroup | "System" as target?
disable-passive-checks                 | - | Service; Host; ServiceGroup; HostGroup | diable all passive checks for services of hosts y in hostgroup x?
enable-active-checks                   | - | Host; HostGroup | -
disable-active-checks                  | - | Host; HostGroup | -
enable-notifications                   | - | Service; Host; ServiceGroup; HostGroup | Enable all notifications for services of host x?
disable-notifications                  | - | Service; Host; ServiceGroup; HostGroup | -
enable-flap-detection                  | - | Service; Host; ServiceGroup; HostGroup | -
disable-flap-detection                 | - | Service; Host; ServiceGroup; HostGroup | -
enable-event-handler                   | - | Service; Host | -
disable-event-handler                  | - | Service; Host | -

enable-global-notifications            | - | - | -
disable-global-notifications           | - | - | -
enable-global-flap-detection           | - | - | -
disable-global-flap-detection          | - | - | -
enable-global-event-handlers           | - | - | -
disable-global-event-handlers          | - | - | -
enable-global-performance-data         | - | - | -
disable-global-performance-data        | - | - | -
start-global-executing-svc-checks      | - | - | -
stop-global-executing-svc-checks       | - | - | -
start-global-executing-host-checks     | - | - | -
stop-global-executing-host-checks      | - | - | -
shutdown-process                       | - | - | -
restart-process                        | - | - | -


Examples:

Reschedule a service check for all services in NOT-OK state:

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/actions/reschedule-check?filter=service.state!=0&type=Service' -X POST | python -m json.tool
    {
        "results": [
            {
                "code": 200.0,
                "status": "Successfully rescheduled check for nbmif.int.netways.de!http."
            },
            {
                "code": 200.0,
                "status": "Successfully rescheduled check for nbmif.int.netways.de!disk."
            },
            {
                "code": 200.0,
                "status": "Successfully rescheduled check for nbmif.int.netways.de!disk /."
            }
        ]
    }




## <a id="icinga2-api-event-streams"></a> Event Streams

**TODO** https://dev.icinga.org/issues/9078



## <a id="icinga2-api-objects"></a> API Objects

Provides functionality for all configuration object url endpoints listed
[here](9-icinga2-api.md#icinga2-api-url-overview).

### <a id="icinga2-api-objects"></a> API Objects and Cluster Config Sync

Newly created or updated objects can be synced throughout your
Icinga 2 cluster. Set the `zone` attribute to the zone this object
belongs to and let the API and cluster handle the rest.

If you add a new cluster instance, or boot an instance beeing offline
for a while, Icinga 2 takes care of the initial object sync for all
objects created by the API.

More information about distributed monitoring, cluster and its
configuration can be found [here](13-distributed-monitoring-ha.md#distributed-monitoring-high-availability).

### <a id="icinga2-api-hosts"></a> Hosts

All object attributes are prefixed with their respective object type.

Example:

    host.address

Output listing and url parameters use the same syntax.

#### <a id="icinga2-api-hosts-list"></a> List All Hosts

Send a `GET` request to `/v1/hosts` to list all host objects and
their attributes.

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts'


#### <a id="icinga2-api-hosts-create"></a> Create New Host Object

New objects must be created by sending a PUT request. The following
parameters need to be passed inside the JSON body:

  Parameters	| Description
  --------------|------------------------------------
  name		| **Optional.** If not specified inside the url, this is **required**.
  templates	| **Optional.** Import existing configuration templates, e.g. `generic-host`.
  attrs		| **Required.** Set specific [Host](6-object-types.md#objecttype-host) object attributes.


If attributes are of the Dictionary type, you can also use the indexer format:

    "attrs": { "vars.os": "Linux" }

Example:

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts/google.com' \
    -X PUT \
    -d '{ "templates": [ "generic-host" ], "attrs": { "address": "8.8.8.8", "vars.os" : "Linux" } }' \
    | python -m json.tool
    {
        "results": [
            {
                "code": 200.0,
                "status": "Object was created."
            }
        ]
    }

**Note**: Host objects require the `check_command` attribute. In the example above the `generic-host`
template already provides such.

If the configuration validation fails, the new object will not be created and the response body
contains a detailed error message. The following example omits the required `check_command` attribute.

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts/google.com' \
    -X PUT \
    -d '{ "attrs": { "address": "8.8.8.8", "vars.os" : "Linux" } }' \
    | python -m json.tool
    {
        "results": [
            {
                "code": 500.0,
                "errors": [
                    "Error: Validation failed for object 'google.com' of type 'Host'; Attribute 'check_command': Attribute must not be empty."
                ],
                "status": "Object could not be created."
            }
        ]
    }

#### <a id="icinga2-api-hosts-show"></a> Show Host

Send a `GET` request including the host name inside the url:

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts/google.com'

You can select specific attributes by adding them as url parameters using `?attrs=...`. Multiple
attributes must be added one by one, e.g. `?attrs=host.address&attrs=host.name`.

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts/google.com?attrs=host.name&attrs=host.address' | python -m json.tool
    {
        "results": [
            {
                "attrs": {
                    "host.address": "8.8.8.8",
                    "host.name": "google.com"
                }
            }
        ]
    }

#### <a id="icinga2-api-hosts-modify"></a> Modify Host

Existing objects must be modifed by sending a `POST` request. The following
parameters need to be passed inside the JSON body:

  Parameters	| Description
  --------------|------------------------------------
  name		| **Optional.** If not specified inside the url, this is **required**.
  templates	| **Optional.** Import existing configuration templates, e.g. `generic-host`.
  attrs		| **Required.** Set specific [Host](6-object-types.md#objecttype-host) object attributes.


If attributes are of the Dictionary type, you can also use the indexer format:

    "attrs": { "vars.os": "Linux" }


Example for existing object `google.com`:

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts/google.com' \
    -X POST \
    -d '{ "attrs": { "address": "8.8.4.4", "vars.os" : "Windows" } }' \
    | python -m json.tool
    {
        "results": [
            {
                "code": 200.0,
                "name": "google.com",
                "status": "Attributes updated.",
                "type": "Host"
            }
        ]
    }

#### <a id="icinga2-api-hosts-delete"></a> Delete Host

You can delete objects created using the API by sending a `DELETE`
request. Specify the object name inside the url.

  Parameters	| Description
  --------------|------------------------------------
  cascade	| **Optional.** Delete objects depending on the deleted objects (e.g. services on a host).

Example:

    $ curl -u root:icinga -k -s 'https://localhost:5665/v1/hosts/google.com?cascade=1' -X DELETE | python -m json.tool
    {
        "results": [
            {
                "code": 200.0,
                "name": "google.com",
                "status": "Object was deleted.",
                "type": "Host"
            }
        ]
    }



**TODO** Add more config objects



## <a id="icinga2-api-config-management"></a> Configuration Management

The main idea behind configuration management is to allow external applications
creating configuration packages and stages based on configuration files and
directory trees. This replaces any additional SSH connection and whatnot to
dump configuration files to Icinga 2 directly.
In case you’re pushing a new configuration stage to a package, Icinga 2 will
validate the configuration asynchronously and populate a status log which
can be fetched in a separated request.

### <a id="icinga2-api-config-management-create-package"></a> Create Config Package

Send a `POST` request to a new config package called `puppet` in this example. This
will create a new empty configuration package.

    $ curl -k -s -u root:icinga -X POST https://localhost:5665/v1/config/packages/puppet | python -m json.tool
    {
        "results": [
            {
                "code": 200.0,
                "package": "puppet",
                "status": "Created package."
            }
        ]
    }

### <a id="icinga2-api-config-management-create-config-stage"></a> Create Configuration to Package Stage

Send a `POST` request to the url endpoint `/v1/config/stages` including an existing
configuration package, e.g. `puppet`.
The request body must contain the `files` attribute with the value being
a dictionary of file targets and their content.

The example below will create a new file called `test.conf` underneath the `conf.d`
directory populated by the sent configuration.
The Icinga 2 API returns the `package` name this stage was created for, and also
generates a unique name for the `package` attribute you'll need for later requests.

Note: This example contains an error (`chec_command`), do not blindly copy paste it.

    $ curl -k -s -u root:icinga -X POST -d '{ "files": { "conf.d/test.conf": "object Host \"cfg-mgmt\" { chec_command = \"dummy\" }" } }' https://localhost:5665/v1/config/stages/puppet | python -m json.tool
    {
        "results": [
            {
                "code": 200.0,
                "package": "puppet",
                "stage": "nbmif-1441625839-0",
                "status": "Created stage."
            }
        ]
    }

If the configuration fails, the old active stage will remain active.
If everything is successful, the new config stage is activated and live.
Older stages will still be available in order to have some sort of revision
system in place.

Icinga 2 automatically creates the following files in the main configuration package
stage:

  File		| Description
  --------------|---------------------------
  status	| Contains the [configuration validation](8-cli-commands.md#config-validation) exit code (everything else than 0 indicates an error).
  startup.log	| Contains the [configuration validation](8-cli-commands.md#config-validation) output.

You can [fetch these files](9-icinga2-api.md#icinga2-api-config-management-fetch-config-package-stage-files) via API call
after creating a new stage.

### <a id="icinga2-api-config-management-list-config-packages"></a> List Configuration Packages and their Stages

List all config packages, their active stage and other stages.
That way you may iterate of all of them programmatically for
older revisions and their requests.

The following example contains one configuration package `puppet`.
The latter already has a stage created, but it is not active.

    $ curl -k -s -u root:icinga https://localhost:5665/v1/config/packages | python -m json.tool
    {
        "results": [
            {
                "active-stage": "",
                "name": "puppet",
                "stages": [
                    "nbmif-1441625839-0"
                ]
            }
        ]
    }

### <a id="icinga2-api-config-management-list-config-package-stage-files"></a> List Configuration Packages and their Stages

Sent a `GET` request to the url endpoint `/v1/config/stages` including the package
(`puppet`) and stage (`nbmif-1441625839-0`) name.

    $ curl -k -s -u root:icinga https://localhost:5665/v1/config/stages/puppet/nbmif-1441625839-0 | python -m json.tool
    {
        "results": [
    ...
            {
                "name": "startup.log",
                "type": "file"
            },
            {
                "name": "status",
                "type": "file"
            },
            {
                "name": "conf.d",
                "type": "directory"
            },
            {
                "name": "zones.d",
                "type": "directory"
            },
            {
                "name": "conf.d/test.conf",
                "type": "file"
            }
        ]
    }


### <a id="icinga2-api-config-management-fetch-config-package-stage-files"></a> Fetch Configuration Package Stage Files

Send a `GET` request to the url endpoint `/v1/config/files` including
the package name, the stage name and the relative path to the file.
Note: You cannot use dots in paths.

You can fetch a [list of existing files](9-icinga2-api.md#icinga2-api-config-management-list-config-package-stage-files)
in a configuration stage and then specifically request their content.

The following example fetches the faulty configuration inside `conf.d/test.conf`
for further analysis.

    $ curl -k -s -u root:icinga https://localhost:5665/v1/config/files/puppet/nbmif-1441625839-0/conf.d/test.conf 
    object Host "cfg-mgmt" { chec_command = "dummy" }

Note: The returned files are plain-text instead of JSON-encoded.

### <a id="icinga2-api-config-management-config-package-stage-errors"></a> Configuration Package Stage Errors

Now that we don’t have an active stage for `puppet` yet seen [here](9-icinga2-api.md#icinga2-api-config-management-list-config-packages),
there must have been an error.

Fetch the `startup.log` file and check the config validation errors:

    $ curl -k -s -u root:icinga https://localhost:5665/v1/config/files/puppet/imagine-1441133065-1/startup.log
    ...
    
    critical/config: Error: Attribute 'chec_command' does not exist.
    Location:
    /var/lib/icinga2/api/packages/puppet/imagine-1441133065-1/conf.d/test.conf(1): object Host "cfg-mgmt" { chec_command = "dummy" }
                                                                                                           ^^^^^^^^^^^^^^^^^^^^^^
    
    critical/config: 1 error

The output is similar to the manual [configuration validation](8-cli-commands.md#config-validation).

