#!/usr/bin/env python
# Icinga 2
# Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

import urllib2, json, sys, string, subprocess, re, os
from argparse import ArgumentParser
from git import Repo
from tempfile import NamedTemporaryFile

DESCRIPTION="cherry-pick commits for releases"
VERSION="1.0.0"
ISSUE_URL= "https://dev.icinga.com/issues/"
ISSUE_PROJECT="i2"

arg_parser = ArgumentParser(description= "%s (Version: %s)" % (DESCRIPTION, VERSION))
arg_parser.add_argument('-V', '--version', required=True, type=str, help="define version to query")
arg_parser.add_argument('-p', '--project', type=str, help="project")

args = arg_parser.parse_args(sys.argv[1:])

version_name = args.version

if args.project:
    ISSUE_PROJECT=args.project

rsp = urllib2.urlopen("https://dev.icinga.com/projects/%s/versions.json" % (ISSUE_PROJECT))
versions_data = json.loads(rsp.read())

version_id = None

for version in versions_data["versions"]:
    if version["name"] == version_name:
        version_id = version["id"]
        break

if version_id == None:
    print "Version '%s' not found." % (version_name)
    sys.exit(1)

offset = 0

issues = set()

while True:
    # We could filter using &cf_13=1, however this doesn't currently work because the custom field isn't set
    # for some of the older tickets:
    rsp = urllib2.urlopen("https://dev.icinga.com/projects/%s/issues.json?offset=%d&status_id=closed&fixed_version_id=%d" % (ISSUE_PROJECT, offset, version_id))
    issues_data = json.loads(rsp.read())
    issues_count = len(issues_data["issues"])
    offset = offset + issues_count

    if issues_count == 0:
        break

    for issue in issues_data["issues"]:
        ignore_issue = False

        if "custom_fields" in issue:
            for field in issue["custom_fields"]:
                if field["id"] == 12 and "value" in field and field["value"] != "Not yet backported":
                    ignore_issue = True
                    break

            if ignore_issue:
                continue

        issues.add(issue["id"])

repo = Repo(".")

repo.git.fetch()

new_branch = repo.create_head("auto-merged-" + version_name, "origin/support/" + ".".join(version_name.split(".")[:-1]))
new_branch.checkout()

commits = reversed(list(repo.iter_commits(rev="origin/master")))

tmpEditor = NamedTemporaryFile()

tmpEditor.write("#!/bin/sh\ncat > $1 <<COMMITS\n")

for commit in commits:
    ids = set([int(id) for id in re.findall('#(?P<id>\d+)', commit.message)])

    if not ids.intersection(issues):
        continue

    tmpEditor.write("pick %s\n" % commit.hexsha)

tmpEditor.write("COMMITS")
tmpEditor.flush()

os.chmod(tmpEditor.name, 0700)

env = os.environ.copy()
env["EDITOR"] = tmpEditor.name

subprocess.call(["git", "rebase", "-i", "HEAD"], env=env)

sys.exit(0)
