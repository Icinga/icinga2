#!/usr/bin/env python
import os, re
from setuptools import setup, find_packages

def get_icinga2_version():
    spec = open(os.path.join('@PROJECT_SOURCE_DIR@', 'icinga2.spec')).read()
    m = re.search('^Version: (.*)$', spec, re.MULTILINE)
    if not m:
        return None
    return m.group(1)

setup(
    name = 'icinga2',
    version = get_icinga2_version(),
    packages = find_packages(),
    entry_points = {
        'console_scripts': [ 'icinga2-list-objects=icinga2.commands.list_objects:main' ]
    }
)

