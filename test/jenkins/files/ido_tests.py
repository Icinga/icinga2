from __future__ import unicode_literals

import sys
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


def success(msg):
    print '[OK] {0}'.format(msg).encode('utf-8')
    sys.stdout.flush()
    return True


def fail(msg):
    print '[FAIL] {0}'.format(msg).encode('utf-8')
    sys.stdout.flush()
    return False


def info(msg):
    print '[INFO] {0}'.format(msg).encode('utf-8')
    sys.stdout.flush()


def validate_tables(tables):
    """
    Return whether all tables of the IDO database scheme exist in
    the given table listing

    """
    info('Checking database scheme... (tables)')
    failures = False
    for table in (TABLE_PREFIX + n for n in TABLES):
        if table in tables:
            success('Found table "{0}" in database'.format(table))
        else:
            fail('Could not find table "{0}" in database'.format(table))
            failures = True

    return not failures


def verify_host_config(config_data):
    """
    Return whether the example hosts exist in the given "hosts" table

    """
    info('Checking example host configuration...')
    failures = False
    for hostname in EXAMPLE_CONFIG:
        if not any(1 for e in config_data if e['alias'] == hostname):
            fail('Could not find host "{0}"'.format(hostname))
            failures = True
        else:
            success('Found host "{0}"'.format(hostname))

    return not failures


def verify_service_config(config_data):
    """
    Return whether the example services exist in the given "services" table

    """
    info('Checking example service configuration...')
    failures = False
    for hostname, servicename in ((h, s) for h, ss in EXAMPLE_CONFIG.iteritems()
                                         for s in ss):
        if not any(1 for c in config_data
                     if c['alias'] == hostname and
                        c['display_name'] == servicename):
            fail('Could not find service "{0}" on host "{1}"'
                 ''.format(servicename, hostname))
            failures = True
        else:
            success('Found service "{0}" on host "{1}"'
                    ''.format(servicename, hostname))

    return not failures


def check_last_host_status_update(check_info):
    """
    Return whether the example hosts are checked as scheduled

    """
    info('Checking last host status updates...')
    failures = False
    for host_info in check_info:
        if host_info['alias'] == 'localhost':
            last_check = datetime.fromtimestamp(float(host_info['last_check']))
            if datetime.now() - last_check > timedelta(minutes=CHECK_INTERVAL,
                                                       seconds=10):
                fail('The last status update of host "{0}" was more than {1} '
                     'minutes ago'.format(host_info['alias'], CHECK_INTERVAL))
                failures = True
            else:
                success('Host "{0}" is being updated'.format(host_info['alias']))
        elif host_info['alias'] == 'nsca-ng':
            if float(host_info['last_check']) > 0:
                fail('The host "{0}" was checked even though it has'
                     ' no check service'.format(host_info['alias']))
                failures = True
            else:
                success('Host "{0}" is not being checked because there '
                        'is no check service'.format(host_info['alias']))
        else:
            info('Skipping host "{0}"'.format(host_info['alias']))

    return not failures


def check_last_service_status_update(check_info):
    """
    Return whether the example services are checked as scheduled

    """
    info('Checking last service status updates...')
    failures = False
    for svc_info in check_info:
        if svc_info['display_name'] in EXAMPLE_CONFIG.get(svc_info['alias'], []):
            last_check = datetime.fromtimestamp(float(svc_info['last_check']))
            if datetime.now() - last_check > timedelta(minutes=CHECK_INTERVAL,
                                                       seconds=10):
                fail('The last status update of service "{0}" on'
                     ' host "{1}" was more than {2} minutes ago'
                     ''.format(svc_info['display_name'], svc_info['alias'],
                               CHECK_INTERVAL))
                failures = True
            else:
                success('Service "{0}" on host "{1}" is being updated'
                        ''.format(svc_info['display_name'], svc_info['alias']))
        else:
            info('Skipping service "{0}" on host "{1}"'
                 ''.format(svc_info['display_name'], svc_info['alias']))

    return not failures


def check_logentries(logentry_info):
    """
    Return whether the given logentry originates from host "localhost"
    and refers to its very last hard status change

    """
    info('Checking status log for host "localhost"...')
    if logentry_info and logentry_info[0]['alias'] == 'localhost':
        entry_time = datetime.fromtimestamp(float(logentry_info[0]['entry_time']))
        state_time = datetime.fromtimestamp(float(logentry_info[0]['state_time']))
        if entry_time - state_time > timedelta(seconds=10):
            return fail('The last hard state of host "localhost"'
                        ' seems not to have been logged')
    else:
        return fail('No logs found in the IDO for host "localhost"')

    return success('The last hard state of host "localhost" was properly logged')

