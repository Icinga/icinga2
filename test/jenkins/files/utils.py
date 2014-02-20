from __future__ import unicode_literals

import os
import sys
import time
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
    Logger.debug('Sent MYSQL query: {0!r}\n'.format(query))
    resultset = [l.decode('utf-8') for l in p.stdout.readlines()]
    Logger.debug('Received MYSQL resultset: {0!r}\n'
                 ''.format(''.join(resultset)), True)
    return _parse_mysql_result(resultset)


def _parse_mysql_result(resultset):
    result, header = [], None
    for line in (l for l in resultset if MYSQL_SEPARATOR in l):
        columns = [c.strip() for c in line[1:-3].split(MYSQL_SEPARATOR)]
        if header is None:
            header = columns
        else:
            result.append(dict((header[i], v if v != 'NULL' else None)
                               for i, v in enumerate(columns)))
    return result


def run_pgsql_query(query, path):
    p = subprocess.Popen([path] + PGSQL_PARAMS + [query.encode('utf-8')],
                         stdout=subprocess.PIPE, env=PGSQL_ENVIRONMENT)
    Logger.debug('Sent PostgreSQL query: {0!r}\n'.format(query))
    resultset = [l.decode('utf-8') for l in p.stdout.readlines()]
    Logger.debug('Received PostgreSQL resultset: {0!r}\n'
                 ''.format(''.join(resultset)), True)
    return _parse_pgsql_result(resultset)


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

        self._connected = False

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_value, tb):
        self.close()

    def connect(self):
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        Logger.debug('Opened UNIX stream socket\n', True)
        self.sock.connect(self.path)
        Logger.debug('Connected to Livestatus socket: {0}\n'.format(self.path),
                     True)
        self._connected = True

    def reconnect(self, timeout=30):
        Logger.debug('Reconnecting to Livestatus socket\n', True)
        start = time.time()
        while not self._connected and time.time() - start < timeout:
            try:
                self.connect()
            except socket.error, error:
                Logger.debug('Could not connect: {0}\n'.format(error), True)
                # Icinga2 does some "magic" with the socket during startup
                # which causes random errors being raised (EACCES, ENOENT, ..)
                # so we just ignore them until the timeout is reached
                time.sleep(1)
        if not self._connected:
            # Raise the very last exception once the timeout is reached
            raise

    def close(self):
        if self._connected:
            self.sock.shutdown(socket.SHUT_RDWR)
            Logger.debug('Shutted down Livestatus connection\n', True)
            self.sock.close()
            Logger.debug('Closed Livestatus socket\n', True)
            self._connected = False

    def query(self, command):
        self.send(command)
        statuscode, response = self.recv()

        if statuscode != 200:
            raise LiveStatusError(statuscode, response)

        return response

    def send(self, query):
        if not self._connected:
            raise RuntimeError('Tried to write to closed socket')

        full_query = '\n'.join([query] + self.options)
        self.sock.sendall((full_query + '\n\n').encode('utf-8'))
        Logger.debug('Sent Livestatus query: {0!r}\n'.format(full_query))

    def recv(self):
        if not self._connected:
            raise RuntimeError('Tried to read from closed socket')

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

        Logger.debug('Received Livestatus response: {0!r} (Header was: {1!r})'
                     '\n'.format(response, response_header), True)
        return response_code, response


class Logger(object):
    INFO = 1
    OK = 2
    FAIL = 3
    ERROR = 4
    DEBUG_STD = 5
    DEBUG_EXT = 6

    VERBOSITY = None

    @classmethod
    def permitted(cls, severity):
        if cls.VERBOSITY is None:
            cls.VERBOSITY = next((int(sys.argv.pop(i).partition('=')[2])
                                  for i, a in enumerate(sys.argv)
                                  if a.startswith('--verbosity=')), 1)

        return (severity == cls.INFO and cls.VERBOSITY >= 1) or \
               (severity == cls.OK and cls.VERBOSITY >= 1) or \
               (severity == cls.FAIL and cls.VERBOSITY >= 1) or \
               (severity == cls.ERROR and cls.VERBOSITY >= 1) or \
               (severity == cls.DEBUG_STD and cls.VERBOSITY >= 2) or \
               (severity == cls.DEBUG_EXT and cls.VERBOSITY >= 3)

    @staticmethod
    def write(text, stderr=False):
        if stderr:
            sys.stderr.write(text)
            sys.stderr.flush()
        else:
            sys.stdout.write(text)
            sys.stdout.flush()

    @classmethod
    def log(cls, severity, text):
        if severity == cls.INFO and cls.permitted(cls.INFO):
            cls.write('\x00[INFO] {0}'.format(text))
        elif severity == cls.ERROR and cls.permitted(cls.ERROR):
            cls.write('\x00[ERROR] {0}'.format(text))
        elif severity == cls.FAIL and cls.permitted(cls.FAIL):
            cls.write('\x00[FAIL] {0}'.format(text))
        elif severity == cls.OK and cls.permitted(cls.OK):
            cls.write('\x00[OK] {0}'.format(text))
        elif severity == cls.DEBUG_STD and cls.permitted(cls.DEBUG_STD):
            cls.write('\x00\x00[DEBUG] {0}'.format(text))
        elif severity == cls.DEBUG_EXT and cls.permitted(cls.DEBUG_EXT):
            cls.write('\x00\x00\x00\x00[DEBUG] {0}'.format(text))
        else:
            return False
        return True

    @classmethod
    def info(cls, text):
        return cls.log(cls.INFO, text)

    @classmethod
    def error(cls, text):
        return cls.log(cls.ERROR, text)

    @classmethod
    def fail(cls, text):
        return cls.log(cls.FAIL, text)

    @classmethod
    def ok(cls, text):
        return cls.log(cls.OK, text)

    @classmethod
    def debug(cls, text, extended=False):
        return cls.log(cls.DEBUG_EXT if extended else cls.DEBUG_STD, text)

