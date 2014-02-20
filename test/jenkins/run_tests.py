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
    OK = 2
    FAIL = 3
    ERROR = 4
    DEBUG_STD = 5
    DEBUG_EXT = 6

    VERBOSITY = 0
    OUTPUT_LENGTH = 1024

    @staticmethod
    def write(text, stderr=False):
        if stderr:
            sys.stderr.write(text.encode('utf-8'))
            sys.stderr.flush()
        else:
            sys.stdout.write(text.encode('utf-8'))
            sys.stdout.flush()

    @classmethod
    def set_verbosity(cls, verbosity):
        cls.VERBOSITY = verbosity

    @classmethod
    def log(cls, severity, text):
        if severity == cls.INFO and cls.VERBOSITY >= 1:
            cls.write('\033[1;94m[INFO]\033[1;0m {0}'.format(text))
        elif severity == cls.ERROR and cls.VERBOSITY >= 1:
            cls.write('\033[1;33m[ERROR]\033[1;0m {0}'.format(text), True)
        elif severity == cls.FAIL and cls.VERBOSITY >= 1:
            cls.write('\033[1;31m[FAIL] {0}\033[1;0m'.format(text))
        elif severity == cls.OK and cls.VERBOSITY >= 1:
            cls.write('\033[1;32m[OK]\033[1;0m {0}'.format(text))
        elif severity == cls.DEBUG_STD and cls.VERBOSITY >= 2:
            cls.write('\033[1;90m[DEBUG]\033[1;0m {0}'.format(text))
        elif severity == cls.DEBUG_EXT and cls.VERBOSITY >= 3:
            if cls.VERBOSITY < 4 and len(text) > cls.OUTPUT_LENGTH:
                suffix = '... (Truncated to {0} bytes)\n' \
                         ''.format(cls.OUTPUT_LENGTH)
                text = text[:cls.OUTPUT_LENGTH] + suffix
            cls.write('\033[1;90m[DEBUG]\033[1;0m {0}'.format(text))
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

            totaltests = document.createElement('tests')
            totaltests.appendChild(document.createTextNode(str(info['total'])))
            testresult.appendChild(totaltests)

            failedtests = document.createElement('failures')
            failedtests.appendChild(document.createTextNode(str(info['failures'])))
            testresult.appendChild(failedtests)

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
            Logger.debug('Copying test "{0}" to remote machine\n'.format(test_name))
            self._copy_test(path)
            self._apply_setup_routines(test_name, 'setup')
            note_printed = Logger.info('Running test "{0}"...\n'.format(test_name))
            result = self._run_test(path)
            Logger.info('Test "{0}" has finished (Total tests: {1}, Failures: {2})\n'
                        ''.format(test_name, result['total'], result['failures']))
            self._apply_setup_routines(test_name, 'teardown')
            Logger.debug('Removing test "{0}" from remote machine\n'.format(test_name))
            self._remove_test(test_name)
            self._results[test_name] = result
            if note_printed:
                Logger.write('\n')

    def _apply_setup_routines(self, test_name, context):
        instructions = next((t[1].get(context)
                             for t in self._config.get('setups', {}).iteritems()
                             if re.match(t[0], test_name)), None)
        if instructions is not None:
            note_printed = Logger.info('Applying {0} routines for test "{1}" .. '
                                       ''.format(context, test_name))
            for instruction in instructions.get('copy', []):
                source, _, destination = instruction.partition('>>')
                self._copy_file(source.strip(), destination.strip())
            for filepath in instructions.get('clean', []):
                self._remove_file(filepath)
            for command in instructions.get('exec', []):
                self._exec_command(command)
            if note_printed:
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
            Logger.error('Command "{0}" exited with exit code "{1}"\n' \
                         ''.format(command, rc))

    def _copy_file(self, source, destination):
        command = self._config['commands']['copy'].format(source, destination)
        rc = subprocess.call(command, stdout=DEVNULL, shell=True)
        if rc != 0:
            Logger.error('Cannot copy file "{0}" to "{1}" ({2})\n' \
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
        options = ['--verbosity={0}'.format(Logger.VERBOSITY)]
        p = subprocess.Popen(command.format(' '.join([target] + options)),
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             shell=True)
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

            verbosity_level = line.count('\x00')
            line = line[verbosity_level:]
            if line.startswith('[ERROR] '):
                Logger.error(line[8:])
            elif line.startswith('[DEBUG] '):
                Logger.debug(line[8:], verbosity_level == 4)
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
    parser = OptionParser(version='0.5')
    parser.add_option('-C', '--config', default="run_tests.conf",
                      help='The path to the config file to use [%default]')
    parser.add_option('-R', '--results',
                      help='The file where to store the test results')
    parser.add_option('-v', '--verbose', action='count', default=1,
                      help='Be more verbose (Maximum output: -vvv)')
    parser.add_option('-q', '--quiet', action='count', default=0,
                      help='Be less verbose')
    return parser.parse_args()


def main():
    options, arguments = parse_commandline()
    suite = TestSuite(options.config)

    for path in (p for a in arguments for p in glob.glob(a)):
        suite.add_test(path)

    Logger.set_verbosity(options.verbose - options.quiet)
    suite.run()

    if options.results is not None:
        with open(options.results, 'w') as f:
            f.write(suite.get_report().encode('utf-8'))

    return 0


if __name__ == '__main__':
    sys.exit(main())

