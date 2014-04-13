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
import sys, os, hashlib

def warning(*objs):
    print(*objs, file=sys.stderr)

if len(sys.argv) < 2:
    warning("Syntax: %s <identity>" % (sys.argv[0]))
    sys.exit(1)

cn = sys.argv[1]

inventory_file = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/inventory/" + hashlib.sha256(cn).hexdigest()

if not os.path.isfile(inventory_file):
    warning("There's no inventory file for agent '%s'." % (cn))
    sys.exit(0)

os.unlink(inventory_file)

peer_file = "@CMAKE_INSTALL_FULL_LOCALSTATEDIR@/lib/icinga2/agent/inventory/" + hashlib.sha256(cn).hexdigest() + ".peer"

if os.path.isfile(peer_file):
    os.unlink(peer_file)

print("Inventory information has been removed for agent '%s'." % (cn))
sys.exit(0)
