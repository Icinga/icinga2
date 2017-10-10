### Keys

All Keys are prefixed with "icinga:"

Key                    | Type   | Description     | Dev notes
-----------------------|--------|-----------------|-----------
status:{feature}       | Hash   | Feature status  | Currently the hash only contains one key (name) which returns a json string
{type}state.{SHA1}     | String | Host state      | json string of the current state of the object SHA1 is of the object name
config:{type}          | Hash   | Config          | Has all the config with name as key and json string of config as value
config:{type}:checksum | Hash   | Checksums       | Key is name, returns a json encoded string of checksums
subscription:{name}    | String | sub description | json string describing the subsciption
event:{name}           | List   | sub output      | Publish endpoint for subscription of the same name
:trigger::configchange | List   | ?               | I have no idea what this is or where it comes from
