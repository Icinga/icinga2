-- -----------------------------------------
-- upgrade path for Icinga 2.4.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #9286 - zone tables
-- -----------------------------------------

ALTER TABLE icinga_endpoints ADD COLUMN zone_object_id bigint(20) unsigned DEFAULT '0';
ALTER TABLE icinga_endpointstatus ADD COLUMN zone_object_id bigint(20) unsigned DEFAULT '0';

CREATE TABLE IF NOT EXISTS icinga_zones (
  zone_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  zone_object_id bigint(20) unsigned DEFAULT '0',
  config_type smallint(6) DEFAULT '0',
  parent_zone_object_id bigint(20) unsigned DEFAULT '0',
  is_global smallint(6),
  PRIMARY KEY  (zone_id)
) ENGINE=InnoDB COMMENT='Zone configuration';

CREATE TABLE IF NOT EXISTS icinga_zonestatus (
  zonestatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  zone_object_id bigint(20) unsigned DEFAULT '0',
  status_update_time timestamp NOT NULL,
  parent_zone_object_id bigint(20) unsigned DEFAULT '0',
  PRIMARY KEY  (zonestatus_id)
) ENGINE=InnoDB COMMENT='Zone status';


-- -----------------------------------------
-- #9576 - freshness_threshold
-- -----------------------------------------

ALTER TABLE icinga_services MODIFY freshness_threshold int;
ALTER TABLE icinga_hosts MODIFY freshness_threshold int;

-- -----------------------------------------
-- #10392 - original attributes
-- -----------------------------------------

ALTER TABLE icinga_servicestatus ADD COLUMN original_attributes TEXT character set latin1  default NULL;
ALTER TABLE icinga_hoststatus ADD COLUMN original_attributes TEXT character set latin1  default NULL;

-- -----------------------------------------
-- #10436 deleted custom vars
-- -----------------------------------------

ALTER TABLE icinga_customvariables ADD COLUMN session_token int default NULL;
ALTER TABLE icinga_customvariablestatus ADD COLUMN session_token int default NULL;

CREATE INDEX cv_session_del_idx ON icinga_customvariables (session_token);
CREATE INDEX cvs_session_del_idx ON icinga_customvariablestatus (session_token);

-- -----------------------------------------
-- #10431 comment/downtime name
-- -----------------------------------------

ALTER TABLE icinga_comments ADD COLUMN name TEXT character set latin1 default NULL;
ALTER TABLE icinga_commenthistory ADD COLUMN name TEXT character set latin1 default NULL;

ALTER TABLE icinga_scheduleddowntime ADD COLUMN name TEXT character set latin1 default NULL;
ALTER TABLE icinga_downtimehistory ADD COLUMN name TEXT character set latin1 default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.14.0', modify_time=NOW();
