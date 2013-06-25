
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


package Icinga2::Convert;

use strict;
#use Icinga2;
use Data::Dumper;
use File::Find;
use Storable qw(dclone);

use feature 'say';
our $dbg_lvl = 1;

################################################################################
## Validation
#################################################################################

sub obj_1x_is_template {
    my $obj_1x = shift;

    if (defined($obj_1x->{'register'})) {
        if ($obj_1x->{'register'} == 0) {
            return 1;
        }
    }

    return 0;
}

sub obj_1x_uses_template {
    my $obj_1x = shift;

    if (defined($obj_1x->{'use'})) {
        return 1;
    }

    return 0;
}

# check if notification object exists (2.x only)
sub obj_2x_notification_exists {
    my $objs = shift;
    my $obj_type = 'notification';
    my $obj_attr = '__I2CONVERT_NOTIFICATION_OBJECT_NAME'; # this must be set outside, no matter if template or not XXX
    my $obj_val = shift;

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");
        if ($obj->{$obj_attr} eq $obj_val) {
            #debug("Found object: ".Dumper($obj));
            return 1;
        }
    }

    return 0;
}

# check if command object exists (2.x only)
sub obj_2x_command_exists {
    my $objs = shift;
    my $obj_type = 'command';
    my $obj_attr = '__I2CONVERT_COMMAND_NAME';
    my $obj_val = shift;

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");
        if ($obj->{$obj_attr} eq $obj_val) {
            #debug("Found object: ".Dumper($obj));
            return 1;
        }
    }

    return 0;
}


################################################################################
# Migration 
#################################################################################


#################################################################################
# Get Object Helpers 
#################################################################################

# get host object by attr 'host_name'
sub obj_get_host_obj_by_host_name {
    my $objs = shift;
    my $obj_type = 'host';
    my $obj_attr = 'host_name';
    my $obj_val = shift;

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");
        if ($obj->{$obj_attr} eq $obj_val) {
            #debug("Found object: ".Dumper($obj));
            return $obj;
        }
    }

    return undef;
}

# get service object by attr 'host_name' and 'service_description'
sub obj_get_service_obj_by_host_name_service_description {
    my $objs = shift;
    my $obj_type = 'service';
    my $obj_attr_host = shift;
    my $obj_attr_service = shift;
    my $obj_val_host = shift;
    my $obj_val_service = shift;

    #debug("My objects hive: ".Dumper($objs));

    #Icinga2::Utils::debug("Checking for service with host_name=$obj_val_host service_description=$obj_val_service ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr_host});
        next if !defined($obj->{$obj_attr_service});
        #Icinga2::Utils::debug("Getting attr $obj_attr_host/$obj_attr_service and val $obj_val_host/$obj_val_service");
        if (($obj->{$obj_attr_host} eq $obj_val_host) && ($obj->{$obj_attr_service} eq $obj_val_service)) {
            #Icinga2::Utils::debug("Found object: ".Dumper($obj));
            return $obj;
        }
    }
   
    return undef;
}

# get contact object by attr 'contact_name'
sub obj_get_contact_obj_by_contact_name {
    my $objs = shift;
    my $obj_type = shift;
    my $obj_attr = shift;
    my $obj_val = shift;

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");
        if ($obj->{$obj_attr} eq $obj_val) {
            #debug("Found object: ".Dumper($obj));
            return $obj;
        }
    }

    return undef;
}

# get user object by attr 'user_name'
sub obj_get_user_obj_by_user_name {
    my $objs = shift;
    my $obj_type = 'user';
    my $obj_attr = 'user_name';
    my $obj_val = shift;

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");
        if ($obj->{$obj_attr} eq $obj_val) {
            #debug("Found object: ".Dumper($obj));
            return $obj;
        }
    }

    return undef;
}

# get template object by attr 'name'
sub obj_get_tmpl_obj_by_tmpl_name {
    my $objs = shift;
    my $obj_tmpl_type = shift;
    my $obj_attr_tmpl_name = shift;

    #debug("My objects hive: ".Dumper($objs));

    #Icinga2::Utils::debug("Checking for template name with $obj_attr_tmpl_name");
    foreach my $obj_key (keys %{@$objs{$obj_tmpl_type}}) {
        my $obj = @$objs{$obj_tmpl_type}->{$obj_key};
        next if !defined($obj->{'name'});
        # XXX it would be safe, but we cannot garantuee it here, so better check before if we want a template or not
        if ($obj->{'name'} eq $obj_attr_tmpl_name) {
            #Icinga2::Utils::debug("Found object: ".Dumper($obj));
            return $obj;
        }
    }

    return undef;
}


# get hostgroup object by attr 'hostgroup_name'
sub obj_get_hostgroup_obj_by_hostgroup_name {
    my $objs = shift;
    my $obj_type = 'hostgroup';
    my $obj_attr = 'hostgroup_name';
    my $obj_val = shift;

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");
        if ($obj->{$obj_attr} eq $obj_val) {
            #debug("Found object: ".Dumper($obj));
            return $obj;
        }
    }

    return undef;
}

#################################################################################
# Get Object Attribute Helpers 
#################################################################################


# get host_names by attr 'hostgroup_name'
sub obj_get_hostnames_arr_by_hostgroup_name {
    my $objs = shift;
    my $obj_type = 'host';
    my $obj_attr = 'hostgroups';
    my $obj_val = shift;
    my @host_names = ();

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");

        foreach my $hg (@{$obj->{$obj_attr}}) {
            if ($hg eq $obj_val) {
                #debug("Found object: ".Dumper($obj));
                push @host_names, $obj->{'host_name'};
            }
        }
    }

    return @host_names;
}

sub obj_get_usernames_arr_by_usergroup_name {
    my $objs = shift;
    my $obj_type = 'user';
    my $obj_attr = 'usergroups';
    my $obj_val = shift;
    my @user_names = ();

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");

        foreach my $user (@{$obj->{$obj_attr}}) {
            if ($user eq $obj_val) {
                #debug("Found object: ".Dumper($obj));
                push @user_names, $obj->{'user_name'};
            }
        }
    }

    return @user_names;
}

# used after relinking all services with servicegroups
sub obj_2x_get_service_arr_by_servicegroup_name {
    my $objs_2x = shift;
    my $objs = $objs_2x;
    my $obj_type = 'service';
    my $obj_attr = 'servicegroups';
    my $obj_val = shift;
    my @service_names = ();

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        # this there's no attr, try template tree
        my @servicegroups = ();
        my $host_name = $obj->{'__I2CONVERT_SERVICE_HOSTNAME'};

        # skip invalid resolved objects
        if (!defined($host_name)) {
            #say Dumper("missing host name...");
            next;
        }

        if (defined($obj->{$obj_attr}) && scalar(@{$obj->{$obj_attr}} > 0)) {
            push @servicegroups, @{$obj->{$obj_attr}};
            #say Dumper("$obj_attr ========== found in object $obj->{'__I2CONVERT_SERVICE_HOSTNAME'}:$obj->{'__I2CONVERT_SERVICEDESCRIPTION'}");
            #say Dumper(@servicegroups);
        } else {
            #say Dumper("START ------------------------");
            #say Dumper($obj);
            my @service_sgs = obj_2x_get_service_servicegroups($objs_2x,$obj,$host_name,$obj_attr);
            #say Dumper(@service_sgs);
            if (scalar @service_sgs > 0) {
                push @servicegroups, @service_sgs;
                #say Dumper("$obj_attr ========== found in template tree $obj->{'__I2CONVERT_SERVICE_HOSTNAME'}:$obj->{'__I2CONVERT_SERVICEDESCRIPTION'}");
                #say Dumper(@servicegroups);
                #say Dumper($obj);
            }
            #say Dumper("END ------------------------");
        }
        #debug("Getting attr $obj_attr and val $obj_val");

        # check if servicegroup_name is in the array of servicegroups for this processed service
        foreach my $servicegroup (@servicegroups) {
            # skip templates
            next if ($obj->{'__I2CONVERT_SERVICE_IS_TEMPLATE'} == 1);
            if ($servicegroup eq $obj_val) {
                #debug("Found object: ".Dumper($obj));
                my $service_name;
                $service_name->{'__I2CONVERT_SERVICE_HOSTNAME'} = $obj->{'__I2CONVERT_SERVICE_HOSTNAME'};
                $service_name->{'__I2CONVERT_SERVICEDESCRIPTION'} = $obj->{'__I2CONVERT_SERVICEDESCRIPTION'};
                push @service_names, $service_name;
            }
        }
    }

    return @service_names;
}

sub obj_1x_get_all_hostnames_arr {
    my $objs = shift;
    my $obj_type = 'host';
    my $obj_attr = 'host_name';
    my $obj_val = '*';
    my @host_names = ();

    #debug("My objects hive: ".Dumper($objs));

    #debug("Checking for type=$obj_type attr=$obj_attr val=$obj_val ");
    foreach my $obj_key (keys %{@$objs{$obj_type}}) {
        my $obj = @$objs{$obj_type}->{$obj_key};
        next if !defined($obj->{$obj_attr});
        #debug("Getting attr $obj_attr and val $obj_val");

        push @host_names, $obj->{$obj_attr};
    }

    return @host_names;
}



# get host_name from object
sub obj_1x_get_host_host_name {
    my $objs_1x = shift;
    my $obj_1x = shift;
    my $host_name = "";

    # if this object is invalid, bail early
    return undef if !defined($obj_1x);

    # first, check if we already got a host_name here in our struct (recursion safety)
    return $obj_1x->{'__I2CONVERT_HOSTNAME'} if defined($obj_1x->{'__I2CONVERT_HOSTNAME'});
    delete $obj_1x->{'__I2CONVERT_HOSTNAME'};

    # if this object got what we want, return (it can be recursion and a template!)
    if(defined($obj_1x->{'host_name'})) {
        $obj_1x->{'__I2CONVERT_HOSTNAME'} = $obj_1x->{'host_name'};
        return $obj_1x->{'__I2CONVERT_HOSTNAME'};
    }

    # we don't have a host name, should we look into a template?
    # make sure _not_ to use 
    if (defined($obj_1x->{'__I2CONVERT_USES_TEMPLATE'}) && $obj_1x->{'__I2CONVERT_USES_TEMPLATE'} == 1) {
        # get the object referenced as template - this is an array of templates, loop (funny recursion here)
        foreach my $obj_1x_template (@{$obj_1x->{'__I2CONVERT_TEMPLATE_NAMES'}}) {

            # get the template object associated with by its unique 'name' attr
            my $obj_1x_tmpl = obj_get_tmpl_obj_by_tmpl_name($objs_1x, 'host', $obj_1x_template);

            # now recurse into ourselves and look for a possible service_description
            $host_name = obj_1x_get_host_host_name($objs_1x,$obj_1x_tmpl);
            # bail here if search did not unveil anything
            next if(!defined($host_name));

            # get the host_name and return - first template wins
            $obj_1x->{'__I2CONVERT_HOSTNAME'} = $host_name;
            return $obj_1x->{'__I2CONVERT_HOSTNAME'};
        }
    }
    # no template used, and no host name - broken object, ignore it
    else {
        return undef;
    }

    # we should never hit here
    return undef;
}

# get host_name(s) from service object
sub obj_1x_get_service_host_name_arr {
# service objects may contain comma seperated host lists (ugly as ...)
    my $objs_1x = shift;
    my $obj_1x = shift;
    my @host_name = ();

    # if this object is invalid, bail early
    return undef if !defined($obj_1x);

    # first, check if we already got a host_name here in our struct (recursion safety)
    return $obj_1x->{'__I2CONVERT_HOSTNAMES'} if defined($obj_1x->{'__I2CONVERT_HOSTNAMES'});
    delete $obj_1x->{'__I2CONVERT_HOSTNAMES'};

    # if this object got what we want, return (it can be recursion and a template!)
    if(defined($obj_1x->{'host_name'})) {

        #print "DEBUG: found $obj_1x->{'host_name'}\n";

        # convert to array
        delete($obj_1x->{'__I2CONVERT_HOSTNAMES'});

        # check if host_name is a wildcard, or a possible comma seperated list
        # using object tricks - http://docs.icinga.org/latest/en/objecttricks.html#objecttricks-service
        if ($obj_1x->{'host_name'} =~ /^\*$/) {
            @host_name = obj_1x_get_all_hostnames_arr($objs_1x);
        } else {
            @host_name = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x->{'host_name'}, ',', 1);
        }
        push @{$obj_1x->{'__I2CONVERT_HOSTNAMES'}}, @host_name;

        #print "DEBUG: @{$obj_1x->{'__I2CONVERT_HOSTNAMES'}}"; 
        return @host_name;
    }

    # we don't have a host name, should we look into a template?
    # make sure _not_ to use 
    if (defined($obj_1x->{'__I2CONVERT_USES_TEMPLATE'}) && $obj_1x->{'__I2CONVERT_USES_TEMPLATE'} == 1) {
        # get the object referenced as template - this is an array of templates, loop (funny recursion here)
        foreach my $obj_1x_template (@{$obj_1x->{'__I2CONVERT_TEMPLATE_NAMES'}}) {

            #say Dumper($obj_1x_template);

            # get the template object associated with by its unique 'name' attr
            my $obj_1x_tmpl = obj_get_tmpl_obj_by_tmpl_name($objs_1x, 'service', $obj_1x_template);

            # now recurse into ourselves and look for all possible hostnames in array 
            @host_name = obj_1x_get_service_host_name_arr($objs_1x,$obj_1x_tmpl);
            #print "DEBUG: from tmpl $obj_1x_template: " . join(" ", @host_name) . "\n";

            # bail here if search did not unveil anything
            next if(!@host_name);

            # get the host_name and return - first template wins
            # convert to array
            delete($obj_1x->{'__I2CONVERT_HOSTNAMES'});
            push @{$obj_1x->{'__I2CONVERT_HOSTNAMES'}}, @host_name;
            return @host_name;
        }
    }
    # no template used, and no host name - broken object, ignore it
    else {
        return undef;
    }

    # we should never hit here
    return undef;
}

# get service_description from object
sub obj_1x_get_service_service_description {
    my $objs_1x = shift;
    my $obj_1x = shift;
    my $host_name = shift;
    my $service_description = "";

    # if this object is invalid, bail early
    return undef if !defined($obj_1x);

    # first, check if we already got a service_description here in our struct (recursion safety)
    return $obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'} if defined($obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'});
    delete $obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'};

    # if this object got what we want, return (it can be recursion and a template!)
    if(defined($obj_1x->{'service_description'})) {
        $obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'} = $obj_1x->{'service_description'};
        return $obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'};
    }

    # we don't have a service description, should we look into a template?
    # make sure _not_ to use 
    if (defined($obj_1x->{'__I2CONVERT_USES_TEMPLATE'}) && $obj_1x->{'__I2CONVERT_USES_TEMPLATE'} == 1) {
        # get the object referenced as template - this is an array of templates, loop (funny recursion here)
        foreach my $obj_1x_template (@{$obj_1x->{'__I2CONVERT_TEMPLATE_NAMES'}}) {

            # get the template object associated with by its unique 'name' attr
            my $obj_1x_tmpl = obj_get_tmpl_obj_by_tmpl_name($objs_1x, 'service', $obj_1x_template);

            # now recurse into ourselves and look for a possible service_description
            $service_description = obj_1x_get_service_service_description($objs_1x,$obj_1x_tmpl,$host_name); # we must pass the host_name
            # bail here if search did not unveil anything
            next if(!defined($service_description));

            # get the service description and return - first template wins
            $obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'} = $service_description;
            return $obj_1x->{'__I2CONVERT_SERVICEDESCRIPTION'};
        }
    }
    # no template used, and not service description - broken object, ignore it
    else {
        return undef;
    }

    # we should never hit here
    return undef;
}

# get service_description from object
sub obj_1x_get_service_attr {
    my $objs_1x = shift;
    my $obj_1x = shift;
    my $host_name = shift;
    my $search_attr = shift;
    my $service_attr = "";

    # if this object is invalid, bail early
    return undef if !defined($obj_1x);

    # first, check if we already got a service_description here in our struct (recursion safety)
    return $obj_1x->{'__I2CONVERT_SEARCH_ATTR'} if defined($obj_1x->{'__I2CONVERT_SEARCH_ATTR'});
    delete $obj_1x->{'__I2CONVERT_SEARCH_ATTR'};

    # if this object got what we want, return (it can be recursion and a template!)
    if(defined($obj_1x->{$search_attr})) {
        $obj_1x->{'__I2CONVERT_SEARCH_ATTR'} = $obj_1x->{$search_attr};
        return $obj_1x->{$search_attr};
        #return $obj_1x->{'__I2CONVERT_SEARCH_ATTR'};
    }

    # we don't have the attribute, should we look into a template?
    # make sure _not_ to use 
    if (defined($obj_1x->{'__I2CONVERT_USES_TEMPLATE'}) && $obj_1x->{'__I2CONVERT_USES_TEMPLATE'} == 1) {
        # get the object referenced as template - this is an array of templates, loop (funny recursion here)
        foreach my $obj_1x_template (@{$obj_1x->{'__I2CONVERT_TEMPLATE_NAMES'}}) {

            # get the template object associated with by its unique 'name' attr
            my $obj_1x_tmpl = obj_get_tmpl_obj_by_tmpl_name($objs_1x, 'service', $obj_1x_template);

            # now recurse into ourselves and look for a possible service_description
            $service_attr = obj_1x_get_service_attr($objs_1x,$obj_1x_tmpl,$host_name,$search_attr); # we must pass the host_name and search_attr
            #say Dumper($service_attr);
            # bail here if search did not unveil anything
            next if(!defined($service_attr));

            # get the service attr and return - first template wins
            $obj_1x->{'__I2CONVERT_SEARCH_ATTR'} = $service_attr;
            return $service_attr;
            #return $obj_1x->{'__I2CONVERT_SEARCH_ATTR'};
        }
    }
    # no template used, and not service description - broken object, ignore it
    else {
        return undef;
    }

    # we should never hit here
    return undef;
}

# get service_description from object
sub obj_1x_get_contact_attr {
    my $objs_1x = shift;
    my $obj_1x = shift;
    my $search_attr = shift;
    my $contact_attr = "";

    # if this object is invalid, bail early
    return undef if !defined($obj_1x);

    # first, check if we already got a attr here in our struct (recursion safety)
    return $obj_1x->{'__I2CONVERT_SEARCH_ATTR'} if defined($obj_1x->{'__I2CONVERT_SEARCH_ATTR'});
    delete $obj_1x->{'__I2CONVERT_SEARCH_ATTR'};

    # if this object got what we want, return (it can be recursion and a template!)
    if(defined($obj_1x->{$search_attr})) {
        $obj_1x->{'__I2CONVERT_SEARCH_ATTR'} = $obj_1x->{$search_attr};
        return $obj_1x->{'__I2CONVERT_SEARCH_ATTR'};
    }

    # we don't have the attribute, should we look into a template?
    # make sure _not_ to use 
    if (defined($obj_1x->{'__I2CONVERT_USES_TEMPLATE'}) && $obj_1x->{'__I2CONVERT_USES_TEMPLATE'} == 1) {
        # get the object referenced as template - this is an array of templates, loop (funny recursion here)
        foreach my $obj_1x_template (@{$obj_1x->{'__I2CONVERT_TEMPLATE_NAMES'}}) {

            # get the template object associated with by its unique 'name' attr
            my $obj_1x_tmpl = obj_get_tmpl_obj_by_tmpl_name($objs_1x, 'contact', $obj_1x_template);

            # now recurse into ourselves and look for a possible contact attr
            $contact_attr = obj_1x_get_contact_attr($objs_1x,$obj_1x_tmpl,$search_attr); # we must pass the search_attr
            # bail here if search did not unveil anything
            next if(!defined($contact_attr));

            # get the contact attr and return - first template wins
            $obj_1x->{'__I2CONVERT_SEARCH_ATTR'} = $contact_attr;
            return $obj_1x->{'__I2CONVERT_SEARCH_ATTR'};
        }
    }
    # no template used, and attr - broken object, ignore it
    else {
        return undef;
    }

    # we should never hit here
    return undef;
}

# get servicegroups  from object (already 2x and _array_ XXX)
sub obj_2x_get_service_servicegroups {
    my $objs_2x = shift;
    my $obj_2x = shift;
    my $host_name = shift;
    my $search_attr = shift;
    my @service_groups;

    #say Dumper("in obj_2x_get_service_attr");
    # if this object is invalid, bail early
    return undef if !defined($obj_2x);

    # if this object got what we want, return (it can be recursion and a template!)
    if(defined($obj_2x->{$search_attr}) && scalar(@{$obj_2x->{$search_attr}}) > 0) {
        #say Dumper("in obj_2x_get_service_attr. found ");
        #say Dumper($obj_2x->{$search_attr});
        return @{$obj_2x->{$search_attr}};
    }

    # we don't have the attribute, should we look into a template?
    # make sure _not_ to use 
    if (defined($obj_2x->{'__I2CONVERT_USES_TEMPLATE'}) && $obj_2x->{'__I2CONVERT_USES_TEMPLATE'} == 1) {
        # get the object referenced as template - this is an array of templates, loop (funny recursion here)
        foreach my $obj_2x_template (@{$obj_2x->{'__I2CONVERT_TEMPLATE_NAMES'}}) {

            #say Dumper("in obj_2x_get_service_attr template");
            #say Dumper($obj_2x_template);
            # get the template object associated with by its unique 'name' attr
            my $obj_2x_tmpl = obj_get_tmpl_obj_by_tmpl_name($objs_2x, 'service', $obj_2x_template);
            #say Dumper($obj_2x_tmpl);

            # now recurse into ourselves and look for a possible service_description
            push @service_groups, obj_2x_get_service_servicegroups($objs_2x,$obj_2x_tmpl,$host_name,$search_attr); # we must pass the host_name and search_attr
            #say Dumper($service_attr);
            # bail here if search did not unveil anything
            next if(scalar(@service_groups) == 0);

            # get the service attr and return - first template wins
            return @service_groups;
        }
    }
    # no template used, and not service description - broken object, ignore it
    else {
        return undef;
    }

    # we should never hit here
    return undef;
}


################################################################################
# Conversion 
#################################################################################

# host|service_notification_commands are a comma seperated list w/o arguments
sub convert_notificationcommand {
    my $objs_1x = shift;
    my $commands_1x = shift;
    my $obj_1x = shift;
    my $user_macros_1x = shift;
    my $command_name_1x;
    my @commands = ();
    my $notification_commands_2x = ();
    my $host_notification_commands;
    my $service_notification_commands;

    # bail early if this is not a valid contact object
    return undef if (!defined($obj_1x->{'contact_name'}));

    # bail early if required commands not available (not a valid 1.x object either)
    if (defined($obj_1x->{'host_notification_commands'})) {
        $host_notification_commands = $obj_1x->{'host_notification_commands'};
    }
    else {
        # look in the template
        $host_notification_commands = obj_1x_get_contact_attr($objs_1x,$obj_1x,'host_notification_commands');
    }
    if (defined($obj_1x->{'service_notification_commands'})) {
        $service_notification_commands = $obj_1x->{'service_notification_commands'};
    }
    else {
        # look in the template
        $service_notification_commands = obj_1x_get_contact_attr($objs_1x,$obj_1x,'service_notification_commands');
    }


    # a contact has a comma seperated list of notification commands by host and service
    my $all_notification_commands = {};
    push @{$all_notification_commands->{'host'}}, split /,\s+/, $host_notification_commands;
    push @{$all_notification_commands->{'service'}}, split /,\s+/, $service_notification_commands;


    foreach my $obj_notification_command_key ( keys %{$all_notification_commands}) {

        # fetch all command names in array via type key
        my @notification_commands = @{$all_notification_commands->{$obj_notification_command_key}};
        my $notification_command_type = $obj_notification_command_key;

        foreach my $notification_command (@notification_commands) {

            # now back in all command objects of 1.x
            foreach my $command_1x_key (keys %{$commands_1x}) {
                chomp $notification_command; # remove trailing spaces

                if ($commands_1x->{$command_1x_key}->{'command_name'} eq $notification_command) {
                    # save the type (host, service) and then by command name
                    $notification_commands_2x->{$notification_command_type}->{$notification_command} = Icinga2::Utils::escape_str($commands_1x->{$command_1x_key}->{'command_line'});
                    #say Dumper($commands_1x->{$command_1x_key});
                }
            }

        }
    }

    return $notification_commands_2x;

} 

# event_handler
sub convert_eventhandler {
    my $commands_1x = shift;
    my $obj_1x = shift;
    my $user_macros_1x = shift;
    my $command_name_1x;
    my @commands = ();
    my $event_commands_2x = ();

    # bail early if required commands not available (not a valid 1.x object either)
    return if (!defined($obj_1x->{'event_handler'}));

    my $event_command = $obj_1x->{'event_handler'};
    #say Dumper($event_command);

    # now back in all command objects of 1.x
    foreach my $command_1x_key (keys %{$commands_1x}) {
        chomp $event_command; # remove trailing spaces

        if ($commands_1x->{$command_1x_key}->{'command_name'} eq $event_command) {
            # save the command line and command name
            $event_commands_2x->{'command_name'} = $event_command;
            $event_commands_2x->{'command_line'} = Icinga2::Utils::escape_str($commands_1x->{$command_1x_key}->{'command_line'});
        }
    }

    return $event_commands_2x;

}


# check_command accepts argument parameters, special treatment
sub convert_checkcommand {
    my $commands_1x = shift;
    my $obj_1x = shift; #host or service
    my $user_macros_1x = shift;

    my $command_1x;
    my $command_2x = {};
    #say Dumper($commands_1x); 
    #say Dumper($obj_1x); 

    # ignore objects without check_command (may defined in template!)
    return if (!defined($obj_1x->{'check_command'}));

    #debug("check_command: $obj_1x->{'check_command'}" );
    # split by ! and take only the check command
    my ($real_command_name_1x, @command_args_1x) = split /!/, $obj_1x->{'check_command'};

    # ignore objects with empty check_command attribute
    #return if (!defined($real_command_name_1x));

    #debug("1x Command Name: $real_command_name_1x");
    if (@command_args_1x) {
        #debug("1x Command Args: @command_args_1x");
    }

    foreach my $command_1x_key (keys %{$commands_1x}) {

        #say Dumper($commands_1x->{$command_1x_key}->{'command_name'});
        if ($commands_1x->{$command_1x_key}->{'command_name'} eq $real_command_name_1x) {
            #debug("Found: $real_command_name_1x");

            # save the command_line and the $ARGn$ macros
            $command_2x->{'check_command'} = Icinga2::Utils::escape_str($commands_1x->{$command_1x_key}->{'command_line'});
            $command_2x->{'check_command_name_1x'} = $real_command_name_1x;
            #Icinga2::Utils::debug("2x Command: $command_2x->{'check_command'}");

            # detect $USERn$ macros and replace them too XXX - this should be a global macro?
            if ($commands_1x->{$command_1x_key}->{'command_line'} =~/\$(USER\d)\$/) {
                $command_2x->{'command_macros'}->{$1} = Icinga2::Utils::escape_str($user_macros_1x->{$1});
                #debug("\$$1\$=$command_2x->{'macros'}->{$1}");
            }

            # save all command args as macros (we'll deal later with them in service definitions)
            my $arg_cnt = 1;
            foreach my $command_arg_1x (@command_args_1x) {
                my $macro_name_2x = "ARG" . $arg_cnt;
                $command_2x->{'command_macros'}->{$macro_name_2x} = Icinga2::Utils::escape_str($command_arg_1x);
                #debug("\$$macro_name_2x\$=$command_2x->{'macros'}->{$macro_name_2x}");
                $arg_cnt++;
            }
        }
    }

    return $command_2x;
}


# convert existing 1x objects into the 2x objects hive
sub convert_2x {
# v1 -> v2
# register 0 == template
# use == inherits template
# dependency == ...
# escalation == ...

    # hashref is selectable by type first
    my $icinga2_cfg = shift;
    my $cfg_obj_1x = shift;
    my $cfg_obj_cache_1x = shift;
    my $user_macros_1x = shift;

    # build a new hashref with the actual 2.x config inside
    my $cfg_obj_2x = {};

    my $command_obj_cnt = 0;

    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = '__I2CONVERT_COMMAND_DUMMY';

    ######################################
    # SERVICE
    # do the magic lookup for host_name/
    # service_description for each object
    # only once
    ######################################
    my $service_cnt = 0;

    foreach my $service_obj_1x_key (keys %{@$cfg_obj_1x{'service'}}) {

        #say Dumper(@$cfg_obj_1x{'service'}->{$service_obj_1x_key});
        my $obj_1x_service = @$cfg_obj_1x{'service'}->{$service_obj_1x_key};

        ####################################################
        # verify template is/use
        ####################################################
        $obj_1x_service->{'__I2CONVERT_IS_TEMPLATE'} = obj_1x_is_template($obj_1x_service);
        $obj_1x_service->{'__I2CONVERT_USES_TEMPLATE'} = obj_1x_uses_template($obj_1x_service);
        $obj_1x_service->{'__I2CONVERT_TEMPLATE_NAME'} = $obj_1x_service->{'name'};

        # this can be a comma seperated list of templates
        my @service_templates = ();
        if(defined($obj_1x_service->{'use'})) {
            @service_templates = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_service->{'use'}, ',', 1);
        }
        
        push @{$obj_1x_service->{'__I2CONVERT_TEMPLATE_NAMES'}}, @service_templates;

        # add dependency to ITL template to objects
        if ($obj_1x_service->{'__I2CONVERT_IS_TEMPLATE'} == 0) {
            if(defined($icinga2_cfg->{'itl'}->{'service-template'}) && $icinga2_cfg->{'itl'}->{'service-template'} ne "") {
                push @{$obj_1x_service->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'service-template'};
                $obj_1x_service->{'__I2CONVERT_USES_TEMPLATE'} = 1;
            }
        }

        ####################################################
        # get related host_name/service_description
        # used later in host->service resolval
        ####################################################
        # XXX even if the service object uses templates, we need to figure out its host_name/service_description in order to safely link hosts towards it
        my @host_names = obj_1x_get_service_host_name_arr($cfg_obj_1x, $obj_1x_service);
       
        #print "DEBUG: service @host_names\n";

        delete($obj_1x_service->{'__I2CONVERT_HOSTNAMES'});
        if(@host_names == 0) {
            # set a dummy value for postprocessing - we need the prepared 2.x service for later object tricks
            push @host_names, "__I2CONVERT_DUMMY";
        }
        push @{$obj_1x_service->{'__I2CONVERT_HOSTNAMES'}}, @host_names;


        # if there is more than one host_name involved on the service object, clone it in a loop
        foreach my $service_host_name (@{$obj_1x_service->{'__I2CONVERT_HOSTNAMES'}}) {

            # we can only look up services with their uniqueness to the host_name
            $obj_1x_service->{'__I2CONVERT_SERVICEDESCRIPTION'} = obj_1x_get_service_service_description($cfg_obj_1x, $obj_1x_service, $service_host_name);
            #say Dumper($obj_1x_service);

            # skip non-template objects without a valid service description (we cannot tolerate 'name' here!)
            # XXX find a better way - we actually need all services in the list, even if __I2CONVERT_SERVICEDESCRIPTION is undef
            if (!defined($obj_1x_service->{'__I2CONVERT_SERVICEDESCRIPTION'}) && $obj_1x_service->{'__I2CONVERT_IS_TEMPLATE'} == 0) {
                #Icinga2::Utils::debug("Skipping invalid service object without service_description ".Dumper($obj_1x_service));
                next;
            }

            ####################################################
            # clone service object into 2.x
            ####################################################
            $cfg_obj_2x->{'service'}->{$service_cnt} = dclone(@$cfg_obj_1x{'service'}->{$service_obj_1x_key});

            # immediately overwrite the correct host_name - if there's no dummy value set for further processing
            if ($service_host_name !~ /__I2CONVERT_DUMMY/) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{__I2CONVERT_SERVICE_HOSTNAME} = $service_host_name;
            }

            #say Dumper($cfg_obj_2x->{'service'}->{$service_cnt});
            ####################################################
            # map existing service attributes
            # same:
            # - display_name
            # - max_check_attempts
            # - action_url
            # - notes_url
            # - notes
            # - icon_image
            # - notes
            # change:
            # - servicegroups (commaseperated strings to array)
            # - check_command
            # - check_interval (X min -> Xm) + normal_check_interval
            # - retry_interval (X min -> Xm) + retry_check_interval
            # - notification_interval (X min -> Xm)
            # - check_period - XXX TODO
            # - notification_period - XXX TODO
            # - contacts => users XXX DO NOT DELETE contacts and contactgroups, they will be assembled later for notifications!
            # - 
            ####################################################

            ##########################################
            # escape strings in attributes
            ##########################################
            if(defined($cfg_obj_2x->{'service'}->{$service_cnt}->{'action_url'})) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'action_url'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'service'}->{$service_cnt}->{'action_url'});
            }
            if(defined($cfg_obj_2x->{'service'}->{$service_cnt}->{'notes_url'})) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'notes_url'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'service'}->{$service_cnt}->{'notes_url'});
            }
            if(defined($cfg_obj_2x->{'service'}->{$service_cnt}->{'notes'})) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'notes'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'service'}->{$service_cnt}->{'notes'});
            }
            if(defined($cfg_obj_2x->{'service'}->{$service_cnt}->{'icon_image'})) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'icon_image'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'service'}->{$service_cnt}->{'icon_image'});
            }
            if(defined($cfg_obj_2x->{'service'}->{$service_cnt}->{'icon_image_alt'})) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'icon_image_alt'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'service'}->{$service_cnt}->{'icon_image_alt'});
            }

            ##########################################
            # servicegroups
            ##########################################
            delete($cfg_obj_2x->{'service'}->{$service_cnt}->{'servicegroups'});
            # debug #
            @{$cfg_obj_2x->{'service'}->{$service_cnt}->{'servicegroups'}} = ();

            if(defined($obj_1x_service->{'servicegroups'})) {
                # check if there's additive inheritance required, and save a flag
                if ($obj_1x_service->{'servicegroups'} =~ /^\+/) {
                    $cfg_obj_2x->{'service'}->{$service_cnt}->{'__I2_CONVERT_SG_ADD'} = 1;
                    $obj_1x_service->{'servicegroups'} =~ s/^\+//;
                }
                # convert comma seperated list to array
                push @{$cfg_obj_2x->{'service'}->{$service_cnt}->{'servicegroups'}}, Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_service->{'servicegroups'}, ',', 1);
                #print "DEBUG: servicegroups " . join (" ", @{$cfg_obj_2x->{'service'}->{$service_cnt}->{'servicegroups'}});
            }
            #say Dumper($cfg_obj_2x->{'service'}->{$service_cnt}->{__I2CONVERT_SERVICE_HOSTNAME});
            #say Dumper($cfg_obj_2x->{'service'}->{$service_cnt}->{'servicegroups'});

            ##########################################
            # check_interval
            ##########################################
            my $service_check_interval = undef;
            if(defined($obj_1x_service->{'normal_check_interval'})) {
                $service_check_interval = $obj_1x_service->{'normal_check_interval'};
            } 
            if(defined($obj_1x_service->{'check_interval'})) {
                $service_check_interval = $obj_1x_service->{'check_interval'};
            } 
            # we assume that 1.x kept 1m default interval, and map it
            if (defined($service_check_interval)) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'check_interval'} = $service_check_interval."m";
            }

            ##########################################
            # retry_interval
            ##########################################
            my $service_retry_interval = undef;
            if(defined($obj_1x_service->{'retry_check_interval'})) {
                $service_retry_interval = $obj_1x_service->{'retry_check_interval'};
            } 
            if(defined($obj_1x_service->{'retry_interval'})) {
                $service_retry_interval = $obj_1x_service->{'retry_interval'};
            } 
            # we assume that 1.x kept 1m default interval, and map it
            if (defined($service_retry_interval)) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'retry_interval'} = $service_retry_interval."m";
            }

            ##########################################
            # notification_interval
            ##########################################
            my $service_notification_interval = undef;
            if(defined($obj_1x_service->{'notification_interval'})) {
                $service_notification_interval = $obj_1x_service->{'notification_interval'};
            }
            # we assume that 1.x kept 1m default interval, and map it
            if (defined($service_notification_interval)) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'notification_interval'} = $service_notification_interval."m";
            }

            ##########################################
            # eventhandler
            ##########################################
            if (defined($obj_1x_service->{'event_handler'})) {

                my $service_event_command_2x = Icinga2::Convert::convert_eventhandler(@$cfg_obj_1x{'command'}, $obj_1x_service, $user_macros_1x);
                #say Dumper($service_event_command_2x);

                # XXX do not add duplicate event commands, they must remain unique by their check_command origin!
                if ((obj_2x_command_exists($cfg_obj_2x, $obj_1x_service->{'event_handler'}) != 1)) {
                
                    # create a new EventCommand 2x object with the original name
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_TYPE'} = 'Event';
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = $service_event_command_2x->{'command_name'};
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_LINE'} = $service_event_command_2x->{'command_line'};

                    # use the ITL plugin check command template
                    if(defined($icinga2_cfg->{'itl'}->{'eventcommand-template'}) && $icinga2_cfg->{'itl'}->{'eventcommand-template'} ne "") {
                        push @{$cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'eventcommand-template'};
                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                    }

                    # our PK
                    $command_obj_cnt++;
                }
                    
                # the event_handler name of 1.x is still the unique command object name, so we just keep 
                # in __I2_CONVERT_EVENTCOMMAND_NAME in our service object
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'__I2_CONVERT_EVENTCOMMAND_NAME'} = $service_event_command_2x->{'command_name'};

            }


            ##########################################
            # volatile is bool only
            ##########################################
            if (defined($obj_1x_service->{'is_volatile'})) {
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'volatile'} = ($obj_1x_service->{'is_volatile'} > 0) ? 1 : 0;
            }

            ##########################################
            # map the service check_command to 2.x
            ##########################################
            my $service_check_command_2x = Icinga2::Convert::convert_checkcommand(@$cfg_obj_1x{'command'}, $obj_1x_service, $user_macros_1x);

            #say Dumper($service_check_command_2x);

            if (defined($service_check_command_2x->{'check_command_name_1x'})) {

                # XXX do not add duplicate check commands, they must remain unique by their check_command origin!
                if (obj_2x_command_exists($cfg_obj_2x, $service_check_command_2x->{'check_command_name_1x'}) != 1) {

                    # create a new CheckCommand 2x object with the original name
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_TYPE'} = 'Check';
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = $service_check_command_2x->{'check_command_name_1x'};
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_LINE'} = $service_check_command_2x->{'check_command'};

                    # use the ITL plugin check command template
                    if(defined($icinga2_cfg->{'itl'}->{'checkcommand-template'}) && $icinga2_cfg->{'itl'}->{'checkcommand-template'} ne "") {
                        push @{$cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'checkcommand-template'};
                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                    }

                    # add the command macros to the command 2x object
                    if(defined($service_check_command_2x->{'command_macros'})) {
                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_MACROS'} = dclone($service_check_command_2x->{'command_macros'});
                    }

                    # our PK
                    $command_obj_cnt++;
                }

                # make sure service object still got the checkcommand assigned
                # the check command name of 1.x is still the unique command object name, so we just keep 
                # in $service_check_command_2x->{'check_command'} the cut real check_command_name_1x
                delete($service_check_command_2x->{'check_command'});
                $cfg_obj_2x->{'service'}->{$service_cnt}->{'__I2_CONVERT_CHECKCOMMAND_NAME'} = $service_check_command_2x->{'check_command_name_1x'};

            }

            # XXX make sure to always add the service specific command arguments, since we have a n .. 1 relation here
            # add the command macros to the command 2x object
            if(defined($service_check_command_2x->{'command_macros'})) {
                @$cfg_obj_2x{'service'}->{$service_cnt}->{'__I2CONVERT_MACROS'} = dclone($service_check_command_2x->{'command_macros'});
            }

            # our PK
            $service_cnt++;
        }

    }
    ######################################
    # HOST
    # use => inherit template
    # register 0 => template
    # check_command => create a new service? 
    ######################################

    # "get all 'host' hashref as array in hashmap, and their keys to access it"    
    foreach my $host_obj_1x_key (keys %{@$cfg_obj_1x{'host'}}) {

        #say Dumper(@$cfg_obj_1x{'host'}->{$host_obj_1x_key});
        my $obj_1x_host = @$cfg_obj_1x{'host'}->{$host_obj_1x_key};

        ####################################################
        # verify template is/use
        ####################################################
        $obj_1x_host->{'__I2CONVERT_IS_TEMPLATE'} = obj_1x_is_template($obj_1x_host);
        $obj_1x_host->{'__I2CONVERT_USES_TEMPLATE'} = obj_1x_uses_template($obj_1x_host);
        $obj_1x_host->{'__I2CONVERT_TEMPLATE_NAME'} = $obj_1x_host->{'name'};

        # this can be a comma seperated list of templates
        my @host_templates = ();
        if(defined($obj_1x_host->{'use'})) {
            @host_templates = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_host->{'use'}, ',', 1);
        }
        
        push @{$obj_1x_host->{'__I2CONVERT_TEMPLATE_NAMES'}}, @host_templates;

        ####################################################
        # get the related host_name
        ####################################################
        # XXX even if the host object uses templates, we need to figure out its host_name in order to safely link services towards it
        $obj_1x_host->{'__I2CONVERT_HOSTNAME'} = obj_1x_get_host_host_name($cfg_obj_1x, $obj_1x_host);

        ####################################################
        # skip objects without a valid hostname
        ####################################################
        if (!defined($obj_1x_host->{'__I2CONVERT_HOSTNAME'}) && $obj_1x_host->{'__I2CONVERT_IS_TEMPLATE'} == 0) {
            #Icinga2::Utils::debug("Skipping invalid host object without host_name ".Dumper($obj_1x_host));
            next;
        }
        #say Dumper($obj_1x_host);
        #

        # FIXME do that later on
        # primary host->service relation resolval

        ####################################################
        # Clone the existing object into 2.x
        ####################################################
        # save a copy with the valid ones
        $cfg_obj_2x->{'host'}->{$host_obj_1x_key} = dclone(@$cfg_obj_1x{'host'}->{$host_obj_1x_key});

        ####################################################
        # map existing host attributes
        # same:
        # - max_check_attempts
        # - action_url
        # - notes_url
        # - notes
        # - icon_image
        # - statusmap_image
        # - notes
        # change:
        # - display_name (if alias is set, overwrites it)
        # - hostgroups (commaseperated strings to array)
        # - check_interval (X min -> Xm) + normal_check_interval
        # - retry_interval (X min -> Xm) + retry_check_interval
        # - notification_interval (X min -> Xm)
        # - check_period - XXX TODO
        # - notification_period - XXX TODO
        # - contacts => users XXX DO NOT DELETE contacts and contactgroups - they will be assembled later for notifications!
        # - 
        # remove:
        # - check_command
        ####################################################

        ##########################################
        # macros (address*, etc)  
        ##########################################
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'address'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'__I2CONVERT_MACROS'}->{'address'} = $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'address'};
        }
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'address6'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'__I2CONVERT_MACROS'}->{'address6'} = $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'address6'};
        }

        ##########################################
        # escape strings in attributes
        ##########################################
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'action_url'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'action_url'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'action_url'});
        }
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notes_url'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notes_url'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notes_url'});
        }
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notes'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notes'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notes'});
        }
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'icon_image'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'icon_image'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'icon_image'});
        }
        if(defined($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'icon_image_alt'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'icon_image_alt'} = Icinga2::Utils::escape_str($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'icon_image_alt'});
        }

        ####################################################
        # display_name -> alias mapping
        ####################################################
        # if there was an host alias defined, make this the primary display_name for 2x
        if(defined($obj_1x_host->{'alias'})) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'display_name'} = Icinga2::Utils::escape_str($obj_1x_host->{'alias'});
        }

        ##########################################
        # hostgroups
        ##########################################
        delete($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'hostgroups'});

        if(defined($obj_1x_host->{'hostgroups'})) {
            # check if there's additive inheritance required, and save a flag
            if ($obj_1x_host->{'hostgroups'} =~ /^\+/) {
                $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'__I2_CONVERT_HG_ADD'} = 1;
                $obj_1x_host->{'hostgroups'} =~ s/^\+//;
            }

            # convert comma seperated list to array
            push @{$cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'hostgroups'}}, Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_host->{'hostgroups'}, ',', 1);
            #print "DEBUG: hostgroups " . join (" ", @{$cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'hostgroups'}});
        }

        ##########################################
        # check_interval
        ##########################################
        my $host_check_interval = undef;
        if(defined($obj_1x_host->{'normal_check_interval'})) {
            $host_check_interval = $obj_1x_host->{'normal_check_interval'};
        }
        if(defined($obj_1x_host->{'check_interval'})) {
            $host_check_interval = $obj_1x_host->{'check_interval'};
        }
        # we assume that 1.x kept 1m default interval, and map it
        if (defined($host_check_interval)) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'check_interval'} = $host_check_interval."m";
        }

        ##########################################
        # retry_interval
        ##########################################
        my $host_retry_interval = undef;
        if(defined($obj_1x_host->{'retry_check_interval'})) {
            $host_retry_interval = $obj_1x_host->{'retry_check_interval'};
        }
        if(defined($obj_1x_host->{'retry_interval'})) {
            $host_retry_interval = $obj_1x_host->{'retry_interval'};
        }
        # we assume that 1.x kept 1m default interval, and map it
        if (defined($host_retry_interval)) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'retry_interval'} = $host_retry_interval."m";
        }

        ##########################################
        # notification_interval
        ##########################################
        my $host_notification_interval = undef;
        if(defined($obj_1x_host->{'notification_interval'})) {
            $host_notification_interval = $obj_1x_host->{'notification_interval'};
        }
        # we assume that 1.x kept 1m default interval, and map it
        if (defined($host_notification_interval)) {
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'notification_interval'} = $host_notification_interval."m";
        }

        if(defined($obj_1x_host->{'parents'})) {
            my @host_parents = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_host->{'parents'}, ',', 1);
            push @{$cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'__I2CONVERT_PARENT_HOSTNAMES'}}, @host_parents;
        }
        ####################################################
        # Icinga 2 Hosts don't have a check_command anymore
        # - get a similar service with command_name lookup
        #   and link that service
        ####################################################

        my $host_check_command_2x = Icinga2::Convert::convert_checkcommand(@$cfg_obj_1x{'command'}, $obj_1x_host, $user_macros_1x);
        #say Dumper($host_check_command_2x);

        delete($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'check_command'});

        if(defined($host_check_command_2x->{'check_command_name_1x'})) {
            # XXX TODO match on the command_name in available services for _this_ host later on. right on, just save it
            $cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'__I2CONVERT_HOSTCHECK_NAME'} = $host_check_command_2x->{'check_command_name_1x'};
        }

        # XXX skip all host templates, they do not need to be linked with services!
        #if ($cfg_obj_2x->{'host'}->{$host_obj_1x_key}->{'__I2CONVERT_IS_TEMPLATE'} == 1) {
        #    Icinga2::Utils::debug("Skipping host template for linking against service.");
        #    next;
        #}

        # NOTE: the relation between host and services for 2x will be done later
        # this is due to the reason we may manipulate service objects later
        # e.g. when relinking the servicegroup members, etc
        # otherwise we would have to make sure to update 2 locations everytime
    }

    ######################################
    # CONTACT => USER
    ######################################
    my $user_cnt = 0;

    if (!@$cfg_obj_1x{'contact'}) {
        goto SKIP_CONTACTS;
    }

    foreach my $contact_obj_1x_key (keys %{@$cfg_obj_1x{'contact'}}) {
        my $obj_1x_contact = @$cfg_obj_1x{'contact'}->{$contact_obj_1x_key};

        ####################################################
        # verify template is/use
        ####################################################
        $obj_1x_contact->{'__I2CONVERT_IS_TEMPLATE'} = obj_1x_is_template($obj_1x_contact);
        $obj_1x_contact->{'__I2CONVERT_USES_TEMPLATE'} = obj_1x_uses_template($obj_1x_contact);
        $obj_1x_contact->{'__I2CONVERT_TEMPLATE_NAME'} = $obj_1x_contact->{'name'}; # XXX makes sense when IS_TEMPLATE is set

        # this can be a comma seperated list of templates
        my @contact_templates = ();
        if(defined($obj_1x_contact->{'use'})) {
            @contact_templates = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_contact->{'use'}, ',', 1);
        }
        
        push @{$obj_1x_contact->{'__I2CONVERT_TEMPLATE_NAMES'}}, @contact_templates;

        ####################################################
        # get all notification commands
        ####################################################
        my $notification_commands_2x = Icinga2::Convert::convert_notificationcommand($cfg_obj_1x, @$cfg_obj_1x{'command'}, $obj_1x_contact, $user_macros_1x);
        #say Dumper($obj_1x_contact);
        #say Dumper($notification_commands_2x);
        #say Dumper("======================================");

        # clone it into our users hash
        $cfg_obj_2x->{'user'}->{$contact_obj_1x_key} = dclone(@$cfg_obj_1x{'contact'}->{$contact_obj_1x_key});

        # set our own __I2CONVERT_TYPE
        $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'__I2CONVERT_TYPE'} = "user";

        ##########################################
        # macros (email, pager, address1..6)  
        ##########################################
        if(defined($obj_1x_contact->{'email'})) {
            $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'__I2CONVERT_MACROS'}->{'email'} = $obj_1x_contact->{'email'};
        }
        if(defined($obj_1x_contact->{'pager'})) {
            $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'__I2CONVERT_MACROS'}->{'pager'} = $obj_1x_contact->{'pager'};
        }
        for(my $i=1;$i<=6;$i++) {
            my $address = "address$i";
            if(defined($obj_1x_contact->{$address})) {
                $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'__I2CONVERT_MACROS'}->{$address} = $obj_1x_contact->{$address};
            }
        }

        ####################################################
        # migrate renamed attributes
        ####################################################
        $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'user_name'} = $obj_1x_contact->{'contact_name'};

        if(defined($obj_1x_contact->{'alias'})) {
            $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'display_name'} = Icinga2::Utils::escape_str($obj_1x_contact->{'alias'});
        }
        
        delete($cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'usergroups'});
        if(defined($obj_1x_contact->{'contactgroups'})) {

            # check if there's additive inheritance required, and save a flag
            if ($obj_1x_contact->{'contactgroups'} =~ /^\+/) {
                $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'__I2_CONVERT_UG_ADD'} = 1;
                $obj_1x_contact->{'contactgroups'} =~ s/^\+//;
            }

            push @{$cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'usergroups'}}, Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_contact->{'contactgroups'}, ',', 1);
            #print "DEBUG: usergroups " . join (" ", @{$cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'usergroups'}});
        }


        # we need to rebuild that notification logic entirely for 2.x
        # do that later when all objects are processed and prepared (all relations?)
        #say Dumper($notification_commands_2x);
        $cfg_obj_2x->{'user'}->{$contact_obj_1x_key}->{'__I2CONVERT_NOTIFICATION_COMMANDS'} = $notification_commands_2x;
    }

    SKIP_CONTACTS:

    ######################################
    # GROUPS 
    ######################################

    if (!@$cfg_obj_1x{'hostgroup'}) {
        goto SKIP_HOSTGROUPS;
    }

    # host->hostgroups and hostgroup-members relinked together 
    foreach my $hostgroup_obj_1x_key (keys %{@$cfg_obj_1x{'hostgroup'}}) {
        my $obj_1x_hostgroup = @$cfg_obj_1x{'hostgroup'}->{$hostgroup_obj_1x_key};
        # clone it into our hash
        $cfg_obj_2x->{'hostgroup'}->{$hostgroup_obj_1x_key} = dclone(@$cfg_obj_1x{'hostgroup'}->{$hostgroup_obj_1x_key});

        if(defined($obj_1x_hostgroup->{'alias'})) {
            $cfg_obj_2x->{'hostgroup'}->{$hostgroup_obj_1x_key}->{'display_name'} = Icinga2::Utils::escape_str($obj_1x_hostgroup->{'alias'});
        }

        ####################################################
        # check if host_groupname exists, if not, try to copy it from 'name' (no template inheritance possible in groups!)
        ####################################################
        if(!defined($obj_1x_hostgroup->{'hostgroup_name'})) {
            if(defined($obj_1x_hostgroup->{'name'})) {
                $cfg_obj_2x->{'hostgroup'}->{$hostgroup_obj_1x_key}->{'hostgroup_name'} = $obj_1x_hostgroup->{'name'};
            }
        }

        ####################################################
        # check if there are members defined, we must re-link them in their host object again
        ####################################################
        if(defined($obj_1x_hostgroup->{'members'})) {

            my @hg_members = ();
            # check if members is a wildcard, or a possible comma seperated list
            # using object tricks - http://docs.icinga.org/latest/en/objecttricks.html#objecttricks-service
            # XXX better create a master template where all hosts inherit from, and use additive hostgroups attribute
            if ($obj_1x_hostgroup->{'members'} =~ /^\*$/) {
                @hg_members = obj_1x_get_all_hostnames_arr($cfg_obj_2x);
            } else {
                @hg_members = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_hostgroup->{'members'}, ',', 1);
            }

            foreach my $hg_member (@hg_members) {
                my $obj_2x_hg_member = obj_get_host_obj_by_host_name($cfg_obj_2x, $hg_member);           
                #print "DEBUG: $hg_member found.\n";
                push @{$obj_2x_hg_member->{'hostgroups'}}, $obj_1x_hostgroup->{'hostgroup_name'};
            }
        }
    }

    SKIP_HOSTGROUPS:

    if (!@$cfg_obj_1x{'servicegroup'}) {
        goto SKIP_SERVICEGROUPS;
    }

    # service->servicegroups and servicegroup->members relinked together
    foreach my $servicegroup_obj_1x_key (keys %{@$cfg_obj_1x{'servicegroup'}}) {
        my $obj_1x_servicegroup = @$cfg_obj_1x{'servicegroup'}->{$servicegroup_obj_1x_key};
        # clone it into our hash
        $cfg_obj_2x->{'servicegroup'}->{$servicegroup_obj_1x_key} = dclone(@$cfg_obj_1x{'servicegroup'}->{$servicegroup_obj_1x_key});

        if(defined($obj_1x_servicegroup->{'alias'})) {
            $cfg_obj_2x->{'servicegroup'}->{$servicegroup_obj_1x_key}->{'display_name'} = Icinga2::Utils::escape_str($obj_1x_servicegroup->{'alias'});
        }

        ####################################################
        # check if service_groupname exists, if not, try to copy it from 'name' (no template inheritance possible in groups!)
        ####################################################
        if(!defined($obj_1x_servicegroup->{'servicegroup_name'})) {
            if(defined($obj_1x_servicegroup->{'name'})) {
                $cfg_obj_2x->{'servicegroup'}->{$servicegroup_obj_1x_key}->{'servicegroup_name'} = $obj_1x_servicegroup->{'name'};
            }
        }

        ####################################################
        # check if there are members defined, we must re-link them in their service object again
        ####################################################
        if(defined($obj_1x_servicegroup->{'members'})) {

            # host1,svc1,host2,svc2 is just an insane way of parsing stuff - do NOT sort here.
            my @sg_members = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_servicegroup->{'members'}, ',', 0);
            # just some safety for debugging
            if(@sg_members % 2 != 0) {
                Icinga2::Utils::debug("servicegroup $obj_1x_servicegroup->{'servicegroup_name'} members list not even: $obj_1x_servicegroup->{'members'}");
            }
            #print "DEBUG: $obj_1x_servicegroup->{'servicegroup_name'}: @sg_members\n";
            my $obj_2x_sg_member;
            while (scalar(@sg_members) > 0) {
                my $sg_member_host = shift(@sg_members);
                my $sg_member_service = shift(@sg_members);
                #print "DEBUG: Looking for $obj_1x_servicegroup->{'servicegroup_name'}: $sg_member_host/$sg_member_service\n";
                # since we require the previously looked up unique hostname/service_description, we use the new values in 2x objects (__I2CONVERT_...)
                my $obj_2x_sg_member = obj_get_service_obj_by_host_name_service_description($cfg_obj_2x, "__I2CONVERT_SERVICE_HOSTNAME", "__I2CONVERT_SERVICEDESCRIPTION", $sg_member_host, $sg_member_service);
                #print "DEBUG: $sg_member_host,$sg_member_service found.\n";
                push @{$obj_2x_sg_member->{'servicegroups'}}, $obj_1x_servicegroup->{'servicegroup_name'};
            }
            #say Dumper($cfg_obj_2x->{'service'});
        }
    }

    SKIP_SERVICEGROUPS:

    if (!@$cfg_obj_1x{'contactgroup'}) {
        goto SKIP_CONTACTGROUPS;
    }

    # contact->contactgroups and contactgroup->members relinked together
    foreach my $contactgroup_obj_1x_key (keys %{@$cfg_obj_1x{'contactgroup'}}) {
        my $obj_1x_contactgroup = @$cfg_obj_1x{'contactgroup'}->{$contactgroup_obj_1x_key};
        # clone it into our hash
        $cfg_obj_2x->{'usergroup'}->{$contactgroup_obj_1x_key} = dclone(@$cfg_obj_1x{'contactgroup'}->{$contactgroup_obj_1x_key});
        $cfg_obj_2x->{'usergroup'}->{$contactgroup_obj_1x_key}->{'__I2CONVERT_TYPE'} = "usergroup";

        ####################################################
        # migrate renamed attributes
        ####################################################
        $cfg_obj_2x->{'usergroup'}->{$contactgroup_obj_1x_key}->{'usergroup_name'} = $obj_1x_contactgroup->{'contactgroup_name'};

        if(defined($obj_1x_contactgroup->{'alias'})) {
            $cfg_obj_2x->{'usergroup'}->{$contactgroup_obj_1x_key}->{'display_name'} = Icinga2::Utils::escape_str($obj_1x_contactgroup->{'alias'});
        }

        ####################################################
        # check if contact_groupname exists, if not, try to copy it from 'name' (no template inheritance possible in groups!)
        ####################################################
        if(!defined($obj_1x_contactgroup->{'contactgroup_name'})) {
            if(defined($obj_1x_contactgroup->{'name'})) {
                $cfg_obj_2x->{'usergroup'}->{$contactgroup_obj_1x_key}->{'usergroup_name'} = $obj_1x_contactgroup->{'name'};
            }
        }

        ####################################################
        # check if there are members defined, we must re-link them in their host object again
        ####################################################
        if(defined($obj_1x_contactgroup->{'members'})) {
            my @cg_members = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_contactgroup->{'members'}, ',', 1);
            foreach my $cg_member (@cg_members) {
                my $obj_2x_cg_member = obj_get_contact_obj_by_contact_name($cfg_obj_2x, "user", "user_name", $cg_member);           
                #print "DEBUG: $cg_member found.\n";
                push @{$obj_2x_cg_member->{'usergroups'}}, $obj_1x_contactgroup->{'contactgroup_name'};
            }
        }
    }

    SKIP_CONTACTGROUPS:

    ######################################
    # TIMEPERIODS
    ######################################

    if (!@$cfg_obj_1x{'timeperiod'}) {
        goto SKIP_TIMEPERIODS;
    }

    foreach my $timeperiod_obj_1x_key (keys %{@$cfg_obj_1x{'timeperiod'}}) {
        my $obj_1x_timeperiod = @$cfg_obj_1x{'timeperiod'}->{$timeperiod_obj_1x_key};
        # clone it into our hash
        $cfg_obj_2x->{'timeperiod'}->{$timeperiod_obj_1x_key} = dclone(@$cfg_obj_1x{'timeperiod'}->{$timeperiod_obj_1x_key});

        ####################################################
        # add dependency to ITL template to objects
        ####################################################
        if(defined($icinga2_cfg->{'itl'}->{'timeperiod-template'}) && $icinga2_cfg->{'itl'}->{'timeperiod-template'} ne "") {
            push @{$cfg_obj_2x->{'timeperiod'}->{$timeperiod_obj_1x_key}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'timeperiod-template'};
            $cfg_obj_2x->{'timeperiod'}->{$timeperiod_obj_1x_key}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
        }

        ####################################################
        # display_name -> alias mapping
        ####################################################
        # if there was a timeperiod alias defined, make this the primary display_name for 2x
        if(defined($obj_1x_timeperiod->{'alias'})) {
            $cfg_obj_2x->{'timeperiod'}->{$timeperiod_obj_1x_key}->{'display_name'} = Icinga2::Utils::escape_str($obj_1x_timeperiod->{'alias'});
            delete($cfg_obj_2x->{'timeperiod'}->{$timeperiod_obj_1x_key}->{'alias'});
        }

    }

    SKIP_TIMEPERIODS:

    ######################################
    # DEPENDENCIES
    ######################################

    if (!@$cfg_obj_1x{'hostdependency'}) {
        goto SKIP_HOSTDEPS;
    }

    foreach my $hostdependency_obj_1x_key (keys %{@$cfg_obj_1x{'hostdependency'}}) {
        my $obj_1x_hostdependency = @$cfg_obj_1x{'hostdependency'}->{$hostdependency_obj_1x_key};
        # clone it into our hash
        $cfg_obj_2x->{'hostdependency'}->{$hostdependency_obj_1x_key} = dclone(@$cfg_obj_1x{'hostdependency'}->{$hostdependency_obj_1x_key});

        # 1. the single host_name entries
        # host_name is the master host (comma seperated list)
        # dependent_host_name is the child host (comma seperated list)
        my @master_host_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_hostdependency->{'host_name'}, ',', 1);
        my @child_host_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_hostdependency->{'dependent_host_name'}, ',', 1);

        # go through all child hosts, and push to the parents array
        foreach my $child_host_name (@child_host_names) {
            my $child_host_obj = obj_get_host_obj_by_host_name($cfg_obj_2x, $child_host_name);

            push @{$child_host_obj->{'__I2CONVERT_PARENT_HOSTNAMES'}}, @master_host_names;
        }

        # 2. the infamous group logic - let's loop because we're cool
        my @master_hostgroup_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_hostdependency->{'hostgroup_name'}, ',', 1);
        my @child_hostgroup_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_hostdependency->{'dependent_hostgroup_name'}, ',', 1);

        my @all_master_hostgroup_hostnames = ();

        # get all hosts as array for the master host groups
        foreach my $master_hostgroup_name (@master_hostgroup_names) {
            my @host_master_hostgroup_hostnames = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $master_hostgroup_name);
            push @all_master_hostgroup_hostnames, @host_master_hostgroup_hostnames;        
        }

        # go through all child hostgroups and fetch their host objects, setting 
        foreach my $child_hostgroup_name (@child_hostgroup_names) {
            my @host_child_hostgroup_hostnames = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $child_hostgroup_name);
            foreach my $host_child_hostgroup_hostname (@host_child_hostgroup_hostnames) {
                my $child_host_obj = obj_get_host_obj_by_host_name($cfg_obj_2x, $host_child_hostgroup_hostname);
                push @{$child_host_obj->{'__I2CONVERT_PARENT_HOSTNAMES'}}, @all_master_hostgroup_hostnames;
            }
        }
    }

    # XXX ugly but works
    SKIP_HOSTDEPS:

    if (!@$cfg_obj_1x{'servicedependency'}) {
        goto SKIP_SVCDEPS;
    }

    foreach my $servicedependency_obj_1x_key (keys %{@$cfg_obj_1x{'servicedependency'}}) {
        my $obj_1x_servicedependency = @$cfg_obj_1x{'servicedependency'}->{$servicedependency_obj_1x_key};
        # clone it into our hash
        $cfg_obj_2x->{'servicedependency'}->{$servicedependency_obj_1x_key} = dclone(@$cfg_obj_1x{'servicedependency'}->{$servicedependency_obj_1x_key});

        # 1. the single host_name / service_description entries
        # service_description is a string, while the host_name directive is still a comma seperated list
        my @master_host_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_servicedependency->{'host_name'}, ',', 1);
        my @child_host_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_servicedependency->{'dependent_host_name'}, ',', 1);

        my $master_service_description = $obj_1x_servicedependency->{'service_description'};
        my $child_service_description = $obj_1x_servicedependency->{'dependent_service_description'};

        # XXX object tricks allow more here
        # - comma seperated list of service descriptions on a single *host_name
        # - wildcard * for all services on a single *host_name

        # go through all child hosts, and get the service object by host_name and our single service_description
        foreach my $child_host_name (@child_host_names) {
            my $child_service_obj = obj_get_service_obj_by_host_name_service_description($cfg_obj_2x, "__I2CONVERT_SERVICE_HOSTNAME", "__I2CONVERT_SERVICEDESCRIPTION", $child_host_name, $child_service_description); 
            # stash all master dependencies onto the child service
            foreach my $master_host_name (@master_host_names) {
                # use some calculated unique key here (no, i will not split the string later! we are perl, we can do hashes)
                my $master_key = $master_host_name."-".$master_service_description;
                $child_service_obj->{'__I2CONVERT_PARENT_SERVICES'}->{$master_key}->{'host'} = $master_host_name;
                $child_service_obj->{'__I2CONVERT_PARENT_SERVICES'}->{$master_key}->{'service'} = $master_service_description;
            }
        }

        # 2. the infamous group logic - but only for hostgroups here
        my @master_hostgroup_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_servicedependency->{'hostgroup_name'}, ',', 1);
        my @child_hostgroup_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_servicedependency->{'dependent_hostgroup_name'}, ',', 1);

        my @all_master_hostgroup_hostnames = ();

        # get all hosts as array for the master host groups
        foreach my $master_hostgroup_name (@master_hostgroup_names) {
            my @host_master_hostgroup_hostnames = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $master_hostgroup_name);
            push @all_master_hostgroup_hostnames, @host_master_hostgroup_hostnames;
        }

        #say Dumper($obj_1x_servicedependency);
        #say " DEBUG: all master hg hostnames: ".Dumper(@all_master_hostgroup_hostnames);

        # go through all child hostgroups and fetch their host objects, setting 
        foreach my $child_hostgroup_name (@child_hostgroup_names) {
            my @host_child_hostgroup_hostnames = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $child_hostgroup_name); # child hostgroup members
            #say " DEBUG: child hg hostnames: ".Dumper(@host_child_hostgroup_hostnames);

            foreach my $host_child_hostgroup_hostname (@host_child_hostgroup_hostnames) {
                my $child_service_obj = obj_get_service_obj_by_host_name_service_description($cfg_obj_2x, "__I2CONVERT_SERVICE_HOSTNAME", "__I2CONVERT_SERVICEDESCRIPTION", $host_child_hostgroup_hostname, $child_service_description);

                # now loop through all master hostgroups and get their hosts
                foreach my $master_hostgroup_name (@master_hostgroup_names) {
                    my @host_master_hostgroup_names = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $master_hostgroup_name); # master hostgroup members
                    foreach my $host_master_hostgroup_hostname (@host_master_hostgroup_names) {

                        # use some calculated unique key here (no, i will not split the string later! we are perl, we can do hashes)
                        my $master_key = $host_master_hostgroup_hostname."-".$master_service_description;
                        $child_service_obj->{'__I2CONVERT_PARENT_SERVICES'}->{$master_key}->{'host'} = $host_master_hostgroup_hostname; # XXX 5th foreach. awesome!
                        $child_service_obj->{'__I2CONVERT_PARENT_SERVICES'}->{$master_key}->{'service'} = $master_service_description;
                    }
                }

            }
        }
    }

    # XXX ugly but works
    SKIP_SVCDEPS:

    ######################################
    # SERVICE->HG<-HOSTMEMBERS MAGIC
    # we've skipped services without
    # host_name before, now deal with them
    # hostgroups have been prepared with
    # all their members too (!!)
    # we're working on 2.x objects now
    ######################################

    # get the max key for hosts (required for adding more)
    my $obj_2x_hosts_cnt = (reverse sort {$a <=> $b} (keys %{@$cfg_obj_2x{'host'}}))[0];
    #print "FOO: $obj_2x_hosts_cnt\n";

    my $obj_2x_services_hg = {};

    # filter all services with a hostgroup_name into smaller list
    foreach my $service_obj_2x_key (keys %{@$cfg_obj_2x{'service'}}) {

        my $obj_2x_service = @$cfg_obj_2x{'service'}->{$service_obj_2x_key};

        #print "DEBUG: now checking $obj_2x_service->{'service_description'}...\n";
        # skip all services which already got a host_name? which one wins here? XXX

        # skip all services without hostgroup_name
        next if(!defined($obj_2x_service->{'hostgroup_name'}));

        # XXX object tricks allow to use a comma seperated list of hostgroup_names!
        # http://docs.icinga.org/latest/en/objecttricks.html
        my @hostgroup_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_2x_service->{'hostgroup_name'}, ',', 1);

        foreach my $hostgroup_name (@hostgroup_names) {
            # we need to save all services first, but our new key is the hostgroupname
            # so that we can create multiple services for a single hosthg template later on
            push @{$obj_2x_services_hg->{$hostgroup_name}}, $service_obj_2x_key;
        }
    }

    # now loop over all hostgroups with service relations
    foreach my $service_hg_obj_2x_key (keys %{$obj_2x_services_hg}) {

        #say Dumper($obj_2x_services_hg);

        # get the stored unique key to our services
        my $hg_name = $service_hg_obj_2x_key;
        my @service_keys = @{$obj_2x_services_hg->{$hg_name}};

        #print "DEBUG: Looking for $hg_name ...\n";

        my $obj_2x_hostgroup = obj_get_hostgroup_obj_by_hostgroup_name($cfg_obj_2x, $hg_name);

        if(!defined($obj_2x_hostgroup)) {
            # no hostgroup defined?
        }

        # we now need all host names for this hostgroup name, as an array

        my @service_hostgroup_hostnames = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $hg_name);

        if(@service_hostgroup_hostnames == 0) {
            # no members, so service cannot be linked. log a warning XXX
            #print " DEBUG: no members found, skipping $hg_name\n";
            next;
        }
        
        # we've got:
        # * n services linked to hostgroups, 
        # * a hostgroup 
        # * an array of hosts as hostgroup members
        # we'll create: 
        # * n service templates, 
        # * a hg-host template referencing the service templates, 
        # * host objects inheriting from it
        #
        my $svc_count = 0;

        # create a host template with hgname-group
        my $obj_2x_host_template;
        $obj_2x_host_template->{'__I2CONVERT_IS_TEMPLATE'} = 1;
        $obj_2x_host_template->{'__I2CONVERT_TEMPLATE_NAME'} = $obj_2x_hostgroup->{'hostgroup_name'}."-group"; # XXX hardcode it for now

        # loop through all services and attach them to the host template
        foreach my $service_obj_2x_key_val (@service_keys) {

            #print "DEBUG: Working on $service_obj_2x_key_val ...\n";
            # get the service object by key
            my $obj_2x_service = @$cfg_obj_2x{'service'}->{$service_obj_2x_key_val};

            # set the service as template.
            $obj_2x_service->{'__I2CONVERT_IS_TEMPLATE'} = 1;
            $obj_2x_service->{'__I2CONVERT_TEMPLATE_NAME'} = $obj_2x_service->{'service_description'}."-group-".$svc_count; # XXX hardcode it for now
        
            # create a dummy service inheriting the service template
            my $obj_2x_service_inherit;
            $obj_2x_service_inherit->{__I2CONVERT_USES_TEMPLATE} = 1;
            push @{$obj_2x_service_inherit->{'__I2CONVERT_TEMPLATE_NAMES'}}, $obj_2x_service->{'__I2CONVERT_TEMPLATE_NAME'};
            $obj_2x_service_inherit->{'service_description'} = $obj_2x_service->{'service_description'};
            $obj_2x_service_inherit->{'__I2CONVERT_SERVICEDESCRIPTION'} = $obj_2x_service->{'service_description'};

            # link the service inherit to the host template
            $obj_2x_host_template->{'SERVICE'}->{$svc_count} = $obj_2x_service_inherit;

            $svc_count++;
        }

        # all host objects on the hostgroup members will get the host hg template name pushed into their array
        foreach my $hostgroup_member_host_name (@service_hostgroup_hostnames) {
            # get the host obj
            my $obj_2x_host = obj_get_host_obj_by_host_name($cfg_obj_2x, $hostgroup_member_host_name); # this is a reference in memory, not a copy!
            
            # push the template used
            # (override __I2CONVERT_USES_TEMPLATE too)
            $obj_2x_host->{__I2CONVERT_USES_TEMPLATE} = 1;
            push @{$obj_2x_host->{'__I2CONVERT_TEMPLATE_NAMES'}}, $obj_2x_host_template->{'__I2CONVERT_TEMPLATE_NAME'};

        }

        # push back the newly created host template (incl the service inherit below SERVICE) to the objects 2.x hive
        #say Dumper($obj_2x_host_template);

        $obj_2x_hosts_cnt++;
        #print "adding new host at key " . $obj_2x_hosts_cnt . "\n";
        $cfg_obj_2x->{'host'}->{$obj_2x_hosts_cnt} = $obj_2x_host_template;

    }

    ######################################
    # NEW: NOTIFICATION MAPPING 
    # old:  contact->notification_commands->commands
    #       contact->email/etc
    #       host/service -> contact
    # new:  notification->notification_command
    #       user->mail/etc
    #       host/service->notifications[type]->notification_templates,users
    ######################################
    my $notification_obj_cnt = 0;
    my $obj_notification_cnt = 0;
    # add a dummy value so that we can check against it
    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_NAME'} = '__I2CONVERT_NOTIFICATION_DUMMY';

    # go through all users and build notifications based on the notification_command
    foreach my $user_obj_2x_key (keys %{@$cfg_obj_2x{'user'}}) {
        
        my $obj_2x_user = @$cfg_obj_2x{'user'}->{$user_obj_2x_key};

        my $user_notification;
        ####################################################
        # get all notification_commands, and create new notification templates
        ####################################################
        my $notification_commands = $obj_2x_user->{'__I2CONVERT_NOTIFICATION_COMMANDS'};
        #say Dumper($notification_commands);

        foreach my $notification_command_type (keys %{$notification_commands}) {
            foreach my $notification_command_name (keys %{$notification_commands->{$notification_command_type}}) {
                my $notification_command_line = $notification_commands->{$notification_command_type}->{$notification_command_name};
                #print "type: $notification_command_type name: $notification_command_name line: $notification_command_line\n";

                my $notification_command_name_2x = $notification_command_type."-".$notification_command_name;

                my $notification_name_2x = $notification_command_name_2x.$obj_notification_cnt;
                $obj_notification_cnt++;

                # save a relation to this user and which notification templates are now linked ( ["name"] = { templates = "template" } )
                # we'll use that later on when processing hosts/services and linking to users and notifications
                $user_notification->{$notification_name_2x}->{'name'} = $notification_name_2x;
                
                push @{$user_notification->{$notification_name_2x}->{'templates'}}, $notification_command_name_2x;
                push @{$user_notification->{$notification_name_2x}->{'users'}}, $obj_2x_user->{'user_name'};

                # save the type for later objects (host or service)
                $user_notification->{$notification_name_2x}->{'type'} = $notification_command_type;

                # XXX do not add duplicate notifications, they must remain unique by their notification_command origin!
                #say Dumper("checking existing $notification_command_name_2x");
                #say Dumper($user_notification);
                if (obj_2x_notification_exists($cfg_obj_2x, $notification_command_name_2x) == 1) {
                    #say Dumper("already existing $notification_command_name_2x");
                    next;
                }

                next if (!defined($notification_command_name_2x));

                # create a new NotificationCommand 2x object with the original name
                $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_TYPE'} = 'Notification';
                $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = $notification_command_name;
                $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_LINE'} = $notification_command_line;

                # use the ITL plugin notification command template
                if(defined($icinga2_cfg->{'itl'}->{'notificationcommand-template'}) && $icinga2_cfg->{'itl'}->{'notificationcommand-template'} ne "") {
                    push @{$cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notificationcommand-template'};
                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                }

                # the check command name of 1.x is still the unique command object name, so we just keep it
                # in __I2CONVERT_NOTIFICATION_COMMAND

                # our global PK
                $command_obj_cnt++;

                # create a new notification template object
                $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TEMPLATE_NAME'} = $notification_command_name_2x; 
                $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_OBJECT_NAME'} = $notification_command_name_2x; 
                $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_COMMAND'} = $notification_command_name;
                $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_IS_TEMPLATE'} = 1; # this is a template, used in hosts/services then

                # more reference
                $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'users'} = $user_notification->{$notification_name_2x}->{'users'};

                # add dependency to ITL template to objects
                if(defined($icinga2_cfg->{'itl'}->{'notification-template'}) && $icinga2_cfg->{'itl'}->{'notification-template'} ne "") {
                    @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}} = ();
                    push @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notification-template'};
                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1; # we now use a template, otherwise it won't be dumped
                }

                $notification_obj_cnt++;
            }
        }
        $cfg_obj_2x->{'user'}->{$user_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'} = $user_notification;

        #say Dumper($cfg_obj_2x->{'user'}->{$user_obj_2x_key});
    }

    # go through all hosts/services, and add notifications based on the users
    # XXX hosts - do we notify on hosts?
    foreach my $host_obj_2x_key (keys %{@$cfg_obj_2x{'host'}}) {

        my $obj_2x_host = @$cfg_obj_2x{'host'}->{$host_obj_2x_key};
        # make sure there are none
        delete($cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'});
        @{$cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'}} = ();

        # convert users and usergroupmembers into a unique list of users
        my @users = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_2x_host->{'contacts'}, ',', 1);
        my @usergroups = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_2x_host->{'contact_groups'}, ',', 1);

        # get all members of the usergroups
        foreach my $usergroup (@usergroups) {
            my @users_ug = obj_get_usernames_arr_by_usergroup_name($cfg_obj_2x, $usergroup);
            push @users, @users_ug;
        }
        # create a unique array of users (XXX important! XXX)
        my @uniq_users = Icinga2::Utils::uniq(@users);

        # now loop and fetch objects, and their needed notification values as array
        # (prepared above - look for $user_notification->{$notification_command_name_2x}...) 
        foreach my $uniq_user (@uniq_users) {
            my $obj_2x_user = obj_get_user_obj_by_user_name($cfg_obj_2x, $uniq_user);
            push @{$cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'}}, $obj_2x_user->{'__I2CONVERT_NOTIFICATIONS'};
            # we'll add a reference to all notifications here. decide on dump which object type is given, and dump only those notifications!
        }
        #say Dumper($obj_2x_service);

    }

    # XXX services
    foreach my $service_obj_2x_key (keys %{@$cfg_obj_2x{'service'}}) {
        
        my $obj_2x_service = @$cfg_obj_2x{'service'}->{$service_obj_2x_key};
        # make sure there are none
        delete($cfg_obj_2x->{'service'}->{$service_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'});
        @{$cfg_obj_2x->{'service'}->{$service_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'}} = ();

        # convert users and usergroupmembers into a unique list of users
        my @users = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_2x_service->{'contacts'}, ',', 1);
        my @usergroups = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_2x_service->{'contact_groups'}, ',', 1);
        
        # get all members of the usergroups
        foreach my $usergroup (@usergroups) {
            my @users_ug = obj_get_usernames_arr_by_usergroup_name($cfg_obj_2x, $usergroup);
            push @users, @users_ug;
        }
        # create a unique array of users (XXX important! XXX)
        my @uniq_users = Icinga2::Utils::uniq(@users);

        # now loop and fetch objects, and their needed notification values as array
        # (prepared above - look for $user_notification->{$notification_command_name_2x}...) 
        foreach my $uniq_user (@uniq_users) {
            my $obj_2x_user = obj_get_user_obj_by_user_name($cfg_obj_2x, $uniq_user);
            push @{$cfg_obj_2x->{'service'}->{$service_obj_2x_key}->{'__I2CONVERT_NOTIFICATIONS'}}, $obj_2x_user->{'__I2CONVERT_NOTIFICATIONS'};      
            # we'll add a reference to all notifications here. decide on dump which object type is given, and dump only those notifications!
        }
        #say Dumper($obj_2x_service);

    }
    #exit(0);

    ######################################
    # NEW: ESCALATION TO NOTIFICATION
    ######################################

    my $obj_notification_escal_cnt = 0;
    if (!@$cfg_obj_1x{'serviceescalation'}) {
        goto SKIP_SVCESCAL;
    }
    foreach my $serviceescalation_obj_1x_key (keys %{@$cfg_obj_1x{'serviceescalation'}}) {
        my $obj_1x_serviceescalation = @$cfg_obj_1x{'serviceescalation'}->{$serviceescalation_obj_1x_key};

        ######################################
        # create a unique users list
        ######################################
        # we need to get all notification_commands and create a notification escalation item from that source
        my $notification_prefix = 'serviceescalation';

        # convert users and usergroupmembers into a unique list of users
        my @users = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_serviceescalation->{'contacts'}, ',', 1);
        my @usergroups = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_serviceescalation->{'contact_groups'}, ',', 1);

        # get all members of the usergroups
        foreach my $usergroup (@usergroups) {
            my @users_ug = obj_get_usernames_arr_by_usergroup_name($cfg_obj_2x, $usergroup);
            push @users, @users_ug;
        }
        # create a unique array of users (XXX important! XXX)
        my @uniq_users = Icinga2::Utils::uniq(@users);

        ######################################
        # link users to this notification
        ######################################
        foreach my $uniq_user (@uniq_users) {
            my $obj_2x_user = obj_get_user_obj_by_user_name($cfg_obj_2x, $uniq_user);
            #say Dumper($obj_2x_user);

            my $user_notification;

            my $notification_commands = $obj_2x_user->{'__I2CONVERT_NOTIFICATION_COMMANDS'};
            #say Dumper($notification_commands);

            foreach my $notification_command_type (keys %{$notification_commands}) {
                foreach my $notification_command_name (keys %{$notification_commands->{$notification_command_type}}) {
                    my $notification_command_line = $notification_commands->{$notification_command_type}->{$notification_command_name};
                    #print "type: $notification_command_type name: $notification_command_name line: $notification_command_line\n";

                    my $notification_command_name_2x = $notification_prefix."-".$notification_command_type."-".$notification_command_name;

                    my $notification_name_2x = $notification_command_name_2x.$obj_notification_escal_cnt;
                    $obj_notification_escal_cnt++;

                    # save a relation to this user and which notification templates are now linked ( ["name"] = { templates = "template" } )
                    # we'll use that later on when processing hosts/services and linking to users and notifications
                    $user_notification->{$notification_name_2x}->{'name'} = $notification_name_2x;

                    push @{$user_notification->{$notification_name_2x}->{'templates'}}, $notification_command_name_2x;
                    push @{$user_notification->{$notification_name_2x}->{'users'}}, $obj_2x_user->{'user_name'};

                    # save the type for later objects (host or service)
                    $user_notification->{$notification_name_2x}->{'type'} = $notification_command_type;

                    ######################################
                    # create a unique services list, and
                    # link that to the notification
                    # - host_name/service_description
                    # - hostgroup_name/service_description
                    # - servicegroup_name
                    ######################################
                    my $notification_interval = 60; # assume some default if everything goes wrong

                    ######################################
                    # get the obj by host_name/service_description
                    ######################################
                    if (defined($obj_1x_serviceescalation->{'host_name'}) && defined($obj_1x_serviceescalation->{'service_description'})) {
                        my $serviceescalation_service_obj = obj_get_service_obj_by_host_name_service_description($cfg_obj_2x, "__I2CONVERT_SERVICE_HOSTNAME", "__I2CONVERT_SERVICEDESCRIPTION", $obj_1x_serviceescalation->{'host_name'}, $obj_1x_serviceescalation->{'service_description'});
                        push @{$serviceescalation_service_obj->{'__I2CONVERT_NOTIFICATIONS'}}, $user_notification;

                        #say Dumper($serviceescalation_service_obj);
                        # we need to calculate begin/end based on service->notification_interval
                        # if notification_interval is not defined, we need to look it up in the template tree!
                        if (defined($serviceescalation_service_obj->{'notification_interval'})) {
                            $notification_interval = $serviceescalation_service_obj->{'notification_interval'};
                        } else {
                            $notification_interval = obj_1x_get_service_attr($cfg_obj_1x, $serviceescalation_service_obj, $serviceescalation_service_obj->{'__I2CONVERT_SERVICE_HOSTNAME'}, 'notification_interval');
                        }
                        #say Dumper($notification_interval);
                        $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'}->{'begin'} = $obj_1x_serviceescalation->{'first_notification'} * $notification_interval;
                        $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'}->{'end'} = $obj_1x_serviceescalation->{'last_notification'} * $notification_interval;

                        # save a reference to more infos
                        $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TIMES'} = $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'};
                        #say Dumper($obj_1x_serviceescalation);
                        #say Dumper($user_notification);

                        ######################################
                        # now ADD the new escalation notification
                        ######################################

                        # XXX do not add duplicate notifications, they must remain unique by their notification_command origin!
                        next if (obj_2x_notification_exists($cfg_obj_2x, $notification_command_name_2x) == 1);

                        next if (!defined($notification_command_name_2x));

                        # create a new NotificationCommand 2x object with the original name
                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_TYPE'} = 'Notification';
                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = $notification_command_name;
                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_LINE'} = $notification_command_line;

                        # use the ITL plugin notification command template
                        if(defined($icinga2_cfg->{'itl'}->{'notificationcommand-template'}) && $icinga2_cfg->{'itl'}->{'notificationcommand-template'} ne "") {
                            push @{$cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notificationcommand-template'};
                            $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                        }

                        # the check command name of 1.x is still the unique command object name, so we just keep it
                        # in __I2CONVERT_NOTIFICATION_COMMAND

                        # our global PK
                        $command_obj_cnt++;

                        # create a new notification template object
                        $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TEMPLATE_NAME'} = $notification_command_name_2x; 
                        $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_OBJECT_NAME'} = $notification_command_name_2x; 
                        $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_COMMAND'} = $notification_command_name;
                        $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_IS_TEMPLATE'} = 1; # this is a template, used in hosts/services then

                        # more reference
                        $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'users'} = $user_notification->{$notification_name_2x}->{'users'};

                        # add dependency to ITL template to objects
                        if(defined($icinga2_cfg->{'itl'}->{'notification-template'}) && $icinga2_cfg->{'itl'}->{'notification-template'} ne "") {
                            @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}} = ();
                            push @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notification-template'};
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1; # we now use a template, otherwise it won't be dumped
                        }


                        #say Dumper($cfg_obj_2x->{'notification'}->{$notification_obj_cnt});
                        # our PK
                        $notification_obj_cnt++;

                    }

                    ######################################
                    # get all hosts in hostgroup, with service_description
                    ######################################
                    if (defined($obj_1x_serviceescalation->{'hostgroup_name'}) && defined($obj_1x_serviceescalation->{'service_description'})) {
                        my @serviceescalation_hostgroup_names = Icinga2::Utils::str2arr_by_delim_without_excludes($obj_1x_serviceescalation->{'hostgroup_name'}, ',', 1);

                        foreach my $serviceescalation_hostgroup_name (@serviceescalation_hostgroup_names) {
                            # get hg members
                            my @serviceescalation_hostgroup_hostnames = obj_get_hostnames_arr_by_hostgroup_name($cfg_obj_2x, $serviceescalation_hostgroup_name);

                            foreach my $serviceescalation_hostgroup_hostname (@serviceescalation_hostgroup_hostnames) {

                                if (defined($obj_1x_serviceescalation->{'service_description'})) {
                                    my $serviceescalation_service_obj = obj_get_service_obj_by_host_name_service_description($cfg_obj_2x, "__I2CONVERT_SERVICE_HOSTNAME", "__I2CONVERT_SERVICEDESCRIPTION", $obj_1x_serviceescalation->{'host_name'}, $obj_1x_serviceescalation->{'service_description'});

                                    push @{$serviceescalation_service_obj->{'__I2CONVERT_NOTIFICATIONS'}}, $user_notification;

                                    # we need to calculate begin/end based on service->notification_interval
                                    # if notification_interval is not defined, we need to look it up in the template tree!
                                    if (defined($serviceescalation_service_obj->{'notification_interval'})) {
                                        $notification_interval = $serviceescalation_service_obj->{'notification_interval'};
                                    } else {
                                        $notification_interval = obj_1x_get_service_attr($cfg_obj_1x, $serviceescalation_service_obj, $serviceescalation_service_obj->{'__I2CONVERT_SERVICE_HOSTNAME'}, 'notification_interval');
                                    }
                                    $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'}->{'begin'} = $obj_1x_serviceescalation->{'first_notification'} * $notification_interval;
                                    $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'}->{'end'} = $obj_1x_serviceescalation->{'last_notification'} * $notification_interval;
                                    # save a reference to more infos
                                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TIMES'} = $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'};


                                    ######################################
                                    # now ADD the new escalation notification
                                    ######################################

                                    # XXX do not add duplicate notifications, they must remain unique by their notification_command origin!
                                    next if (obj_2x_notification_exists($cfg_obj_2x, $notification_command_name_2x) == 1);

                                    next if (!defined($notification_command_name_2x));

                                    # create a new NotificationCommand 2x object with the original name
                                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_TYPE'} = 'Notification';
                                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = $notification_command_name;
                                    $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_LINE'} = $notification_command_line;

                                    # use the ITL plugin notification command template
                                    if(defined($icinga2_cfg->{'itl'}->{'notificationcommand-template'}) && $icinga2_cfg->{'itl'}->{'notificationcommand-template'} ne "") {
                                        push @{$cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notificationcommand-template'};
                                        $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                                    }

                                    # the check command name of 1.x is still the unique command object name, so we just keep it
                                    # in __I2CONVERT_NOTIFICATION_COMMAND

                                    # our global PK
                                    $command_obj_cnt++;

                                    # create a new notification template object
                                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TEMPLATE_NAME'} = $notification_command_name_2x; 
                                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_OBJECT_NAME'} = $notification_command_name_2x; 
                                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_COMMAND'} = $notification_command_name;
                                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_IS_TEMPLATE'} = 1; # this is a template, used in hosts/services then

                                    # more references (this is why code duplication happens
                                    $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'users'} = $user_notification->{$notification_name_2x}->{'users'};

                                    # add dependency to ITL template to objects
                                    if(defined($icinga2_cfg->{'itl'}->{'notification-template'}) && $icinga2_cfg->{'itl'}->{'notification-template'} ne "") {
                                        @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}} = ();
                                            push @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notification-template'};
                                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1; # we now use a template, otherwise it won't be dumped
                                    }
                                    # our PK
                                    $notification_obj_cnt++;
                                }
                            }
                        }
                    }

                    #say Dumper($cfg_obj_2x->{'notification'}->{$notification_obj_cnt});

                    ######################################
                    # get all hosts and services from servicegroup definition and link them
                    ######################################
                    # XXX FIXME
                    if (defined($obj_1x_serviceescalation->{'servicegroup_name'})) {
                        say Dumper($obj_1x_serviceescalation);
                        my @service_names = obj_2x_get_service_arr_by_servicegroup_name($cfg_obj_2x, $obj_1x_serviceescalation->{'servicegroup_name'});

                        foreach my $serviceescalation_service_name (@service_names) {
                            my $serviceescalation_service_obj = obj_get_service_obj_by_host_name_service_description($cfg_obj_2x, "__I2CONVERT_SERVICE_HOSTNAME", "__I2CONVERT_SERVICEDESCRIPTION", $serviceescalation_service_name->{'__I2CONVERT_SERVICE_HOSTNAME'}, $serviceescalation_service_name->{'__I2CONVERT_SERVICEDESCRIPTION'});

                            #say Dumper($serviceescalation_service_obj);
                            # skip any templates which would create duplicates
                            next if ($serviceescalation_service_obj->{'__I2CONVERT_IS_TEMPLATE'} == 1);
                            say Dumper($serviceescalation_service_name);

                            push @{$serviceescalation_service_obj->{'__I2CONVERT_NOTIFICATIONS'}}, $user_notification;

                            # we need to calculate begin/end based on service->notification_interval
                            # if notification_interval is not defined, we need to look it up in the template tree!
                            if (defined($serviceescalation_service_obj->{'notification_interval'})) {
                                $notification_interval = $serviceescalation_service_obj->{'notification_interval'};
                            } else {
                                $notification_interval = obj_1x_get_service_attr($cfg_obj_1x, $serviceescalation_service_obj, $serviceescalation_service_obj->{'__I2CONVERT_SERVICE_HOSTNAME'}, 'notification_interval');
                            }
                            $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'}->{'begin'} = $obj_1x_serviceescalation->{'first_notification'} * $notification_interval;
                            $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'}->{'end'} = $obj_1x_serviceescalation->{'last_notification'} * $notification_interval;
                            # save a reference to more infos
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TIMES'} = $user_notification->{$notification_name_2x}->{'__I2CONVERT_NOTIFICATION_TIMES'};

                            ######################################
                            # now ADD the new escalation notification
                            ######################################

                            # XXX do not add duplicate notifications, they must remain unique by their notification_command origin!
                            next if (obj_2x_notification_exists($cfg_obj_2x, $notification_command_name_2x) == 1);

                            next if (!defined($notification_command_name_2x));

                            # create a new NotificationCommand 2x object with the original name
                            $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_TYPE'} = 'Notification';
                            $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_NAME'} = $notification_command_name;
                            $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_COMMAND_LINE'} = $notification_command_line;

                            # use the ITL plugin notification command template
                            if(defined($icinga2_cfg->{'itl'}->{'notificationcommand-template'}) && $icinga2_cfg->{'itl'}->{'notificationcommand-template'} ne "") {
                                push @{$cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notificationcommand-template'};
                                $cfg_obj_2x->{'command'}->{$command_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                            }

                            # the check command name of 1.x is still the unique command object name, so we just keep it
                            # in __I2CONVERT_NOTIFICATION_COMMAND

                            # our global PK
                            $command_obj_cnt++;

                            # create a new notification template object
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_TEMPLATE_NAME'} = $notification_command_name_2x; 
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_OBJECT_NAME'} = $notification_command_name_2x; 
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_NOTIFICATION_COMMAND'} = $notification_command_name;
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_IS_TEMPLATE'} = 1; # this is a template, used in hosts/services then

                            # more references (this is why code duplication happens
                            $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'users'} = $user_notification->{$notification_name_2x}->{'users'};

                            # add dependency to ITL template to objects
                            if(defined($icinga2_cfg->{'itl'}->{'notification-template'}) && $icinga2_cfg->{'itl'}->{'notification-template'} ne "") {
                                @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}} = ();
                                push @{$cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $icinga2_cfg->{'itl'}->{'notification-template'};
                                $cfg_obj_2x->{'notification'}->{$notification_obj_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1; # we now use a template, otherwise it won't be dumped
                            }
                            # our PK
                            $notification_obj_cnt++;
                        }
                    }
                }
            }

            $obj_2x_user->{'__I2CONVERT_NOTIFICATIONS'} = $user_notification;
        }
    }

    SKIP_SVCESCAL:



    ######################################
    # HOST->SERVICE MAGIC
    # we need to do it _after_ we've
    # manipulated all service objects!
    ######################################

    # "get all 'host' hashref as array in hashmap, and their keys to access it"    
    foreach my $host_obj_2x_key (keys %{@$cfg_obj_2x{'host'}}) {

        #say Dumper(@$cfg_obj_2x{'host'}->{$host_obj_2x_key});
        my $obj_2x_host = @$cfg_obj_2x{'host'}->{$host_obj_2x_key};

        ####################################################
        # Create Host->Service Relation for later dumping
        # we use the prep'ed 2x service hashref already
        # all attributes _must_have been resolved already!
        ####################################################
        my $obj_2x_host_service_cnt = 0;

        # find all services for this host
        foreach my $service_obj_2x_key (keys %{@$cfg_obj_2x{'service'}}) {

            my $obj_2x_service = @$cfg_obj_2x{'service'}->{$service_obj_2x_key};

            ######################################
            # get host_name/service_desc for obj
            # (prepared in service loop already)
            ######################################
            my $obj_2x_service_host_name = $obj_2x_service->{'__I2CONVERT_SERVICE_HOSTNAME'};
            my $obj_2x_service_service_description = $obj_2x_service->{'__I2CONVERT_SERVICEDESCRIPTION'};

            ######################################
            # skip service templates 
            ######################################
            if ($obj_2x_service->{'__I2CONVERT_IS_TEMPLATE'} == 1) {
                #Icinga2::Utils::debug("WARNING: Skipping service template '$obj_2x_service->{'__I2CONVERT_TEMPLATE_NAMES'}' for linking to host '$obj_2x_host->{'__I2CONVERT_HOSTNAME'}'.");
                next;
            }

            # save it for later
            # XXX if host_name can't be located in the service template tree, check if hostgroup is set somewhere
            # we then need to check if the service -> hostgroup <- hostmember applies (ugly) FIXME

            # XXX if host_name can't be determined, log an error XXX templates MUST be skipped before (they cannot look down, only up in use tree) 
            if (!defined($obj_2x_service_host_name)) {
                #print "ERROR: No host_name for service given " . Dumper($obj_2x_service);
                next;
            }

            ######################################
            # found a host->service relation?
            ######################################
            if ($obj_2x_service_host_name eq $obj_2x_host->{'__I2CONVERT_HOSTNAME'}) {
                #debug("service_description: $obj_2x_service_service_description host_name: $obj_2x_service_host_name");

                # 1. generate template name "host-service"
                my $service_template_name = $obj_2x_service_host_name."-".$obj_2x_service_service_description;

                # 2. make the service object a template with a special unique name
                $cfg_obj_2x->{'service'}->{$service_obj_2x_key}->{'__I2CONVERT_IS_TEMPLATE'} = 1;
                $cfg_obj_2x->{'service'}->{$service_obj_2x_key}->{'__I2CONVERT_TEMPLATE_NAME'} = $service_template_name;

                # 3. use the template name as reference for the host->service
                $cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'SERVICE'}->{$obj_2x_host_service_cnt}->{'__I2CONVERT_USES_TEMPLATE'} = 1;
                push @{$cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'SERVICE'}->{$obj_2x_host_service_cnt}->{'__I2CONVERT_TEMPLATE_NAMES'}}, $service_template_name;
                
                # 4. define the service description for the service
                $cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'SERVICE'}->{$obj_2x_host_service_cnt}->{'__I2CONVERT_SERVICEDESCRIPTION'} = $obj_2x_service_service_description;

                ######################################
                # LINK HOST COMMAND WITH SERVICE CHECK
                ######################################
                my $service_check_command_2x = Icinga2::Convert::convert_checkcommand(@$cfg_obj_1x{'command'}, $obj_2x_service, $user_macros_1x);

                # check if this service check is a possible match for __I2CONVERT_HOST_CHECK?
                if (defined($service_check_command_2x->{'check_command_name_1x'})) {
                    if ($cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'__I2CONVERT_HOSTCHECK_NAME'} eq $service_check_command_2x->{'check_command_name_1x'}) {
                        # set service as hostcheck
                        $cfg_obj_2x->{'host'}->{$host_obj_2x_key}->{'__I2CONVERT_HOSTCHECK'} = $obj_2x_service_service_description;
                    }
                }

                # primary key
                $obj_2x_host_service_cnt++;
            }
            else {
                 # no match
                #say "ERROR: No Match with ". Dumper($obj_1x_host);
            }
        }
    }

    ############################################################################
    ############################################################################
    # export takes place outside again

    return $cfg_obj_2x;
}



1;

__END__
# vi: sw=4 ts=4 expandtab :
