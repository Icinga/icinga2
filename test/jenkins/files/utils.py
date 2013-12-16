from __future__ import unicode_literals

import os
import json
import socket
import subprocess

__all__ = ['parse_statusdata', 'run_mysql_query', 'run_pgsql_query',
           'LiveStatusSocket']


MYSQL_PARAMS = b"-t -D icinga -u icinga --password=icinga -e".split()
MYSQL_SEPARATOR = '|'

PGSQL_PARAMS = b"-nq -U icinga -d icinga -c".split()
PGSQL_SEPARATOR = '|'
PGSQL_ENVIRONMENT = {
    b'PGPASSWORD': b'icinga'
    }


def parse_statusdata(data, intelligent_cast=True):
    parsed_data, data_type, type_data = {}, '', {}
    for line in (l for l in data.split(os.linesep)
                   if l and not l.startswith('#')):
        if '{' in line:
            data_type = line.partition('{')[0].strip()
        elif '}' in line:
            parsed_data.setdefault(data_type, []).append(type_data)
        else:
            key, _, value = line.partition('=')

            if intelligent_cast:
                value = _cast_status_value(value)

            type_data[key.strip()] = value

    return parsed_data


def _cast_status_value(value):
    try:
        return int(value)
    except ValueError:
        try:
            return float(value)
        except ValueError:
            return value


def run_mysql_query(query, path):
    p = subprocess.Popen([path] + MYSQL_PARAMS + [query.encode('utf-8')],
                         stdout=subprocess.PIPE)
    return _parse_mysql_result([l.decode('utf-8') for l in p.stdout.readlines()])


def _parse_mysql_result(resultset):
    result, header = [], None
    for line in (l for l in resultset if MYSQL_SEPARATOR in l):
        columns = [c.strip() for c in line[1:-3].split(MYSQL_SEPARATOR)]
        if header is None:
            header = columns
        else:
            result.append(dict((header[i], v) for i, v in enumerate(columns)))
    return result


def run_pgsql_query(query, path):
    p = subprocess.Popen([path] + PGSQL_PARAMS + [query.encode('utf-8')],
                         stdout=subprocess.PIPE, env=PGSQL_ENVIRONMENT)
    return _parse_pgsql_result([l.decode('utf-8') for l in p.stdout.readlines()])


def _parse_pgsql_result(resultset):
    result, header = [], None
    for line in (l for l in resultset if PGSQL_SEPARATOR in l):
        columns = [c.strip() for c in line.split(PGSQL_SEPARATOR)]
        if header is None:
            header = columns
        else:
            result.append(dict((header[i], v) for i, v in enumerate(columns)))
    return result


class LiveStatusError(Exception):
    pass


class LiveStatusSocket(object):
    options = [
        'KeepAlive: on',
        'OutputFormat: json',
        'ResponseHeader: fixed16'
        ]

    def __init__(self, path):
        self.path = path

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, tb):
        self.close()

    def connect(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.path)

    def close(self):
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

    def query(self, command):
        self.send(command)
        statuscode, response = self.recv()

        if statuscode != 200:
            raise LiveStatusError(statuscode, response)

        return response

    def send(self, query):
        full_query = '\n'.join([query] + self.options)
        self.sock.sendall((full_query + '\n\n').encode('utf-8'))

    def recv(self):
        response = b''
        response_header = self.sock.recv(16)
        response_code = int(response_header[:3])
        response_length = int(response_header[3:].strip())

        if response_length > 0:
            while len(response) < response_length:
                response += self.sock.recv(response_length - len(response))

            response = response.decode('utf-8')

            try:
                response = json.loads(response)
            except ValueError:
                pass

        return response_code, response

