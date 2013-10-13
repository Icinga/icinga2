# Define: cmmi
#
# This module downloads, extracts, builds and installs tar.gz archives using
# wget, tar and the autotools stack. Build directory is always /usr/local/src.
#
# *Note* make sure to install build essentials before running cmmi.
#
# Parameters:
#   [*url*]           - fetch archive via wget from this url.
#   [*output*]        - filename to fetch the archive into.
#   [*flags*]         - configure options.
#   [*creates*]       - target directory the software will install to.
#   [*make* ]         - command to make and make install the software.
#   [*make_timeout* ] - timeout for the make command.
#
# Actions:
#
# Requires:
#
# Sample Usage:
#
#   cmmi { 'example-software':
#     url          => 'http://example-software.com/download/',
#     output       => 'example-software.tar.gz',
#     flags        => '--prefix=/opt/example-software',
#     creates      => '/opt/example-software',
#     make         => 'make && make install'
#     make_timeout => 600
#   }
#
define cmmi(
  $url,
  $output,
  $flags,
  $creates,
  $make,
  $make_timeout=300
) {

  Exec { path => '/bin:/usr/bin' }

  $cwd = '/usr/local/src'

  include wget

  exec { "download-${name}":
    cwd     => $cwd,
    command => "wget -q \"${url}\" -O ${output}",
    creates => "${cwd}/${output}",
    require => Class['wget']
  }

  $tld = inline_template('<%= File.basename(output, ".tar.gz") %>')
  $src = "${cwd}/${name}/${tld}"

  exec { "extract-${name}":
    cwd     => $cwd,
    command => "mkdir -p ${name}/${tld} && tar --no-same-owner \
                --no-same-permissions -xzf ${output} -C ${name}/${tld} \
                --strip-components 1",
    creates => $src,
    require => Exec["download-${name}"]
  }

  exec { "configure-${name}":
    cwd     => $src,
    command => "sh ./configure ${flags}",
    creates => "${src}/Makefile",
    require => Exec["extract-${name}"]
  }

  exec { "make-${name}":
    cwd     => $src,
    command => $make,
    creates => $creates,
    require => Exec["configure-${name}"],
    timeout => $make_timeout
  }
}
