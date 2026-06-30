-- -----------------------------------------
-- upgrade path for Icinga 2.12.7
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2021 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- -------------
-- set dbversion
-- -------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.15.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.15.0', modify_time=NOW();
