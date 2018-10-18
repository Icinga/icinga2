-- -----------------------------------------
-- upgrade path for Icinga 2.0.2
--
-- -----------------------------------------
-- Copyright (c) 2014 Icinga Development Team (https://icinga.com/)
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

UPDATE icinga_objects SET name2 = NULL WHERE name2 = '';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.6');

