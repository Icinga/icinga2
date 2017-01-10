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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.

import subprocess, json

inventory_json = subprocess.check_output(["icinga2", "agent", "list", "--batch"])
inventory = json.loads(inventory_json)

for agent, agent_info in inventory.items():
    print "object Endpoint \"%s\" {" % (agent)
    print "  host = \"%s\"" % (agent)
    print "}"
    print ""
    print "object Zone \"%s\" {" % (agent_info["zone"])
    if "parent_zone" in agent_info:
        print "  parent = \"%s\"" % (agent_info["parent_zone"])
    print "  endpoints = [ \"%s\" ]" % (agent)
    print "}"
    print ""

    print "object Host \"%s\" {" % (agent_info["zone"])
    print "  check_command = \"cluster-zone\""
    print "}"
    print ""

    print "apply Dependency \"host-zone-%s\" to Host {" % (agent_info["zone"])
    print "  parent_host_name = \"%s\"" % (agent_info["zone"])
    print "  assign where host.zone == \"%s\"" % (agent_info["zone"])
    print "}"
    print ""

    print "apply Dependency \"service-zone-%s\" to Service {" % (agent_info["zone"])
    print "  parent_host_name = \"%s\"" % (agent_info["zone"])
    print "  assign where service.zone == \"%s\"" % (agent_info["zone"])
    print "}"
    print ""

    for host, services in agent_info["repository"].items():
        if host != agent_info["zone"]:
            print "object Host \"%s\" {" % (host)
            print "  check_command = \"dummy\""
            print "  zone = \"%s\"" % (agent_info["zone"])
            print "}"
            print ""

        for service in services:
            print "object Service \"%s\" {" % (service)
            print "  check_command = \"dummy\""
            print "  host_name = \"%s\"" % (host)
            print "  zone = \"%s\"" % (agent_info["zone"])
            print "}"
            print ""

