#!/usr/bin/env python
from __future__ import print_function
import sys, getopt, hmac, hashlib, subprocess

def warning(*objs):
    print(*objs, file=sys.stderr)

opts, args = getopt.getopt(sys.argv[1:], "k:h", [ "key=", "help" ])

pkey = None

for opt, arg in opts:
    if opt == "-?" or opt == "--help":
        warning("Syntax: %s --key <key> <ipaddr>")
        sys.exit(1)
    elif opt == "-k" or opt == "--key":
        pkey = arg

if not pkey:
    warning("You must specify a key with the --key option.")
    sys.exit(1)

if len(args) != 1:
    warning("Please specify exactly one IP address.")
    sys.exit(1)

ipaddr = args[0]

def ukey(ipaddr):
    return hmac.new(pkey, ipaddr, hashlib.sha256).hexdigest()[0:12]

plugins = []

community = ukey(ipaddr)
warning("IP address: %s, SNMP Community: %s" % (ipaddr, community))

process = subprocess.Popen(["snmpwalk", "-v2c", "-c", community, "-On", ipaddr, ".1.3.6.1.4.1.8072.1.3.2.3.1.2"], stdout=subprocess.PIPE)
(out, err) = process.communicate()

if process.returncode != 0:
    sys.exit(1)

for line in out.split("\n"):
    oid = line.split(" ")[0]
    plugin = oid.split(".")[15:]
    if len(plugin) == 0:
        continue
    plugin = "".join([chr(int(ch)) for ch in plugin])
    plugins.append(plugin)

for plugin in plugins:
    print("apply Service \"%s\" {" % (plugin))
    print("  import \"snmp-extend-service\",")
    print()
    print("  check_command = \"snmp-extend\",")
    print("  macros.community = \"%s\"," % (community))
    print("  macros.plugin = \"%s\"," % (plugin))
    print()
    print("  assign where host.macros.address == \"%s\"" % (ipaddr))
    print("}")
    print()

sys.exit(0)
