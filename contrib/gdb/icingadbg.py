import gdb
import re

class IcingaStringPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return '"' + self.val['m_Data']['_M_dataplus']['_M_p'].string() + '"'

class IcingaValuePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        which = self.val['m_Value']['which_']

        if which == 0:
            return 'Empty'
        elif which == 1:
            return self.val['m_Value']['storage_']['data_']['buf'].cast(gdb.lookup_type('double').pointer()).dereference()
        elif which == 2:
            return self.val['m_Value']['storage_']['data_']['buf'].cast(gdb.lookup_type('icinga::String').pointer()).dereference()
        elif which == 3:
            return self.val['m_Value']['storage_']['data_']['buf'].cast(gdb.lookup_type('icinga::Object').pointer()).dereference()
        else:
            return '<INVALID>'

class IcingaAttributePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['m_Value']

class IcingaSignalPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return '<SIGNAL>'

def lookup_icinga_type(val):
    t = val.type.unqualified()
    if str(t) == 'icinga::String':
        return IcingaStringPrinter(val)
    elif str(t) == 'icinga::Value':
        return IcingaValuePrinter(val)
    elif str(t) == 'icinga::AttributeBase' or re.match('^icinga::Attribute<.*>$', str(t)):
        return IcingaAttributePrinter(val)
    elif re.match('^boost::signals2::signal.*<.*>$', str(t)):
        return IcingaSignalPrinter(val)

    return None

def register_icinga_printers():
    gdb.pretty_printers.append(lookup_icinga_type)
