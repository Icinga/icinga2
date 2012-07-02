#!/usr/bin/env python
import sys
import re

def readObject():
    inObject = False

    obj = {
        'type': None,
        'properties': {}
    }

    for line in sys.stdin:
        # remove new-line as well as other whitespace characters
        line = line.strip()

        # replace tabs with space
        line = line.replace("\t", ' ')

        # ignore comments and empty lines
        if line == '' or line[0] == '#':
            continue

        if not inObject:
            match = re.match('^define +([^ ]+) *{$', line)

            if not match:
                raise ValueError('Invalid line in config file: ' + line)

            obj['type'] = match.group(1)
            inObject = True
        else:
            match = re.match('^}$', line)

            if match:
                return obj

            match = re.match('^ *([^ ]+) *(.*)$', line)

            if match:
                obj['properties'][match.group(1)] = match.group(2)
            else:
                raise ValueError('Invalid line in config file: ' + line)

    return None

def dumpValue(obj, indent = 0):
    result = '';

    indent += 1

    if isinstance(obj, dict):
        result = "{\n"

        for k, v in obj.iteritems():
            op = '+=' if isinstance(v, (dict, list)) else '='
            result += "\t" * indent + k + ' ' + op + ' ' + dumpValue(v, indent) + ",\n"

        result += "\t" * (indent - 1) + "}"
    elif isinstance(obj, list):
        result = "{\n"

        for v in obj:
            result += "\t" * indent + dumpValue(v) + ",\n"

        result += "\t" * (indent - 1) + "}"
    elif isinstance(obj, (int, long)):
        result = str(obj)
    else:
        result = ''.join(['"', str(obj), '"'])

    return result

def printObject(obj):
    if 'abstract' in obj and obj['abstract']:
        print 'abstract',

    if 'local' in obj and obj['local']:
        print 'local',

    if 'temporary' in obj and obj['temporary']:
        print 'temporary',

    print 'object', obj['type'], ''.join(['"', obj['name'], '"']),

    if 'parents' in obj and len(obj['parents']) > 0:
        print 'inherits',
        print ', '.join([''.join(['"', parent, '"']) for parent in obj['parents']]),

    print dumpValue(obj['properties'])
    print

nagios_svc_template = {
    'name': 'nagios-service',
    'type': 'service',
    'abstract': True,
    'properties': {
        'check_type': 'nagios',
        'macros': {
            'USER1': '/tmp/nagios/plugins',
            'SERVICESTATE': 0,
            'SERVICEDURATIONSEC': 0,
            'TOTALHOSTSERVICESCRITICAL': 0,
            'TOTALHOSTSERVICESWARNING': 0
        }
    }
}

printObject(nagios_svc_template)

allObjects = []
objects = {}

while True:
    obj = readObject()

    if obj == None:
        break

    props = obj['properties']

    # transform the name property
    name = None
    for prop in [obj['type'] + '_name', 'name', 'service_description']:
        if prop in props:
            if prop == 'service_description':
                name = props[prop] + '-' + props['host_name']
            else:
                name = props[prop]

            if prop != 'service_description':
                del props[prop]

            break

    if name == None:
        raise ValueError('Object has no name: ' + str(obj))

    obj['name'] = name

    if not obj['type'] in objects:
        objects[obj['type']] = {}

    allObjects.append(obj)
    objects[obj['type']][obj['name']] = obj

for obj in allObjects:
    props = obj['properties']
    newprops = {}

    obj['parents'] = []

    # transform 'register' property
    if 'register' in props:
        if int(props['register']) == 0:
            obj['abstract'] = True

        del props['register']

    # transform 'use' property
    if 'use' in props:
        obj['parents'] = props['use'].split(',')
        del props['use']

    # transform commands into service templates
    if obj['type'] == 'command':
        obj['abstract'] = True
        obj['type'] = 'service'
        obj['parents'].append('nagios-service')

        if 'command_line' in props:
            newprops['check_command'] = props['command_line']
            del props['command_line']

    # transform contactgroups/hostgroups/servicegroups
    elif obj['type'] in ['contactgroup', 'hostgroup', 'servicegroup']:
        if 'alias' in props:
            newprops['alias'] = props['alias']
            del props['alias']

        if 'members' in props:
            newprops['members'] = props['members'].split(',')
            del props['members']

    # transform services
    elif obj['type'] == 'service':
        newprops['macros'] = {}

        if 'check_command' in props:
            tokens = props['check_command'].split('!')
            obj['parents'].append(tokens[0])

            num = 0
            for token in tokens[1:]:
                num += 1
                newprops['macros']['ARG' + str(num)] = token

            del props['check_command']

        if 'check_interval' in props:
            newprops['check_interval'] = int(float(props['check_interval']) * 60)
            del props['check_interval']

        if 'retry_interval' in props:
            newprops['retry_interval'] = int(float(props['retry_interval']) * 60)
            del props['retry_interval']

        if 'max_check_attempts' in props:
            newprops['max_check_attempts'] = int(props['max_check_attempts'])
            del props['max_check_attempts']

        newprops['macros']['SERVICEDESC'] = obj['name']

        if 'host_name' in props:
            newprops['host_name'] = props['host_name']
            newprops['macros']['HOSTNAME'] = props['host_name']
            del props['host_name']

        if 'service_description' in props:
            newprops['alias'] = props['service_description']
            del props['service_description']

        for k, v in props.iteritems():
            if k[0] == '_':
                newprops['macros'][k] = v

    obj['properties'] = newprops

    #if len(props) > 0:
    #    obj['properties']['old'] = props

    printObject(obj)

