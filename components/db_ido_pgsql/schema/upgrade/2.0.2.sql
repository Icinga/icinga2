-- -----------------------------------------
-- upgrade path for Icinga 2.0.2
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (http://www.icinga.org)
--
-- Please check http://docs.icinga.org for upgrading information!
-- -----------------------------------------

UPDATE icinga_objects SET name2 = NULL WHERE name2 = '';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.6');

