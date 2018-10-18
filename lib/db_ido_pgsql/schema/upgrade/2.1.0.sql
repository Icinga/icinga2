-- -----------------------------------------
-- upgrade path for Icinga 2.1.0
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (https://icinga.com/)
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD COLUMN endpoint_name TEXT default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.7');

