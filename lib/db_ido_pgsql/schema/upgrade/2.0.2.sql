-- -----------------------------------------
-- upgrade path for Icinga 2.0.2
--
-- -----------------------------------------
-- Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

UPDATE icinga_objects SET name2 = NULL WHERE name2 = '';

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.6');

