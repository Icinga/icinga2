# Icinga2 Redis <a id="icinga-redis"></a>
## Subscriptions and events

Using the redis feature allows you to add subscriptions for events which will then be served by Icinga 2 over Redis.

Possible event types:

* CheckResult
* StateChange
* Notification
* AcknowledgementSet
* AcknowledgementCleared
* CommentAdded
* CommentRemoved
* DowntimeAdded
* DowntimeRemoved
* DowntimeStarted
* DowntimeTriggered

### Creating a subscription
A subscription is created by creating a new key `icinga:subscription:<name>` in redis with a set of event types. All 
events matching the the type of those listed will then be added to a list at `icinga:event:<name>`. The events are 
rotated on a first-in-first-out basis, the default limit is (TODO) and can be overwritten by setting 
`icinga:subscription:<name>:limit` to the desired ammount.

The events are saved as json encoded strings, similar to the API.

It is recommended to use `LPOP` to read from the list and discard read events at the same time.

Example:

```
$ redis-cli
127.0.0.1:6379> SADD icinga:subscription:noma-2 "Notification" "CheckResult"
(integer) 2
127.0.0.1:6379> SET icinga:subscription:noma-2:limit 500
OK
...
127.0.0.1:6379> LRANGE icinga:event:noma-2 0 1
1) "{\"check_result\":{\"active\":true,\"check_source\":\"icinga-1\",\"command\":[\"/usr/lib/nagios/plugins/check_ping\" ... ]}}"
1) "{\"check_result\":{\"active\":true,\"check_source\":\"icinga-2\",\"command\":[\"/usr/lib/nagios/plugins/check_ping\" ... ]}}"
```


All Keys are prefixed with "icinga:"

Key                    | Type   | Description     | Dev notes
-----------------------|--------|-----------------|-----------
status:{feature}       | Hash   | Feature status  | Currently the hash only contains one key (name) which returns a json string
{type}state.{SHA1}     | String | Host state      | json string of the current state of the object SHA1 is of the object name
config:{type}          | Hash   | Config          | Has all the config with name as key and json string of config as value
config:{type}:checksum | Hash   | Checksums       | Key is name, returns a json encoded string of checksums
subscription:{name}    | String | sub description | json string describing the subsciption
event:{name}           | List   | sub output      | Publish endpoint for subscription of the same name
