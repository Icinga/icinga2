-- -----------------------------------------
-- upgrade path for Icinga 2.3.0
--
-- -----------------------------------------
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- Please check https://docs.icinga.com for upgrading information!
-- -----------------------------------------

-- -----------------------------------------
-- #7765 drop unique constraint
-- -----------------------------------------

ALTER TABLE icinga_servicedependencies DROP KEY instance_id;
ALTER TABLE icinga_hostdependencies DROP KEY instance_id;

ALTER TABLE icinga_servicedependencies ADD KEY instance_id (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical);
ALTER TABLE icinga_hostdependencies ADD KEY instance_id (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable);


-- -----------------------------------------
-- update dbversion
-- -----------------------------------------

INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.13.0', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.13.0', modify_time=NOW();

