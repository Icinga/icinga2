#!/usr/bin/env python
# Copyright (c) 2014 Yusuke Shinyama
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

from __future__ import print_function

##  NetstringParser
##
class NetstringParser(object):
    """
    Decodes a netstring to a list of Python strings.

    >>> parser = NetstringParser()
    >>> parser.feed('3:456,')
    >>> parser.results
    ['456']
    >>> NetstringParser.parse('3:abc,4:defg,')
    ['abc', 'defg']
    """
    
    def __init__(self):
        self.results = []
        self.reset()
        return

    def reset(self):
        self._data = ''
        self._length = 0
        self._parse = self._parse_len
        return
        
    def feed(self, s):
        i = 0
        while i < len(s):
            i = self._parse(s, i)
        return
        
    def _parse_len(self, s, i):
        while i < len(s):
            c = s[i]
            if c < '0' or '9' < c:
                self._parse = self._parse_sep
                break
            self._length *= 10
            self._length += ord(c)-48
            i += 1
        return i
        
    def _parse_sep(self, s, i):
        if s[i] != ':': raise SyntaxError(i)
        self._parse = self._parse_data
        return i+1
        
    def _parse_data(self, s, i):
        n = min(self._length, len(s)-i)
        self._data += s[i:i+n]
        self._length -= n
        if self._length == 0:
            self._parse = self._parse_end
        return i+n
        
    def _parse_end(self, s, i):
        if s[i] != ',': raise SyntaxError(i)
        self.add_data(self._data)
        self.reset()
        return i+1

    def add_data(self, data):
        self.results.append(data)
        return

    @classmethod
    def parse(klass, s):
        self = klass()
        self.feed(s)
        return self.results

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

import socket, ssl, sys, json, os, hashlib

def warning(*objs):
    print(*objs, file=sys.stderr)

if len(sys.argv) < 2:
    warning("Syntax: %s <host> [<port>]" % (sys.argv[0]))
    sys.exit(1)

host = sys.argv[1]
if len(sys.argv) > 2:
    port = int(sys.argv[2])
else:
    port = 8483

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

ssl_sock.write('20:{"method":"get_crs"},')

nsp = NetstringParser()
while True:
    data = ssl_sock.read()
    if not data:
        break
    nsp.feed(data)

ssl_sock.close()

if len(nsp.results) != 1:
    warning("Agent returned invalid response: ", repr(nsp.results))
    sys.exit(1)

response = json.loads(nsp.results[0])
method = response['method']

if method != "push_crs":
    warning("Agent did not return any check results. Make sure you're using the master certificate.")
    sys.exit(1)

params = response['params']

inventory_file = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/inventory/" + hashlib.sha256(cn).hexdigest()
fp = open(inventory_file, "w")
inventory_info = { "identity": cn, "crs": params }
json.dump(inventory_info, fp)
fp.close()

peer_file = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/inventory/" + hashlib.sha256(cn).hexdigest() + ".peer"
fp = open(peer_file, "w")
peer_info = { "agent_host": host, "agent_port": port }
json.dump(peer_info, fp)
fp.close()

print("Inventory information has been updated for agent '%s'." % (cn))
sys.exit(0)
