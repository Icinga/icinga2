# Advanced Topics

## Downtimes

TODO (move to basics?)

## Comments

TODO (move to basics?)

## Acknowledgements

TODO (move to basics?)

## Cluster

TODO

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

