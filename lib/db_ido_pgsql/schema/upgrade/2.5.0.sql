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
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.1');
