# Class: wget
#
#   This class installs wget.
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
#   include wget
#
class wget {
  package { 'wget':
    ensure => installed,
  }
}
