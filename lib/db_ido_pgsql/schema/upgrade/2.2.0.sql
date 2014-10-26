-- -----------------------------------------
-- upgrade path for Icinga 2.2.0
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD COLUMN program_version TEXT default NULL;

ALTER TABLE icinga_hostgroups DROP COLUMN action_url;
ALTER TABLE icinga_hostgroups DROP COLUMN notes_url;
ALTER TABLE icinga_hostgroups DROP COLUMN notes;
ALTER TABLE icinga_servicegroups DROP COLUMN action_url;
ALTER TABLE icinga_servicegroups DROP COLUMN notes_url;
ALTER TABLE icinga_servicegroups DROP COLUMN notes;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.12.0');

