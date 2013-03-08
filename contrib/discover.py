#!/usr/bin/env python
import sys
import subprocess
import socket
from xml.dom.minidom import parse

if len(sys.argv) < 2:
    print "Syntax: %s <xml-file> [<xml-file> ...]" % (sys.argv[0])
    sys.exit(1)

tcp_service_templates = {
  'ssh': 'ssh',
  'http': 'http_ip',
  'https': 'https_ip',
  'smtp': 'smtp',
  'ssmtp': 'ssmtp'
}

udp_service_templates = {
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
                template = tcp_service_templates[serv]
            elif protocol == "udp":
                template = udp_service_templates[serv]
            else:
                raise "Unknown protocol."
        except:
            template = protocol

        if template == "udp":
            continue

        services[serv] = { "template": template, "port": port }

    hosts[name] = { "name": name, "address": address, "services": services }

def print_host(host):
    print "object Host \"%s\" inherits \"discovered-host\" {" % (host["name"])
    print "\tmacros = {"
    print "\t\taddress = \"%s\"" % (host["address"])
    print "\t},"

    for serv, service in host["services"].iteritems():
        print ""
        print "\tservices[\"%s\"] = {" % (serv)
        print "\t\ttemplates = { \"%s\" }," % (service["template"])
        print ""
        print "\t\tmacros = {"
        print "\t\t\tport = %s" % (service["port"])
        print "\t\t}"
        print "\t},"

    print "}"
    print ""


print "#include <itl/itl.conf>"
print ""

for arg in sys.argv[1:]:
    # Expects XML output from 'nmap -oX'
    dom = parse(arg)

    for host in dom.getElementsByTagName("host"):
        process_host(host)

for host in hosts.values():
    print_host(host)
