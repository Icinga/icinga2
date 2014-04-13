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

from __future__ import print_function
import sys, os, json

def warning(*objs):
    print(*objs, file=sys.stderr)

inventory_dir = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/inventory/"

inventory = {}

for root, dirs, files in os.walk(inventory_dir):
    for file in files:
        fp = open(root + file, "r")
        inventory_info = json.load(fp)
        inventory[inventory_info["identity"]] = inventory_info["crs"]["services"].keys()

json.dump(inventory, sys.stdout)
sys.exit(0)
