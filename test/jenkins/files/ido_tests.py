from __future__ import unicode_literals

from datetime import datetime, timedelta

CHECK_INTERVAL = 10 # minutes; The actual interval are 5 minutes but as other
                    # tests might restart Icinga we need to take any
                    # rescheduling into account

TABLE_PREFIX = 'icinga_'
TABLES = [
    # Central tables
    'instances',
    'objects',
    # Debugging tables
    'conninfo',
    # Historical tables
    'acknowledgements',
    'commenthistory',
    'contactnotifications',
    'dbversion',
    'downtimehistory',
    'eventhandlers',
    'externalcommands',
    'flappinghistory',
    'hostchecks',
    'logentries',
    'notifications',
    'processevents',
    'servicechecks',
    'statehistory',
    'systemcommands',
    # Current status tables
    'comments',
    'customvariablestatus',
    'hoststatus',
    'programstatus',
    'runtimevariables',
    'scheduleddowntime',
    'servicestatus',
    'contactstatus',
    # Configuration tables
    'commands',
    'configfiles',
    'configfilevariables',
    'contact_addresses',
    'contact_notificationcommands',
    'contactgroup_members',
    'contactgroups',
    'contactnotificationmethods',
    'contacts',
    'customvariables',
    'host_contactgroups',
    'host_contacts',
    'host_parenthosts',
    'hostdependencies',
    'hostescalation_contactgroups',
    'hostescalation_contacts',
    'hostescalations',
    'hostgroup_members',
    'hostgroups',
    'hosts',
    'service_contactgroups',
    'service_contacts',
    'servicedependencies',
    'serviceescalation_contactgroups',
    'serviceescalation_contacts',
    'serviceescalations',
    'servicegroup_members',
    'servicegroups',
    'services',
    'timeperiod_timeranges',
    'timeperiods'
    ]
EXAMPLE_CONFIG = {
    'localhost': ['disk', 'http', 'icinga', 'load', 'ping4',
                  'ping6', 'processes', 'ssh', 'users'],
    'nsca-ng': ['PassiveService1', 'PassiveService2']
}


def validate_tables(tables):
    """
    Return whether all tables of the IDO database scheme exist in
    the given table listing

    """
    missing = [n for n in TABLES if TABLE_PREFIX + n not in tables]
    if missing:
        print 'Some tables are missing in the IDO'
        print 'Missing tables: ' + ', '.join(missing)
        return False

    print 'All tables were found in the IDO'
    return True


def verify_host_config(config_data):
    """
    Return whether the example hosts exist in the given "hosts" table

    """
    if len([1 for e in config_data if e['alias'] in EXAMPLE_CONFIG]) == 2:
        print 'All example hosts are stored in the IDO'
        return True

    print 'Some example hosts are missing in the IDO'
    return False


def verify_service_config(config_data):
    """
    Return whether the example services exist in the given "services" table

    """
    for hostname, servicename in ((h, s) for h, ss in EXAMPLE_CONFIG.iteritems()
                                         for s in ss):
        # Not very efficient, but suitable for just two hosts...
        if not any(1 for c in config_data
                     if c['alias'] == hostname and
                        c['display_name'] == servicename):
            print 'The config stored in the IDO is missing some services'
            return False

    print 'The service config stored in the IDO is correct'
    return True


def check_last_host_status_update(check_info):
    """
    Return whether the example hosts are checked as scheduled

    """
    for info in check_info:
        if info['alias'] == 'localhost':
            last_check = datetime.fromtimestamp(float(info['last_check']))
            if datetime.now() - last_check > timedelta(minutes=CHECK_INTERVAL,
                                                       seconds=10):
                print 'The last status update of host "localhost"' \
                      ' was more than {0} minutes ago'.format(CHECK_INTERVAL)
                return False
        elif info['alias'] == 'nsca-ng':
            if float(info['last_check']) > 0:
                print 'The host "nsca-ng" was checked even though' \
                      ' it should not be actively checked'
                return False

    print 'The updates of both example hosts are processed as configured'
    return True


def check_last_service_status_update(check_info):
    """
    Return whether the example services are checked as scheduled

    """
    for info in check_info:
        if info['display_name'] in EXAMPLE_CONFIG.get(info['alias'], []):
            last_check = datetime.fromtimestamp(float(info['last_check']))
            if datetime.now() - last_check > timedelta(minutes=CHECK_INTERVAL,
                                                       seconds=10):
                print 'The last status update of service "{0}" of' \
                      ' host "{1}" was more than {2} minutes ago' \
                      ''.format(info['display_name'], info['alias'],
                                CHECK_INTERVAL)
                return False

    print 'The updates of all example services are processed as configured'
    return True


def check_logentries(logentry_info):
    """
    Return whether the given logentry originates from host "localhost"
    and refers to its very last hard status change

    """
    if logentry_info and logentry_info[0]['alias'] == 'localhost':
        entry_time = datetime.fromtimestamp(float(logentry_info[0]['entry_time']))
        state_time = datetime.fromtimestamp(float(logentry_info[0]['state_time']))
        if entry_time - state_time > timedelta(seconds=10):
            print 'The last hard state of host "localhost"' \
                  ' seems not to have been logged'
            return False
    else:
        print 'No logs found in the IDO for host "localhost"'
        return False

    print 'The last hard state of host "localhost" was properly logged'
    return True

