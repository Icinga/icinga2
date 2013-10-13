# Class: epel
#
#   Configure EPEL repository.
#
# Parameters:
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
#   include epel
#
class epel {

  yumrepo { 'epel':
    mirrorlist => "http://mirrors.fedoraproject.org/mirrorlist?repo=epel-6&arch=${::architecture}",
    enabled    => '0',
    gpgcheck   => '0',
    descr      => "Extra Packages for Enterprise Linux 6 - ${::architecture}"
  }
}

