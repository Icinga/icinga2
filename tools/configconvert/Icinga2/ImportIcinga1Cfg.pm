
=pod
/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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


package Icinga2::ImportIcinga1Cfg;

push (@INC, 'pwd');

use strict;
use Data::Dumper;
use File::Find;
use Storable qw(dclone);

use feature 'say';

#use Icinga2;
use Icinga2::Utils;


################################################################################
# PARSE 1.x
################################################################################

sub get_key_from_icinga1_main_cfg {
    my ($file, $key) = @_;

    my @key_arr = ();

    if ( !-f $file) {
        print "cfg file $file does not exist!";
        return;
    }

    if ( open ( my $fh, '<', $file ) ) {
        while ( my $line = <$fh> ) {
            chomp($line);
            $line =~ s/#.*//;
            if ($line =~ /^\s*$key=([^\s]+)/) {
                push @key_arr, $1; # we may have multiple occurences
            }
        }
    }

    return @key_arr;
}

sub parse_icinga1_resource_cfg {
    my $file = shift;

    my @cfg = Icinga2::Utils::slurp($file);

    my $global_macros = {};

    foreach my $line (@cfg) {
        $line = Icinga2::Utils::strip($line);

        # skip comments and empty lines
        next if ($line eq "" || !defined($line) || $line =~ /^\s+$/);
        next if ($line =~ /^[#;]/ || $line =~ /;.*/);

        #debug($line);
        my ($macro_name, $macro_value) = split /=/, $line, 2;
        $macro_name =~ /\$(.*)\$/;
        $macro_name = $1;

        $global_macros->{$macro_name} = Icinga2::Utils::escape_str($macro_value);
    }

    return $global_macros;

}

sub parse_icinga1_global_macros {
    my $icinga1_cfg = shift;

    my ($icinga1_resource_file) = get_key_from_icinga1_main_cfg($icinga1_cfg, "resource_file");

    # resource.cfg
    my $global_macros = parse_icinga1_resource_cfg($icinga1_resource_file);

    # special attributes in icinga.cfg (admin_*)
    my ($admin_pager) = get_key_from_icinga1_main_cfg($icinga1_cfg, "admin_pager");
    my ($admin_email) = get_key_from_icinga1_main_cfg($icinga1_cfg, "admin_email");

    $global_macros->{'ADMINPAGER'} = $admin_pager;
    $global_macros->{'ADMINEMAIL'} = $admin_email;

    return $global_macros;
}

sub parse_icinga1_object_cfg {
    my $cfg_obj = shift;
    my $file = shift;

    my $obj = {}; #hashref
    my $in_define = 0;
    my $in_timeperiod = 0;
    my $type;
    my $append; # this is a special case where multiple lines are appended with \ - not sure if we support THAT.
    my $inline_comment;

    my $attr;
    my $val;

    my @cfg = Icinga2::Utils::slurp($file);

    #Icinga2::Utils::debug("========================================================");
    #Icinga2::Utils::debug("File: $file");
    foreach my $line (@cfg) {
        $line = Icinga2::Utils::strip($line);

        #Icinga2::Utils::debug("Processing line: '$line'");

        # skip comments and empty lines
        next if ($line eq "" || !defined($line) || $line =~ /^\s+$/);
        next if ($line =~ /^[#;]/);

        # || $line =~ /;.*/);
        $line =~ s/[\r\n\s]+$//;
        $line =~ s/^\s+//;

        # end of def
        if ($line =~ /}(\s*)$/) {
            $in_define = undef;
            # store type for later
            $cfg_obj->{'type_cnt'}->{$type} = $cfg_obj->{'type_cnt'}->{$type} + 1;
            $type = "";
            next;
        }
        # start of def
        elsif ($line =~ /define\s+(\w+)\s*{?(.*)$/) {
            $type = $1;
            $append = $2;
            if ($type eq "timeperiod") {
                $in_timeperiod = 1;
            } else {
                $in_timeperiod = 0;
            }

            # save the type
            $cfg_obj->{$type}->{$cfg_obj->{'type_cnt'}->{$type}}->{'__I2CONVERT_TYPE'} = $type;

            # we're ready to process entries
            $in_define = 1;
            # save the current type counter, being our unique key here
            next;
        }
        # in def
        elsif ($in_define == 1) {

            # first, remove the annoying inline comments after ';'
            $line =~ s/\s*[;\#](.*)$//;
            $inline_comment = $1;

            # then split it and save it by type->cnt->attr->val
            #($attr, $val) = split (/\s+/, $line, 2); # important - only split into 2 elements

            # timeperiods require special parser
            if ($in_timeperiod == 1) {
                if ($line =~ /timeperiod_name/ || $line =~ /alias/ || $line =~ /exclude/) {
                    $line =~ m/([\w]+)\s*(.*)/;
                    $attr = Icinga2::Utils::strip($1); $val = Icinga2::Utils::strip($2);
                } else {
                    $line =~ m/(.*)\s+([\d\W]+)/;
                    $attr = Icinga2::Utils::strip($1); $val = Icinga2::Utils::strip($2);
                }
            } else {
                    $line =~ m/([\w]+)\s*(.*)/;
                    $attr = Icinga2::Utils::strip($1); $val = Icinga2::Utils::strip($2);
            }
            # ignore empty values
            next if (!defined($val));
            next if ($val eq "");
            #Icinga2::Utils::debug("cnt: $cfg_obj->{'type_cnt'}->{$type}");
            #Icinga2::Utils::debug("line: '$line'");
            #Icinga2::Utils::debug("type: $type");
            #Icinga2::Utils::debug("attr: $attr");
            #Icinga2::Utils::debug("val: $val");
            #Icinga2::Utils::debug("\n");

            # strip illegal object name characters, replace with _
            if ( ($attr =~ /name/ && $attr !~ /display_name/) ||
                    $attr =~ /description/ ||
                    $attr =~ /contact/ ||
                    $attr =~ /groups/ ||
                    $attr =~ /members/ ||
                    $attr =~ /use/ ||
                    $attr =~ /parents/
                ) {
                $val = Icinga2::Utils::strip_object_name($val);
            }
            # treat 'null' (disable) as '0'
            if ($val eq "null") {
                $val = 0;
            }

            $cfg_obj->{$type}->{$cfg_obj->{'type_cnt'}->{$type}}->{$attr} = $val;

            # ignore duplicated attributes, last one wins
        }
        else {
            $in_define = 0;
        }

    }

    #Icinga2::Utils::debug("========================================================");

    return $cfg_obj;

}

# the idea is to reduce work load - get all the existing object relations (host->service)
# and have core 1.x already mapped that. we focus on getting the details when
# needed, but do not print the object without templates - only if there's no other way.
sub parse_icinga1_objects_cache {
    my $icinga1_cfg = shift;

    # XXX not needed right now
    return undef;

    # functions return array in case of multiple occurences, we'll take only the first one
    my ($object_cache_file) = get_key_from_icinga1_main_cfg($icinga1_cfg, "object_cache_file");

    if(!defined($object_cache_file)) {
        print "ERROR: No objects cache file found in $icinga1_cfg! We'll need for final object conversion.\n";
        return -1;
    }

    if(! -r $object_cache_file) {
        print "ERROR: objects cache file '$object_cache_file' from $icinga1_cfg not found! We'll need it for final object conversion.\n";
        return -1;
    }

    my $cfg_obj_cache = {};

    $cfg_obj_cache = parse_icinga1_object_cfg($cfg_obj_cache, $object_cache_file);

    #say Dumper($cfg_obj_cache);

    return $cfg_obj_cache;

}

# parse all existing config object included in icinga.cfg, with all their templates
# and grouping tricks
sub parse_icinga1_objects {
    my $icinga1_cfg = shift;

    my @cfg_files = get_key_from_icinga1_main_cfg($icinga1_cfg, "cfg_file");
    my @cfg_dirs = get_key_from_icinga1_main_cfg($icinga1_cfg, "cfg_dir");

    sub find_icinga1_cfg_files {
        my $file = $File::Find::name;
        return if -d $file;
        if ($file =~ /\.cfg$/) {
            push @cfg_files, $file;
        }
    }

    foreach my $cfg_dir (@cfg_dirs) {
        find(\&find_icinga1_cfg_files, $cfg_dir);
    }

    # check if there was nothing to include
    if (!@cfg_files) {
        print "ERROR: $icinga1_cfg did not contain any object includes.\n";
        return -1;
    }
    #print "@cfg_files";

    # now fetch all the config information into our global hash ref
    my $cfg_objs = {};

    foreach my $cfg_file (@cfg_files) {
        print "Processing file '$cfg_file'...\n";
        $cfg_objs = parse_icinga1_object_cfg($cfg_objs, $cfg_file);
    }

    #say Dumper($cfg_obj);
    #say Dumper($cfg_obj->{'service'});

    return $cfg_objs;
}


1;

__END__
# vi: sw=4 ts=4 expandtab :
