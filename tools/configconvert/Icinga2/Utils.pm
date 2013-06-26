
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


package Icinga2::Utils;

use strict;
#use Icinga2;

our $dbg_lvl = 1;

################################################################################
# HELPER FUNCTIONS
################################################################################

sub strip {
    my $str = shift;

    #strip trailing and leading whitespaces
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;

    return $str;
}

sub errlog {
    my $err_lvl = shift;
    my $log_str = shift;

    if ($err_lvl > 0) {
        print STDERR color("red"), "$log_str\n";;
    } else {
        print "$log_str\n";
    }

}

sub escape_str {
    my $str = shift;

    $str =~ s/"/\\"/g;
    $str =~ s/\\\\"/\\"/g;

    return $str;
}
sub escape_shell_meta {
    my $str = shift;

    $str =~ s/([;<>`'":&!#\$\[\]\{\}\(\)\*\|])/\\$1/g;
    return $str;
}

sub debug {
    my $dbg_str = shift;
    our $dbg_lvl;

    if ($dbg_lvl > 0) {
        print "$dbg_str\n";
    }
}

sub slurp {
    my $file = shift;

    if ( -f $file ) {
        open ( my $fh, "<", $file ) or die "Could not open $file: $!";
        return do {
            <$fh>;
        }
    } elsif (! -r $file) {
        die "$file not readable. check permissions/user!"
    } else {
        die "$file does not exist";
    }
}

# stolen from http://stackoverflow.com/questions/7651/how-do-i-remove-duplicate-items-from-an-array-in-perl
sub uniq {
    return keys %{{ map { $_ => 1 } @_ }};
}

sub str2arr_by_delim_without_excludes {
    my $str = shift;
    my $delim = shift;
    my $sort = shift;
    my $exclude = shift;
    my @arr = ();

    @arr = map { s/^\s+//; s/\s+$//; $_ }
            grep { !/^!/ }
            split (/$delim/, $str);

    if ($sort == 1) {
        @arr = sort (@arr);
    }

    return @arr;
}

sub strip_object_name {
    my $obj_str = shift;

    #$obj_str =~ s/[`~!\\\$%\^&\*|'"<>\?,\(\)=:]/_/g;
    $obj_str =~ s/[:]/_/g;

    return $obj_str;
}

1;

__END__
# vi: sw=4 ts=4 expandtab :
