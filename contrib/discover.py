#!/usr/bin/env python
import sys
import subprocess
import socket
from xml.dom.minidom import parseString

service_templates = {
  'ssh': 'ssh',
  'http': 'http_ip',
  'https': 'https_ip',
  'smtp': 'smtp',
  'ssmtp': 'ssmtp'
}

# Expects XML output from 'nmap -oX'
dom = parseString(sys.stdin.read())

def processHost(host):
    for element in host.getElementsByTagName("status"):
        if element.getAttribute("state") != "up":
            return

    for element in host.getElementsByTagName("address"):
        if not element.getAttribute("addrtype") in [ "ipv4", "ipv6" ]:
            continue
        
        address = element.getAttribute("addr")
        break

    hostname = address

    for element in host.getElementsByTagName("hostname"):
        hostname = element.getAttribute("name")

    print "object Host \"%s\" inherits \"itl-host\" {" % (hostname)
    print "\tmacros = {"
    print "\t\taddress = \"%s\"" % (address)
    print "\t},"

    for element in host.getElementsByTagName("port"):
        port = int(element.getAttribute("portid"))
        protocol = element.getAttribute("protocol")

        try:
            serv = socket.getservbyport(port, protocol)
        except:
            serv = str(port)

        try:
            template = service_templates[serv]
        except:
            template = protocol

        print ""
        print "\tservices[\"%s\"] = {" % (serv)
        print "\t\ttemplates = { \"%s\" }," % (template)
        print ""
        print "\t\tmacros = {"
        print "\t\t\tport = %s" % (port)
        print "\t\t}"
        print "\t},"

    print "}"
    print ""

print "#include <itl/itl.conf>"

for host in dom.getElementsByTagName("host"):
    processHost(host)
