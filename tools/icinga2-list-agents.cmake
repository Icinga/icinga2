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

import sys, os, json

inventory_dir = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/inventory/"

inventory = {}

for root, dirs, files in os.walk(inventory_dir):
    for file in files:
        if len(file) != 64:
            continue

        fp = open(root + file, "r")
        inventory_info = json.load(fp)
        fp.close()

        inventory[inventory_info["identity"]] = {}
        inventory[inventory_info["identity"]]["services"] = inventory_info["crs"]["services"].keys()

        try:
            fp = open(root + file + ".peer", "r")
            peer_info = json.load(fp)
            fp.close()

            inventory[inventory_info["identity"]]["peer"] = peer_info
        except:
            pass

if len(sys.argv) > 1 and sys.argv[1] == "--batch":
    json.dump(inventory, sys.stdout)
else:
    for host, host_info in inventory.items():
        if "peer" in host_info:
            peer_info = host_info["peer"]
            peer_addr = " (%s:%s)" % (peer_info["agent_host"], peer_info["agent_port"])
        else:
            peer_addr = ""

        print "* %s%s" % (host, peer_addr)

        for service in host_info["services"]:
            print "    * %s" % (service)

sys.exit(0)
