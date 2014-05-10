

-- -----------------------------------------
-- #6094
-- -----------------------------------------

ALTER TABLE icinga_hoststatus ADD COLUMN is_reachable smallint(6) DEFAULT NULL;
ALTER TABLE icinga_servicestatus ADD COLUMN is_reachable smallint(6) DEFAULT NULL;

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.11.3', NOW(), NOW()) ON DUPLICATE KEY UPDATE version='1.11.3', modify_time=NOW();
