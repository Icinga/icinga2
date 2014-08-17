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

from icinga2.utils import netstring
import subprocess

try:
    import json
except ImportError:
    import simplejson as json

class ConsoleColors(object):
    @staticmethod
    def _exec(args):
        return subprocess.Popen(args, stdout=subprocess.PIPE).communicate()[0]

    def __init__(self):
        self.RESET = ConsoleColors._exec(['tput', 'sgr0'])
        self.GREEN = ConsoleColors._exec(['tput', 'setaf', '2'])
        self.CYAN = ConsoleColors._exec(['tput', 'setaf', '6'])

_colors = ConsoleColors()

class DebugObject(object):
    def __init__(self, obj):
        self._obj = obj

    def format(self, use_colors=False):
        if self._obj["abstract"]:
            result = "Template '"
        else:
            result = "Object '"
        result += self._obj["properties"]["__name"] + "' of type '" + self._obj["type"] + "':\n"
        result += self.format_properties(use_colors, 2)
        return result

    @staticmethod
    def format_value(value):
        if isinstance(value, list):
            result = ""
            for avalue in value:
                if result != "":
                    result += ", "
                result += DebugObject.format_value(avalue)
            return "[%s]" % (result)
        elif isinstance(value, basestring):
            return "'%s'" % (str(value))
        else:
            return str(value)

    def format_properties(self, use_colors=False, indent=0, path=[]):
        props = self._obj["properties"]
        for component in path:
            props = props[component]

        if use_colors:
            color_begin = _colors.GREEN
            color_end = _colors.RESET
        else:
            color_begin = ''
            color_end = ''

        result = ""
        for key, value in props.items():
            path.append(key)
            result += ' ' * indent + "* %s%s%s" % (color_begin, key, color_end)
            hints = self.format_hints(use_colors, indent + 2, path)
            if isinstance(value, dict):
                result += "\n" + hints
                result += self.format_properties(use_colors, indent + 2, path)
            else:
                result += " = %s\n" % (DebugObject.format_value(value))
                result += hints
            path.pop()
        return result

    def format_hints(self, use_colors, indent=0, path=[]):
        dhints = self._obj["debug_hints"]
        try:
            for component in path:
                dhints = dhints["properties"][component]
        except KeyError:
            return ""

        if use_colors:
            color_begin = _colors.CYAN
            color_end = _colors.RESET
        else:
            color_begin = ''
            color_end = ''

        result = ""
        for message in dhints["messages"]:
            result += ' ' * indent + "%% %smodified in %s, lines %s:%s-%s:%s%s\n" % (color_begin,
              message[1], message[2], message[3], message[4], message[5], color_end)
        return result

class ObjectsFileReader(object):
    def __init__(self, file):
        self._file = file

    def __iter__(self):
        fr = netstring.FileReader(self._file)

        while True:
            try:
                json_data = fr.readskip()
            except EOFError:
                break
            if json_data == "":
                break
            yield DebugObject(json.loads(json_data))
