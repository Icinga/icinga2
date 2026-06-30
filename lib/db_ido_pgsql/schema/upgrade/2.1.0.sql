-- -----------------------------------------
-- upgrade path for Icinga 2.1.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

ALTER TABLE icinga_programstatus ADD COLUMN endpoint_name TEXT default NULL;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.7');

