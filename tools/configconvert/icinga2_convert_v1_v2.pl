#!/usr/bin/perl

=pod
/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/
=cut

=head1 NAME

icinga2_convert_v1_v2.pl - convert icinga 1.x config to icinga 2.x format

=head1 SYNOPSIS

icinga2_convert_v1_v2.pl -c <path to icinga.cfg>
                         -o <output directory for icinga2 config>
                         [-v]
                         [-h]
                         [-V]

Convert Icinga 1.x configuration to new Icinga 2.x configuration format.

=head1 OPTIONS

=over

=item -c|--icingacfgfile <path to icinga.cfg>

Path to your Icinga 1.x main configuration file "icinga.cfg".

=item -o|--outputcfgdir <output directory for icinga2 config>

Directory to Icinga 2.x configuration output.

=item --hosttmpl=<template name>

Custom template name for all host objects.

=item --servicetmpl=<template name>

Custom template name for all service objects.

=item --usertmpl=<template name>

Custom template name for all user objects.

=item --timeperiodtmpl=<template name>

Custom template name for all timeperiod objects.

=item --notificationtmpl=<template name>

Custom template name for all notification objects.

=item --checkcommandtmpl=<template name>

Custom template name for all checkcommand objects.

=item --notificationcommandtmpl=<template name>

Custom template name for all notificationcommand objects.

=item --eventcommandtmpl=<template name>

Custom template name for all eventcommand objects.

=item -v|--verbose

Verbose mode.

=item -h|--help

Print help page.

=item -V|--version

print version.

=cut

use warnings;
use strict;

use Data::Dumper;
use File::Find;
use Storable qw(dclone);
use Getopt::Long qw(:config no_ignore_case bundling);
use Pod::Usage;

use feature 'say';

push @INC, 'pwd';
use Icinga2::ImportIcinga1Cfg;
use Icinga2::ExportIcinga2Cfg;
use Icinga2::Convert;
use Icinga2::Utils;

my $version = "0.0.1";

# get command-line parameters
our $opt;
GetOptions(
    "c|icingacfgfile=s"         => \$opt->{icinga1xcfg},
    "o|outputcfgdir=s"          => \$opt->{icinga2xoutputprefix},
    "hosttmpl=s"                => \$opt->{hosttmpl},
    "servicetmpl=s"             => \$opt->{servicetmpl},
    "usertmpl=s"                => \$opt->{usertmpl},
    "notificationtmpl=s"        => \$opt->{notificationtmpl},
    "timeperiodtmpl=s"          => \$opt->{timeperiodtmpl},
    "checkcommandtmpl=s"        => \$opt->{checkcommandtmpl},
    "notificationcommandtmpl=s" => \$opt->{notificationcommandtmpl},
    "eventcommandtmpl=s"        => \$opt->{eventcommandtmpl},
    "tmpl=s"                    => \$opt->{tmpl},
    "tmpl=s"                    => \$opt->{tmpl},
    "v|verbose"                 => \$opt->{verbose},
    "h|help"                    => \$opt->{help},
    "V|version"                 => \$opt->{version}
);

my $icinga1_cfg;
my $icinga2_cfg = {};
my $conf_prefix = "./conf";
my $verbose = 0;
our $dbg_lvl = 0;
$icinga2_cfg->{'__I2EXPORT_DEBUG'} = 0;

if(defined($opt->{icinga1xcfg})) {
    $icinga1_cfg = $opt->{icinga1xcfg};
}
if(defined($opt->{icinga2xoutputprefix})) {
    $conf_prefix = $opt->{icinga2xoutputprefix};
}
if(defined($opt->{verbose})) {
    $verbose = $opt->{verbose};
    $icinga2_cfg->{'__I2EXPORT_DEBUG'} = 1;
    $dbg_lvl = 1;
}

if (defined $opt->{version}) { print $version."\n"; exit 0; }
if ($opt->{help}) { pod2usage(1); }

$icinga2_cfg->{'main'}= "$conf_prefix/icinga2.conf";
$icinga2_cfg->{'resource'}= "$conf_prefix/resource.conf";
$icinga2_cfg->{'hosts'}= "$conf_prefix/hosts.conf";
$icinga2_cfg->{'services'}= "$conf_prefix/services.conf";
$icinga2_cfg->{'users'}= "$conf_prefix/users.conf";
$icinga2_cfg->{'groups'}= "$conf_prefix/groups.conf";
$icinga2_cfg->{'notifications'}= "$conf_prefix/notifications.conf";
$icinga2_cfg->{'timeperiods'}= "$conf_prefix/timeperiods.conf";
$icinga2_cfg->{'commands'}= "$conf_prefix/commands.conf";

$icinga2_cfg->{'itl'}->{'host-template'} = "";
$icinga2_cfg->{'itl'}->{'service-template'} = "";
$icinga2_cfg->{'itl'}->{'user-template'} = "";
$icinga2_cfg->{'itl'}->{'notification-template'} = "";
$icinga2_cfg->{'itl'}->{'timeperiod-template'} = "legacy-timeperiod";
$icinga2_cfg->{'itl'}->{'checkcommand-template'} = "plugin-check-command";
$icinga2_cfg->{'itl'}->{'notificationcommand-template'} = "plugin-notification-command";
$icinga2_cfg->{'itl'}->{'eventcommand-template'} = "plugin-event-command";

if (defined($opt->{hosttmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'host-template'} = $opt->{hosttmpl};
    print "Custom host template: '" . $icinga2_cfg->{'customtmpl'}->{'host-template'} . "'\n";
}
if (defined($opt->{servicetmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'service-template'} = $opt->{servicetmpl};
    print "Custom service template: '" . $icinga2_cfg->{'customtmpl'}->{'service-template'} . "'\n";
}
if (defined($opt->{usertmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'user-template'} = $opt->{usertmpl};
    print "Custom user template: '" . $icinga2_cfg->{'customtmpl'}->{'user-template'} . "'\n";
}
if (defined($opt->{notificationtmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'notification-template'} = $opt->{notificationtmpl};
    print "Custom notification template: '" . $icinga2_cfg->{'customtmpl'}->{'notification-template'} . "'\n";
}
if (defined($opt->{timeperiodtmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'timeperiod-template'} = $opt->{timeperiodtmpl};
    print "Custom timeperiod template: '" . $icinga2_cfg->{'customtmpl'}->{'timeperiod-template'} . "'\n";
}
if (defined($opt->{checkcommandtmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'checkcommand-template'} = $opt->{checkcommandtmpl};
    print "Custom checkcommand template: '" . $icinga2_cfg->{'customtmpl'}->{'checkcommand-template'} . "'\n";
}
if (defined($opt->{notificationcommandtmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'notificationcommand-template'} = $opt->{notificationcommandtmpl};
    print "Custom notificationcommand template: '" . $icinga2_cfg->{'customtmpl'}->{'notificationcommand-template'} . "'\n";
}
if (defined($opt->{eventcommandtmpl})) {
    $icinga2_cfg->{'customtmpl'}->{'eventcommand-template'} = $opt->{eventcommandtmpl};
    print "Custom eventcommand template: '" . $icinga2_cfg->{'customtmpl'}->{'eventcommand-template'} . "'\n";
}



my $type_cnt;

################################################################################
# MAIN
################################################################################

# TODO import/export files in parallel?

# verify that we have something to read
if ( !-f $icinga1_cfg) {
    pod2usage(1);
}

# the import
my $icinga1_cfg_obj = Icinga2::ImportIcinga1Cfg::parse_icinga1_objects($icinga1_cfg);
my $icinga1_cfg_obj_cache = Icinga2::ImportIcinga1Cfg::parse_icinga1_objects_cache($icinga1_cfg);
my $icinga1_global_macros = Icinga2::ImportIcinga1Cfg::parse_icinga1_global_macros($icinga1_cfg);

# the conversion magic inside
my $icinga2_cfg_obj = Icinga2::Convert::convert_2x($icinga2_cfg, $icinga1_cfg_obj, $icinga1_cfg_obj_cache, $icinga1_global_macros);

# the export
Icinga2::ExportIcinga2Cfg::dump_cfg_resource_2x($icinga2_cfg, $icinga1_global_macros);
Icinga2::ExportIcinga2Cfg::dump_cfg_obj_2x($icinga2_cfg, $icinga2_cfg_obj);

# print stats
print "######################################################################\n";
print "# CONVERSION STATISTICS\n";
print "######################################################################\n";

Icinga2::Utils::print_sorted_hash($icinga2_cfg_obj->{'__I2_CONVERT_STATS'});

# vi: sw=4 ts=4 expandtab :
