#!/usr/bin/env python
# Icinga 2
# Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

import os
import sys
import re

if len(sys.argv) < 2:
    print "Syntax: %s <md-files>"
    print ""
    print "Updates intra-chapter links in the specified Markdown files."
    sys.exit(1)

anchors = {}

for file in sys.argv[1:]:
    text = open(file).read()
    for match in re.finditer(r"<a id=\"(?P<id>.*?)\">", text):
        id = match.group("id")

        if id in anchors:
            print "Anchor '%s' is used multiple times: in %s and %s" % (id, file, anchors[id])

        anchors[match.group("id")] = file

def update_anchor(match):
    id = match.group("id")

    try:
        file = os.path.basename(anchors[id])
    except KeyError:
        print "Unmatched anchor: %s" % (id)
        file = ""

    return "[%s](%s#%s)" % (match.group("text"), file, id)

for file in sys.argv[1:]:
    text = open(file).read()
    new_text = re.sub(r"\[(?P<text>.*)\]\((?P<file>[0-9-[a-z]\.]+)?#(?P<id>[^#\)]+)\)", update_anchor, text)
    open(file, "w").write(new_text)
