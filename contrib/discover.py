#!/usr/bin/env python
# Icinga 2
# Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)
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

import sys
import subprocess
import socket
from xml.dom.minidom import parse

if len(sys.argv) < 2:
    print "Syntax: %s <xml-file> [<xml-file> ...]" % (sys.argv[0])
    sys.exit(1)

tcp_service_commands = {
  'ssh': 'ssh',
  'http': 'http_ip',
  'https': 'https_ip',
  'smtp': 'smtp',
  'ssmtp': 'ssmtp'
}

udp_service_commands = {
  'ntp': 'ntp_time',
  'snmp': 'snmp-uptime'
}

hosts = {}

def process_host(host_element):
    global hosts

    status = "down"

    for status_element in host_element.getElementsByTagName("status"):
        status = status_element.getAttribute("state")

    if status != "up":
        return

    for address_element in host_element.getElementsByTagName("address"):
        if not address_element.getAttribute("addrtype") in [ "ipv4", "ipv6" ]:
            continue

        address = address_element.getAttribute("addr")
        break

    name = address

    for hostname_element in host_element.getElementsByTagName("hostname"):
        name = hostname_element.getAttribute("name")

    try:
        services = hosts[name]["services"]
    except:
        services = {}

    for port_element in host_element.getElementsByTagName("port"):
        state = "closed"

        for state_element in port_element.getElementsByTagName("state"):
            state = state_element.getAttribute("state")

        if state != "open":
            continue

        port = int(port_element.getAttribute("portid"))
        protocol = port_element.getAttribute("protocol")

        try:
            serv = socket.getservbyport(port, protocol)
        except:
            serv = str(port)

        try:
            if protocol == "tcp":
                command = tcp_service_commands[serv]
            elif protocol == "udp":
                command = udp_service_commands[serv]
            else:
                raise "Unknown protocol."
        except:
            command = protocol

        if command == "udp":
            continue

        services[serv] = { "command": command, "port": port }

    hosts[name] = { "name": name, "address": address, "services": services }

def print_host(host):
    print "object Host \"%s\" {" % (host["name"])
    print "\timport \"discovered-host\","
    print ""
    print "\taddress = \"%s\"," % (host["address"])
    print "}"
    print ""

    for serv, service in host["services"].iteritems():
        print "object Service \"%s\" {" % (serv)
        print "\timport \"discovered-service\","
        print ""
        print "\thost_name = \"%s\"" % (host["name"])
        print "\tcheck_command = \"%s\"," % (service["command"])
        print ""
        print "\tvars.port = %s" % (service["port"])
        print "}"
        print ""

for arg in sys.argv[1:]:
    # Expects XML output from 'nmap -oX'
    dom = parse(arg)

    for host in dom.getElementsByTagName("host"):
        process_host(host)

for host in hosts.values():
    print_host(host)
