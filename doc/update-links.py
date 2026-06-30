#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

import os
import sys
import re

if len(sys.argv) < 2:
    print "Syntax: %s <md-files>" % sys.argv[0]
    print ""
    print "Updates inter-chapter links in the specified Markdown files."
    sys.exit(1)

anchors = {}

for file in sys.argv[1:]:
    text = open(file).read()
    for match in re.finditer(r"<a id=\"(?P<id>.*?)\">", text):
        id = match.group("id")

        if id in anchors:
            print "Error: Anchor '%s' is used multiple times: in %s and %s" % (id, file, anchors[id])

        anchors[match.group("id")] = file

def update_anchor(match):
    id = match.group("id")

    try:
        file = os.path.basename(anchors[id])
    except KeyError:
        print "Error: Unmatched anchor: %s" % (id)
        file = ""

    return "[%s](%s#%s)" % (match.group("text"), file, id)

for file in sys.argv[1:]:
    text = open(file).read()
    print "> Processing file '%s'..." % (file)
    new_text = re.sub(r"\[(?P<text>.*?)\]\((?P<file>[0-9-a-z\.]+)?#(?P<id>[^#\)]+)\)", update_anchor, text)
    open(file, "w").write(new_text)
