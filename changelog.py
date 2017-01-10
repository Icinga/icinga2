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

import urllib2, json, sys, string, collections
from argparse import ArgumentParser

DESCRIPTION="update release changes"
VERSION="1.0.0"
ISSUE_URL= "https://dev.icinga.com/issues/"
ISSUE_PROJECT="i2"

arg_parser = ArgumentParser(description= "%s (Version: %s)" % (DESCRIPTION, VERSION))
arg_parser.add_argument('-V', '--version', required=True, type=str, help="define version to query")
arg_parser.add_argument('-p', '--project', type=str, help="add urls to issues")
arg_parser.add_argument('-l', '--links', action='store_true', help="add urls to issues")
arg_parser.add_argument('-H', '--html', action='store_true', help="print html output (defaults to markdown)")

args = arg_parser.parse_args(sys.argv[1:])

ftype = "md" if not args.html else "html"

def format_header(text, lvl, ftype = ftype):
   if ftype == "html":
       return "<h%s>%s</h%s>" % (lvl, text, lvl)
   if ftype == "md":
       return "#" * lvl + " " + text

def format_logentry(log_entry, args = args, issue_url = ISSUE_URL):
   if args.links:
       if args.html:
           return "<li> {0} <a href=\"{4}{1}\">{1}</a> ({2}): {3}</li>".format(log_entry[0], log_entry[1], log_entry[2], log_entry[3],issue_url)
       else:
           return "* {0} [{1}]({4}{1} \"{0} {1}\") ({2}): {3}".format(log_entry[0], log_entry[1], log_entry[2], log_entry[3], issue_url)
   else:
       if args.html:
           return "<li>%s %d (%s): %s</li>" % log_entry
       else:
           return "* %s %d (%s): %s" % log_entry

def print_category(category, entries):
    if len(entries) > 0:
        print ""
        print format_header(category, 4)
        print ""
        if args.html:
            print "<ul>"

        tmp_entries = collections.OrderedDict(sorted(entries.items()))

        for cat, entry_list in tmp_entries.iteritems():
            for entry in entry_list:
                print format_logentry(entry)

        if args.html:
            print "</ul>"
            print ""


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

changes = ""

if "custom_fields" in version:
    for field in version["custom_fields"]:
        if field["id"] == 14:
            changes = field["value"]
            break

    changes = string.join(string.split(changes, "\r\n"), "\n")

print format_header("What's New in Version %s" % (version_name), 3)
print ""

if changes:
    print format_header("Changes", 4)
    print ""
    print changes
    print ""

offset = 0

features = {}
bugfixes = {}
support = {}
category = ""

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
                if field["id"] == 13 and "value" in field and field["value"] == "0":
                    ignore_issue = True
                    break

            if ignore_issue:
                continue

        if "category" in issue:
            category = str(issue["category"]["name"])
        else:
            category = "no category"

        # the order is important for print_category()
        entry = (issue["tracker"]["name"], issue["id"], category, issue["subject"].strip())

	if issue["tracker"]["name"] == "Feature":
            try:
                features[category].append(entry)
            except KeyError:
                features[category] = [ entry ]
	elif issue["tracker"]["name"] == "Bug":
            try:
                bugfixes[category].append(entry)
            except KeyError:
                bugfixes[category] = [ entry ]
	elif issue["tracker"]["name"] == "Support":
            try:
                support[category].append(entry)
            except KeyError:
                support[category] = [ entry ]

print_category("Feature", features)
print_category("Bugfixes", bugfixes)
print_category("Support", support)


sys.exit(0)
