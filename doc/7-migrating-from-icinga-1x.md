# <a id="migrating-from-icinga-1x"></a> Migrating from Icinga 1.x

## <a id="configuration-migration"></a> Configuration Migration

The Icinga 2 configuration format introduces plenty of behavioural changes. In
order to ease migration from Icinga 1.x,
Icinga 2 ships its own config migration script.

### <a id="configuration-migration-script"></a> Configuration Migration Script

Due to the complexity of the Icinga 1.x configuration format the migration
script might not currently work for all use cases.

The config migration script provides support for basic Icinga 1.x
configuration format migration to native Icinga 2 configuration syntax.

The migration script tries to preserve your existing template structure and
adds new templates where appropriate. However, the original file structure is
not preserved.

The migration script uses templates from the Icinga Template Library where
possible.

    # mkdir /etc/icinga2/conf.d/migrate
    # /usr/bin/icinga2-migrate-config -c /etc/icinga/icinga.cfg -o /etc/icinga2/conf.d/migrate

### <a id="manual-config-migration"></a> Manual Config Migration

For a long-term migration of your configuration you should consider re-creating
your configuration based on the Icinga 2 proposed way of doing configuration right.

Please read the [next chapter](#differences-1x-2) to get an idea about the differences between 1.x and 2.
