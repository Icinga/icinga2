# Migrating from Icinga 1.x

## Configuration Migration

The Icinga 2 configuration format introduces plenty of behavioral changes. In
order to ease migration from Icinga 1.x,
Icinga 2 ships its own config migration script.

### Configuration Migration Script

Due to the complexity of the Icinga 1.x configuration format the migration
script might not currently work for all use cases.

The config migration script provides support for basic Icinga 1.x
configuration format migration to native Icinga 2 configuration syntax.

The migration script tries to preserve your existing template structure and
adds new templates where appropriate. However, the original file structure is
not preserved.

The migration script uses templates from the Icinga Template Library where
possible.

> **Note**
>
> Please check the provided README file for additional notes and possible
> caveats.

    # mkdir /etc/icinga2/conf.d/migrate
    # /usr/bin/icinga2-migrate-config -c /etc/icinga/icinga.cfg -o /etc/icinga2/conf.d/migrate


### Manual Config Migration

For a long-term migration of your configuration you should consider re-creating
your configuration based on the Icinga 2 proposed way of doing configuration right.

Please read the next chapter to get an idea about the differences between 1.x and 2.
