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
            self._apply_setup_routines(test_name, 'setup')
            self._copy_test(path)
            self._results[test_name] = self._run_test(path)
            self._apply_setup_routines(test_name, 'teardown')

    def _apply_setup_routines(self, test_name, context):
        instructions = next((t[1].get(context)
                             for t in self._config.get('setups', {}).iteritems()
                             if re.match(t[0], test_name)), None)
        if instructions is not None:
            for instruction in instructions.get('copy', []):
                source, _, destination = instruction.partition('>>')
                self._copy_file(source.strip(), destination.strip())
            for filepath in instructions.get('clean', []):
                self._remove_file(filepath)
            for command in instructions.get('exec', []):
                self._exec_command(command)

    def _remove_file(self, path):
        command = self._config['commands']['clean'].format(path)
        subprocess.call(command, stdout=DEVNULL, shell=True)

    def _exec_command(self, command):
        command = self._config['commands']['exec'].format(command)
        subprocess.call(command, stdout=DEVNULL, shell=True)

    def _copy_file(self, source, destination):
        command = self._config['commands']['copy'].format(source, destination)
        subprocess.call(command, stdout=DEVNULL, shell=True)

    def _copy_test(self, path):
        self._copy_file(path, os.path.join(self._config['settings']['test_root'],
                                           os.path.basename(path)))

    def _run_test(self, path):
        command = self._config['commands']['exec']
        target = os.path.join(self._config['settings']['test_root'],
                              os.path.basename(path))
        p = subprocess.Popen(command.format(target), stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, shell=True)
        out, err = p.communicate()

        return {
            'stdout': out.decode('utf-8'),
            'stderr': err.decode('utf-8'),
            'returncode': p.returncode
            }


def parse_commandline():
    parser = OptionParser(version='0.1')
    parser.add_option('-C', '--config', default="run_tests.conf",
                      help='The path to the config file to use [%default]')
    parser.add_option('-O', '--output',
                      help='The file which to save the test results. '
                           '(By default this goes to stdout)')
    return parser.parse_args()


def main():
    options, arguments = parse_commandline()
    suite = TestSuite(options.config)

    for path in (p for a in arguments for p in glob.glob(a)):
        suite.add_test(path)

    suite.run()

    report = suite.get_report()
    if options.output is None:
        print report.encode('utf-8')
    else:
        with open(options.output, 'w') as f:
            f.write(report.encode('utf-8'))

    return 0


if __name__ == '__main__':
    sys.exit(main())

