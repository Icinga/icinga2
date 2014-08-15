#!/usr/bin/python
# netstring.py - Netstring encoding/decoding routines.
# Version 1.1 - July 2003
# http://www.dlitz.net/software/python-netstring/
#
# Copyright (c) 2003 Dwayne C. Litzenberger <dlitz@dlitz.net>
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# HISTORY:
#
# Changes between 1.0 and 1.1:
# - Renamed Reader to BaseReader.  Use FileReader and StringReader instead.
# - Added BaseReader.readskip()
# - Switched to saner stream reading semantics.  Now the stream is not read
#   until information is requested which requires it to be read.
# - Added split()
#

from __future__ import generators
import StringIO

"""Conversions to/from netstring format.

The netstring format is defined in http://cr.yp.to/proto/netstrings.txt
(or http://www.dlitz.net/proto/netstrings-abnf.txt if you prefer ABNF)

Classes:

    BaseReader (not to be used directly)
    FileReader
    StringReader

Functions:

    dump
    dumps
    load
    loads
    split

Misc variables:

    maxintlen       - Maximum number of digits when reading integers

"""

__all__ = ['BaseReader', 'FileReader', 'StringReader',
           'dump', 'dumps', 'load', 'loads', 'split']

maxintlen = 999     # Maximum number of digits when reading integers
                    # This allows numbers up to 10**1000 - 1, which should
                    # be large enough for most applications. :-)


def dump(s, file):
    """dump(s, file) -> None

Writes the string s as a netstring to file.
"""
    file.write(dumps(s))


def dumps(s):
    """dumps(s) -> string

Encodes the string s as a netstring, and returns the result.
"""
    return str(len(s)) + ":" + s + ","


def load(file, maxlen=None):
    """load(file, maxlen=None) -> string

Read a netstring from a file, and return the extracted netstring.

If the parsed string would be longer than maxlen, OverflowError is raised.
"""
    n = _readlen(file)
    if maxlen is not None and n > maxlen:
        raise OverflowError
    retval = file.read(n)
    #assert(len(retval) == n)
    ch = file.read(1)
    if ch == "":
        raise EOFError
    elif ch != ",":
        raise ValueError
    return retval


def loads(s, maxlen=None, returnUnparsed=False):
    """loads(s, maxlen=None, returnUnparsed=False) -> string or (string,
    string)

Extract a netstring from a string.  If returnUnparsed is false, return the
decoded netstring, otherwise return a tuple (parsed, unparsed) containing both
the parsed string and the remaining unparsed part of s.

If the parsed string would be longer than maxlen, OverflowError is raised.
"""
    f = StringIO.StringIO(s)
    parsed = load(f, maxlen=maxlen)
    if not returnUnparsed:
        return parsed
    unparsed = f.read()
    return parsed, unparsed


def _readlen(file):
    """_readlen(file) -> integer

Read the initial "[length]:" of a netstring from file, and return the length.
"""
    i = 0
    n = ""
    ch = file.read(1)
    while ch != ":":
        if ch == "":
            raise EOFError
        elif not ch in "0123456789":
            raise ValueError
        n += ch
        i += 1
        if i > maxintlen:
            raise OverflowError
        ch = file.read(1)
    #assert(ch == ":")
    return long(n)


def split(s):
    """split(s) -> list of strings

Return a list of the decoded netstrings in s.
"""
    if s == "":
    	raise EOFError
    retval = []
    unparsed = s
    while unparsed != "":
        parsed, unparsed = loads(unparsed, returnUnparsed=True)
        retval.append(parsed)
    return retval


class BaseReader:
    """BaseReader(file, maxlen=None, blocksize=1024) -> BaseReader object

Return a new BaseReader object.  BaseReader allows reading a
netstring in blocks, instead of reading an netstring into memory at once.

If BaseReader encounters a netstring which is larger than maxlen, it will return
OverflowError.  BaseReader will also return ValueError if it encounters bad
formatting, or EOFError if an unexpected attempt is made to read beyond the
end of file.

The state of BaseReader is undefined once any exception other than StopIteration
is raised.

blocksize is the size of blocks to use when iterating over the BaseReader class.
You should not use BaseReader except when subclassing.  Use FileReader or one
of the other *Reader classes instead.
"""

    def __init__(self, file, maxlen=None, blocksize=1024):
        self._file = file
        self._length = None
        self._bytesleft = 0L
        self._maxlen = maxlen
        self._blocksize = blocksize

    def _readlen(self):
        if self._length is None:
            self._length = _readlen(self._file)
            self._bytesleft = self._length
            if self._maxlen is not None and self._length > self._maxlen:
                raise OverflowError
            # Handle the 0-byte case
            if self._length == 0:
                ch = self._file.read(1)
                if ch == "":
                    raise EOFError
                elif ch != ",":
                    raise ValueError

    def read(self, size=None):
        """x.read([size]) -> string

Works like <fileobject>.read.
"""
        self._readlen()
        if size is None or size > self._bytesleft:
            size = self._bytesleft
        if size == 0:
            return ""
        retval = self._file.read(size)
        self._bytesleft -= len(retval)
        if self._bytesleft == 0:
            ch = self._file.read(1)
            if ch == "":
                raise EOFError
            elif ch != ",":
                raise ValueError
        return retval

    def length(self):
        """x.length() -> long

Return the total length of the decoded string.
"""
        self._readlen()
        return self._length

    def bytesremaining(self):
        """x.bytesremaining() -> long

Return the number of decoded string bytes remaining to be read from the file.
"""
        self._readlen()
        return self._bytesleft

    def skip(self):
        """x.skip() -> None

Skip to the next netstring.
"""
        self._readlen()
        if self._bytesleft:
            self._file.seek(self._bytesleft, 1)
            ch = self._file.read(1)
            if ch == "":
                raise EOFError
            elif ch != ",":
                raise ValueError
        self._bytesleft = 0L
        self._length = None

    def readskip(self, size=None):
        """x.readskip([size]) -> string

Equivalent to x.read([size]); x.skip().  Returns whatever is returned by
x.read().
"""
        retval = self.read(size)
        self.skip()
        return retval

    def __iter__(self):
        """x.__iter__() -> iterator

Return a block of the decoded netstring.
"""
        block = self.read(self._blocksize)
        while block != "":
            yield block
            block = self.read(self._blocksize)

    def __len__(self):
        """x.__len__() -> integer

Return the total length of the decoded string.

Note that this is limited to the maximum integer value.  Use x.length()
wherever possible.
"""
        return int(self.length())


class FileReader(BaseReader):
    """FileReader(file, ...) -> FileReader object

Takes a file as input.  See BaseReader.__doc__ for more information.
"""
    pass


class StringReader(BaseReader):
    """StringReader(s, ...) -> StringReader object

Takes a string as input.  See BaseReader.__doc__ for more information.
"""
    def __init__(self, s, *args, **kwargs):	
        file = StringIO.StringIO(s)
        return BaseReader.__init__(self, file, *args, **kwargs)


# vim:set tw=78 sw=4 ts=4 expandtab:
