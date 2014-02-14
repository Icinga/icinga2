#!/usr/bin/env python
from __future__ import unicode_literals


import os
import re
import sys
import json
import glob
import subprocess
from optparse import OptionParser
from xml.dom.minidom import getDOMImplementation


try:
    from subprocess import DEVNULL
except ImportError:
    DEVNULL = open(os.devnull, 'w')


class Logger(object):
    INFO = 1
    ERROR = 2
    FAIL = 3
    OK = 4

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
        if severity == cls.INFO:
            cls.write('\033[1;94m[INFO]\033[1;0m {0}'.format(text))
        elif severity == cls.ERROR:
            cls.write('\033[1;33m[ERROR]\033[1;0m {0}'.format(text), True)
        elif severity == cls.FAIL:
            cls.write('\033[1;31m[FAIL] {0}\033[1;0m'.format(text))
        elif severity == cls.OK:
            cls.write('\033[1;32m[OK]\033[1;0m {0}'.format(text))

    @classmethod
    def info(cls, text):
        cls.log(cls.INFO, text)

    @classmethod
    def error(cls, text):
        cls.log(cls.ERROR, text)

    @classmethod
    def fail(cls, text):
        cls.log(cls.FAIL, text)

    @classmethod
    def ok(cls, text):
        cls.log(cls.OK, text)


class TestSuite(object):
    def __init__(self, configpath):
        self._tests = []
        self._results = {}

        self.load_config(configpath)

    def add_test(self, filepath):
        self._tests.append(filepath)

    def load_config(self, filepath):
        with open(filepath) as f:
            self._config = json.load(f)

    def get_report(self):
        dom = getDOMImplementation()
        document = dom.createDocument(None, 'testsuite', None)
        xml_root = document.documentElement

        for name, info in self._results.iteritems():
            testresult = document.createElement('testcase')
            testresult.setAttribute('classname', 'vm')
            testresult.setAttribute('name', name)

            systemout = document.createElement('system-out')
            systemout.appendChild(document.createTextNode(info['stdout']))
            testresult.appendChild(systemout)

            systemerr = document.createElement('system-err')
            systemerr.appendChild(document.createTextNode(info['stderr']))
            testresult.appendChild(systemerr)

            if info['returncode'] != 0:
                failure = document.createElement('failure')
                failure.setAttribute('type', 'returncode')
                failure.appendChild(document.createTextNode(
                    'code: {0}'.format(info['returncode'])))
                testresult.appendChild(failure)

            xml_root.appendChild(testresult)

        return document.toxml()

    def run(self):
        for path in self._tests:
            test_name = os.path.basename(path)
            Logger.info('Copying test "{0}" to remote machine\n'.format(test_name))
            self._copy_test(path)
            self._apply_setup_routines(test_name, 'setup')
            Logger.info('Running test "{0}"...\n'.format(test_name))
            result = self._run_test(path)
            Logger.info('Test "{0}" has finished (Total tests: {1}, Failures: {2})\n'
                        ''.format(test_name, result['total'], result['failures']))
            self._apply_setup_routines(test_name, 'teardown')
            Logger.info('Removing test "{0}" from remote machine\n'.format(test_name))
            self._remove_test(test_name)
            self._results[test_name] = result
            Logger.write('\n')

    def _apply_setup_routines(self, test_name, context):
        instructions = next((t[1].get(context)
                             for t in self._config.get('setups', {}).iteritems()
                             if re.match(t[0], test_name)), None)
        if instructions is not None:
            Logger.info('Applying {0} routines for test "{1}" .. '
                        ''.format(context, test_name))
            for instruction in instructions.get('copy', []):
                source, _, destination = instruction.partition('>>')
                self._copy_file(source.strip(), destination.strip())
            for filepath in instructions.get('clean', []):
                self._remove_file(filepath)
            for command in instructions.get('exec', []):
                self._exec_command(command)
            Logger.write('Done\n')

    def _remove_file(self, path):
        command = self._config['commands']['clean'].format(path)
        rc = subprocess.call(command, stdout=DEVNULL, shell=True)
        if rc != 0:
            Logger.error('Cannot remove file "{0}" ({1})\n'.format(path, rc))

    def _exec_command(self, command):
        command = self._config['commands']['exec'].format(command)
        rc = subprocess.call(command, stdout=DEVNULL, shell=True)
        if rc != 0:
            Logger.error('Command "{0}" exited with exit code "{1}"' \
                         ''.format(command, rc))

    def _copy_file(self, source, destination):
        command = self._config['commands']['copy'].format(source, destination)
        rc = subprocess.call(command, stdout=DEVNULL, shell=True)
        if rc != 0:
            Logger.error('Cannot copy file "{0}" to "{1}" ({2})' \
                         ''.format(source, destination, rc))

    def _copy_test(self, path):
        self._copy_file(path, os.path.join(self._config['settings']['test_root'],
                                           os.path.basename(path)))

    def _remove_test(self, test_name):
        test_root = self._config['settings']['test_root']
        self._remove_file(os.path.join(test_root, test_name))

    def _run_test(self, path):
        command = self._config['commands']['exec']
        target = os.path.join(self._config['settings']['test_root'],
                              os.path.basename(path))
        p = subprocess.Popen(command.format(target), stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, shell=True)
        output, test_count, failed_tests = self._watch_output(p.stdout)
        return {
            'total': test_count,
            'failures': failed_tests,
            'stdout': output,
            'stderr': p.stderr.read().decode('utf-8'),
            'returncode': p.wait()
            }

    def _watch_output(self, pipe):
        output, total, failures = '', 0, 0
        while True:
            line = pipe.readline().decode('utf-8')
            if not line:
                break

            if line.startswith('[ERROR] '):
                Logger.error(line[8:])
            elif line.startswith('[FAIL] '):
                Logger.fail(line[7:])
                failures += 1
                total += 1
            elif line.startswith('[OK] '):
                Logger.ok(line[5:])
                total += 1
            else:
                Logger.info(line.replace('[INFO] ', ''))

            output += line
        return (output, total, failures)


def parse_commandline():
    parser = OptionParser(version='0.2')
    parser.add_option('-C', '--config', default="run_tests.conf",
                      help='The path to the config file to use [%default]')
    parser.add_option('-R', '--results',
                      help='The file where to store the test results')
    return parser.parse_args()


def main():
    options, arguments = parse_commandline()
    suite = TestSuite(options.config)

    for path in (p for a in arguments for p in glob.glob(a)):
        suite.add_test(path)

    suite.run()

    if options.results is not None:
        with open(options.results, 'w') as f:
            f.write(suite.get_report().encode('utf-8'))

    return 0


if __name__ == '__main__':
    sys.exit(main())

