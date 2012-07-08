#!/usr/bin/env python
import sys
import subprocess
import socket
from xml.dom.minidom import parseString

if len(sys.argv) < 2:
    print "Syntax: %s <host>" % (sys.argv[0])
    sys.exit(1)

proc = subprocess.Popen(["nmap", "-oX", "-", sys.argv[1]], stdout=subprocess.PIPE)
output = proc.communicate()

dom = parseString(output[0])

hostname = sys.argv[1]

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

    print "object host \"%s\" {" % (hostname)
    print "\tmacros = {"
    print "\t\taddress = \"%s\"" % (address)
    print "\t},"
    print ""
    print "\tservices += { \"ping\" },"

    for element in host.getElementsByTagName("port"):
        port = int(element.getAttribute("portid"))
        protocol = element.getAttribute("protocol")

        try:
            serv = socket.getservbyport(port, protocol)
        except:
            serv = str(port)

        print ""
        print "\tservices[\"%s\"] = {" % (serv)
        print "\t\tservice = \"%s\"," % (protocol)
        print ""
        print "\t\tmacros = {"
        print "\t\t\tport = %s" % (port)
        print "\t\t}"
        print "\t},"

    print "}"
    print ""

for host in dom.getElementsByTagName("host"):
    processHost(host)
