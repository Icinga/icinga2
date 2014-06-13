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
from datetime import datetime

repository_dir = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/api/repository/"

repository = {}

for root, dirs, files in os.walk(repository_dir):
    for file in files:
        if len(file) != 64:
            continue

        fp = open(root + file, "r")
        repository_info = json.load(fp)
        fp.close()

	if not "endpoint" in repository_info:
	    continue

        if not "seen" in repository_info:
            repository_info["seen"] = 0

        repository[repository_info["endpoint"]] = repository_info

        try:
            fp = open(root + file + ".peer", "r")
            peer_info = json.load(fp)
            fp.close()

            repository[repository_info["endpoint"]]["peer"] = peer_info
        except:
            pass

if len(sys.argv) > 1 and sys.argv[1] == "--batch":
    json.dump(repository, sys.stdout)
else:
    for agent, agent_info in repository.items():
        if "peer" in agent_info:
            peer_info = agent_info["peer"]
            peer_addr = "peer address: %s:%s" % (peer_info["agent_host"], peer_info["agent_port"])
        else:
            peer_addr = "no peer address"

        print "* %s (%s, last seen: %s)" % (agent, peer_addr, datetime.fromtimestamp(agent_info["seen"]))

        for host, services in agent_info["repository"].items():
            print "    * %s" % (host)

            for service in services:
                print "        * %s" % (service)

sys.exit(0)

