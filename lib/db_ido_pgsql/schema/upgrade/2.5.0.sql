-- -----------------------------------------
-- upgrade path for Icinga 2.5.0
--
-- -----------------------------------------
-- Copyright (c) 2016 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
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
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.1');
