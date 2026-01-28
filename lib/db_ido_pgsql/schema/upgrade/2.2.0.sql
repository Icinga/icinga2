-- -----------------------------------------
-- upgrade path for Icinga 2.2.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD COLUMN program_version TEXT default NULL;

ALTER TABLE icinga_customvariables ADD COLUMN is_json INTEGER default 0;
ALTER TABLE icinga_customvariablestatus ADD COLUMN is_json INTEGER default 0;


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.12.0');

