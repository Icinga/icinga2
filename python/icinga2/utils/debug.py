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
import codecs

try:
    import json
except ImportError:
    import simplejson as json

def trydecode(inc_string):
    try:
        return codecs.decode(inc_string, 'utf8')
    except UnicodeDecodeError:
        try:
            return inc_string.decode('Windows-1252')
        except UnicodeDecodeError:
            return inc_string.decode('ISO 8859-1')

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
            result = u"Template '"
        else:
            result = u"Object '"
        result += self._obj["properties"]["__name"] + u"' of type '" + self._obj["type"] + u"':\n"
        result += self.format_properties(use_colors, 2)
        return result

    @staticmethod
    def format_value(value):
        if isinstance(value, list):
            result = u""
            for avalue in value:
                if result != u"":
                    result += u", "
                result += DebugObject.format_value(avalue)
            return u"[%s]" % result
        elif isinstance(value, basestring):
            return u"'%s'" % value
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
            color_begin = u''
            color_end = u''

        result = u""
        for key, value in props.items():
            path.append(key)
            result += u' ' * indent + u"* %s%s%s" % (color_begin, key, color_end)
            hints = self.format_hints(use_colors, indent + 2, path)
            if isinstance(value, dict):
                result += u"\n" + hints
                result += self.format_properties(use_colors, indent + 2, path)
            else:
                result += u" = %s\n" % (DebugObject.format_value(value))
                result += hints
            path.pop()
        return result

    def format_hints(self, use_colors, indent=0, path=[]):
        dhints = self._obj["debug_hints"]
        try:
            for component in path:
                dhints = dhints["properties"][component]
        except KeyError:
            return u""

        if use_colors:
            color_begin = _colors.CYAN
            color_end = _colors.RESET
        else:
            color_begin = u''
            color_end = u''

        result = u""
        for message in dhints["messages"]:
            result += u' ' * indent + u"%% %smodified in %s, lines %s:%s-%s:%s%s\n" % (color_begin,
              message[1], message[2], message[3], message[4], message[5], color_end)
        return result

class ObjectsFileReader(object):
    def __init__(self, file):
        self._file = file

    def __iter__(self):
        fr = netstring.FileReader(self._file)

        while True:
            try:
                json_data = trydecode((fr.readskip()))
            except EOFError:
                break
            if json_data == u"":
                break
            yield DebugObject(json.loads(json_data))
