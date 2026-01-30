-- -----------------------------------------
-- upgrade path for Icinga 2.5.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #10069 IDO: check_source should not be a TEXT field
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ALTER COLUMN check_source TYPE varchar(255);
ALTER TABLE icinga_servicestatus ALTER COLUMN check_source TYPE varchar(255);
ALTER TABLE icinga_statehistory ALTER COLUMN check_source TYPE varchar(255);

-- -----------------------------------------
-- #10070
-- -----------------------------------------

CREATE INDEX idx_comments_object_id on icinga_comments(object_id);
CREATE INDEX idx_scheduleddowntime_object_id on icinga_scheduleddowntime(object_id);

-- -----------------------------------------
-- #10066
-- -----------------------------------------

CREATE INDEX idx_endpoints_object_id on icinga_endpoints(endpoint_object_id);
CREATE INDEX idx_endpointstatus_object_id on icinga_endpointstatus(endpoint_object_id);

CREATE INDEX idx_endpoints_zone_object_id on icinga_endpoints(zone_object_id);
CREATE INDEX idx_endpointstatus_zone_object_id on icinga_endpointstatus(zone_object_id);

CREATE INDEX idx_zones_object_id on icinga_zones(zone_object_id);
CREATE INDEX idx_zonestatus_object_id on icinga_zonestatus(zone_object_id);

CREATE INDEX idx_zones_parent_object_id on icinga_zones(parent_zone_object_id);
CREATE INDEX idx_zonestatus_parent_object_id on icinga_zonestatus(parent_zone_object_id);

-- -----------------------------------------
-- #12258
-- -----------------------------------------
ALTER TABLE icinga_comments ADD COLUMN session_token INTEGER default NULL;
ALTER TABLE icinga_scheduleddowntime ADD COLUMN session_token INTEGER default NULL;

CREATE INDEX idx_comments_session_del ON icinga_comments (instance_id, session_token);
CREATE INDEX idx_downtimes_session_del ON icinga_scheduleddowntime (instance_id, session_token);

-- -----------------------------------------
-- #12107
-- -----------------------------------------
CREATE INDEX idx_statehistory_cleanup on icinga_statehistory(instance_id, state_time);

-- -----------------------------------------
-- #12435
-- -----------------------------------------
ALTER TABLE icinga_commands ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_contactgroups ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_contacts ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_hostgroups ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_hosts ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_servicegroups ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_services ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_timeperiods ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_endpoints ADD config_hash VARCHAR(64) DEFAULT NULL;
ALTER TABLE icinga_zones ADD config_hash VARCHAR(64) DEFAULT NULL;

ALTER TABLE icinga_customvariables DROP session_token;
ALTER TABLE icinga_customvariablestatus DROP session_token;

CREATE INDEX idx_customvariables_object_id on icinga_customvariables(object_id);
CREATE INDEX idx_contactgroup_members_object_id on icinga_contactgroup_members(contact_object_id);
CREATE INDEX idx_hostgroup_members_object_id on icinga_hostgroup_members(host_object_id);
CREATE INDEX idx_servicegroup_members_object_id on icinga_servicegroup_members(service_object_id);
CREATE INDEX idx_servicedependencies_dependent_service_object_id on icinga_servicedependencies(dependent_service_object_id);
CREATE INDEX idx_hostdependencies_dependent_host_object_id on icinga_hostdependencies(dependent_host_object_id);
CREATE INDEX idx_service_contacts_service_id on icinga_service_contacts(service_id);
CREATE INDEX idx_host_contacts_host_id on icinga_host_contacts(host_id);

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.1');
