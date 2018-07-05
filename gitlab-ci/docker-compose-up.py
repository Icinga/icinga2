#!/usr/bin/env python

import json
import os
from datetime import datetime
from subprocess import PIPE, Popen, STDOUT


p = Popen(['docker-compose', 'up', '-d', 'icinga2'], stdin=PIPE, stdout=PIPE, stderr=STDOUT)
p.stdin.close()
log = p.stdout.read()
status = p.wait()

print json.dumps({
    'type': 'gitlab-ci',
    'timestamp': datetime.utcnow().isoformat() + 'Z',
    'commit': os.environ['CI_COMMIT_SHA'],
    'commit_author': os.environ['CI_COMMIT_AUTHOR'],
    'commit_title': os.environ['CI_COMMIT_TITLE'],
    'deploy_log': log,
    'deploy_status': status
})
