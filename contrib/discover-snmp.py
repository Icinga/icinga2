#!/usr/bin/env python
from __future__ import print_function
import sys, getopt, hmac, hashlib, subprocess

def warning(*objs):
    print(*objs, file=sys.stderr)

opts, args = getopt.getopt(sys.argv[1:], "k:h", [ "key=", "help" ])

pkey = None

for opt, arg in opts:
    if opt == "-?" or opt == "--help":
        warning("Syntax: %s --key <key> <ipaddr> [<ipaddr>, ...]")
        sys.exit(1)
    elif opt == "-k" or opt == "--key":
        pkey = arg

if not pkey:
    warning("You must specify a key with the --key option.")
    sys.exit(1)

def ukey(ipaddr):
    return hmac.new(pkey, ipaddr, hashlib.sha256).hexdigest()[0:12]

for arg in args:
    plugins = []

    community = ukey(arg)
    warning("IP address: %s, SNMP Community: %s" % (arg, community))

    process = subprocess.Popen(["snmpwalk", "-v2c", "-c", community, arg, ".1.3.6.1.4.1.8072.1.3.2.3.1.2"], stdout=subprocess.PIPE)
    (out, err) = process.communicate()

    for line in out.split("\n"):
        oid = line.split(" ")[0]
        plugin = oid.split(".")[-1]
        if plugin == "":
            continue
        if plugin[0] == "\"":
            plugin = plugin[1:]
        if plugin[-1] == "\"":
            plugin = plugin[:-1]
        plugins.append(plugin)

    print("template Host \"snmp-extend:%s\" {" % (arg))
    print("  macros[\"community\"] = \"%s\"," % (community))
    for plugin in plugins:
        print("  services[\"%s\"] = {" % (plugin))
        print("    templates = [ \"snmp-extend-service\" ],")
        print("    check_command = \"snmp-extend\",")
        print("    macros[\"plugin\"] = \"%s\"" % (plugin))
        print("  },")
    print("}")
