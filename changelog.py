#!/usr/bin/env python
#/******************************************************************************
# * Icinga 2                                                                   *
# * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
# *                                                                            *
# * This program is free software; you can redistribute it and/or              *
# * modify it under the terms of the GNU General Public License                *
# * as published by the Free Software Foundation; either version 2             *
# * of the License, or (at your option) any later version.                     *
# *                                                                            *
# * This program is distributed in the hope that it will be useful,            *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
# * GNU General Public License for more details.                               *
# *                                                                            *
# * You should have received a copy of the GNU General Public License          *
# * along with this program; if not, write to the Free Software Foundation     *
# * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
# ******************************************************************************/

import urllib2, json, sys

if len(sys.argv) < 2:
    print "Usage:", sys.argv[0], "<VERSION>"
    sys.exit(0)

version_name = sys.argv[1]

rsp = urllib2.urlopen("https://dev.icinga.org/projects/i2/versions.json")
versions_data = json.loads(rsp.read())

version_id = None

for version in versions_data["versions"]:
    if version["name"] == version_name:
        version_id = version["id"]
        break

if version_id == None:
    print "Version '%s' not found." % (version_name)
    sys.exit(1)

changes = ""

for field in version["custom_fields"]:
    if field["id"] == 14:
        changes = field["value"]
        break

print "### What's New in Version %s" % (version_name)
print ""
print "#### Changes"
print ""
print changes
print ""
print "#### Issues"
print ""

offset = 0

log_entries = []

while True:
    # We could filter using &cf_13=1, however this doesn't currently work because the custom field isn't set
    # for some of the older tickets:
    rsp = urllib2.urlopen("https://dev.icinga.org/projects/i2/issues.json?offset=%d&status_id=closed&fixed_version_id=%d" % (offset, version_id))
    issues_data = json.loads(rsp.read())
    issues_count = len(issues_data["issues"])
    offset = offset + issues_count

    if issues_count == 0:
        break

    for issue in issues_data["issues"]:
        ignore_issue = False

        for field in issue["custom_fields"]:
            if field["id"] == 13 and "value" in field and field["value"] == "0":
                ignore_issue = True
                break

        if ignore_issue:
            continue

        log_entries.append((issue["tracker"]["name"], issue["id"], issue["subject"]))

for p in range(2):
    not_empty = False

    for log_entry in sorted(log_entries):
        if (p == 0 and log_entry[0] == "Feature") or (p == 1 and log_entry[0] != "Feature"):
            print "* %s %d: %s" % log_entry
            not_empty = True

    if not_empty:
        print ""

sys.exit(0)
