-- -----------------------------------------
-- upgrade path for Icinga 2.2.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

ALTER TABLE `icinga_programstatus` ADD COLUMN `program_version` varchar(64) character set latin1 collate latin1_general_cs default NULL;

ALTER TABLE icinga_contacts MODIFY alias TEXT character set latin1;
ALTER TABLE icinga_hosts MODIFY alias TEXT character set latin1;

ALTER TABLE icinga_customvariables ADD COLUMN is_json smallint default 0;
ALTER TABLE icinga_customvariablestatus ADD COLUMN is_json smallint default 0;

-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.12.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.12.0', modify_time=NOW();

