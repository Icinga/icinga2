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
import socket, ssl, sys, json, os, hashlib, time

def warning(*objs):
    print(*objs, file=sys.stderr)

if len(sys.argv) < 2:
    warning("Syntax: %s <host> [<port>]" % (sys.argv[0]))
    sys.exit(1)

host = sys.argv[1]
if len(sys.argv) > 2:
    port = int(sys.argv[2])
else:
    port = 5665

agentpki = "@CMAKE_INSTALL_FULL_SYSCONFDIR@/icinga2/pki/agent"
keyfile = agentpki + "/agent.key"
certfile = agentpki + "/agent.crt"
cafile = agentpki + "/ca.crt"

if not os.path.isfile(certfile):
    warning("Certificate file (" + certfile + ") not found.")
    warning("Make sure the agent certificates are set up properly.")
    sys.exit(1)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# require a certificate from the server
ssl_sock = ssl.wrap_socket(s,
                           keyfile=keyfile,
                           certfile=certfile,
                           ca_certs=cafile,
                           cert_reqs=ssl.CERT_REQUIRED)

ssl_sock.connect((host, port))

cn = None

subject = ssl_sock.getpeercert()["subject"]

for prdn in subject:
    rdn = prdn[0]
    if rdn[0] == "commonName":
        cn = rdn[1]

if cn == None:
    warning("Agent certificate does not have a commonName:", repr(subject))
    sys.exit(1)

ssl_sock.close()

repository_file = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/api/repository/" + hashlib.sha256(cn).hexdigest()
fp = open(repository_file, "w")
repository_info = { "endpoint": cn, "seen": time.time(), "zone": cn, "repository": {} }
json.dump(repository_info, fp)
fp.close()

peer_file = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/repository/" + hashlib.sha256(cn).hexdigest() + ".peer"
fp = open(peer_file, "w")
peer_info = { "agent_host": host, "agent_port": port }
json.dump(peer_info, fp)
fp.close()

print("Inventory information has been updated for agent '%s'." % (cn))
sys.exit(0)
