-- --------------------------------------------------------
-- pgsql.sql
-- DB definition for IDO Postgresql
--
-- SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
-- SPDX-License-Identifier: GPL-2.0-or-later
--
-- --------------------------------------------------------

--
-- Functions
--

DROP FUNCTION IF EXISTS from_unixtime(bigint);
CREATE FUNCTION from_unixtime(bigint) RETURNS timestamp AS $$
  SELECT to_timestamp($1) AT TIME ZONE 'UTC' AS result
$$ LANGUAGE sql;

DROP FUNCTION IF EXISTS unix_timestamp(timestamp WITH TIME ZONE);
CREATE OR REPLACE FUNCTION unix_timestamp(timestamp) RETURNS bigint AS '
  SELECT CAST(EXTRACT(EPOCH FROM $1) AS bigint) AS result;
' LANGUAGE sql;


-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

CREATE OR REPLACE FUNCTION updatedbversion(version_i TEXT) RETURNS void AS $$
BEGIN
        IF EXISTS( SELECT * FROM icinga_dbversion WHERE name='idoutils')
        THEN
                UPDATE icinga_dbversion
                SET version=version_i, modify_time=NOW()
		WHERE name='idoutils';
        ELSE
                INSERT INTO icinga_dbversion (dbversion_id, name, version, create_time, modify_time) VALUES ('1', 'idoutils', version_i, NOW(), NOW());
        END IF;

        RETURN;
END;
$$ LANGUAGE plpgsql;
-- HINT: su - postgres; createlang plpgsql icinga;



--
-- Database: icinga
--

-- --------------------------------------------------------

--
-- Table structure for table icinga_acknowledgements
--

CREATE TABLE  icinga_acknowledgements (
  acknowledgement_id bigserial,
  instance_id bigint default 0,
  entry_time timestamp,
  entry_time_usec INTEGER  default 0,
  acknowledgement_type INTEGER  default 0,
  object_id bigint default 0,
  state INTEGER  default 0,
  author_name TEXT  default '',
  comment_data TEXT  default '',
  is_sticky INTEGER  default 0,
  persistent_comment INTEGER  default 0,
  notify_contacts INTEGER  default 0,
  end_time timestamp,
  CONSTRAINT PK_acknowledgement_id PRIMARY KEY (acknowledgement_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_commands
--

CREATE TABLE  icinga_commands (
  command_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  object_id bigint default 0,
  command_line TEXT  default '',
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_command_id PRIMARY KEY (command_id) ,
  CONSTRAINT UQ_commands UNIQUE (instance_id,object_id,config_type)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_commenthistory
--

CREATE TABLE  icinga_commenthistory (
  commenthistory_id bigserial,
  instance_id bigint default 0,
  entry_time timestamp,
  entry_time_usec INTEGER  default 0,
  comment_type INTEGER  default 0,
  entry_type INTEGER  default 0,
  object_id bigint default 0,
  comment_time timestamp,
  internal_comment_id bigint default 0,
  author_name TEXT  default '',
  comment_data TEXT  default '',
  is_persistent INTEGER  default 0,
  comment_source INTEGER  default 0,
  expires INTEGER  default 0,
  expiration_time timestamp,
  deletion_time timestamp,
  deletion_time_usec INTEGER  default 0,
  name TEXT default NULL,
  CONSTRAINT PK_commenthistory_id PRIMARY KEY (commenthistory_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_comments
--

CREATE TABLE  icinga_comments (
  comment_id bigserial,
  instance_id bigint default 0,
  entry_time timestamp,
  entry_time_usec INTEGER  default 0,
  comment_type INTEGER  default 0,
  entry_type INTEGER  default 0,
  object_id bigint default 0,
  comment_time timestamp,
  internal_comment_id bigint default 0,
  author_name TEXT  default '',
  comment_data TEXT  default '',
  is_persistent INTEGER  default 0,
  comment_source INTEGER  default 0,
  expires INTEGER  default 0,
  expiration_time timestamp,
  name TEXT default NULL,
  session_token INTEGER default NULL,
  CONSTRAINT PK_comment_id PRIMARY KEY (comment_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfiles
--

CREATE TABLE  icinga_configfiles (
  configfile_id bigserial,
  instance_id bigint default 0,
  configfile_type INTEGER  default 0,
  configfile_path TEXT  default '',
  CONSTRAINT PK_configfile_id PRIMARY KEY (configfile_id) ,
  CONSTRAINT UQ_configfiles UNIQUE (instance_id,configfile_type,configfile_path)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfilevariables
--

CREATE TABLE  icinga_configfilevariables (
  configfilevariable_id bigserial,
  instance_id bigint default 0,
  configfile_id bigint default 0,
  varname TEXT  default '',
  varvalue TEXT  default '',
  CONSTRAINT PK_configfilevariable_id PRIMARY KEY (configfilevariable_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_conninfo
--

CREATE TABLE  icinga_conninfo (
  conninfo_id bigserial,
  instance_id bigint default 0,
  agent_name TEXT  default '',
  agent_version TEXT  default '',
  disposition TEXT  default '',
  connect_source TEXT  default '',
  connect_type TEXT  default '',
  connect_time timestamp,
  disconnect_time timestamp,
  last_checkin_time timestamp,
  data_start_time timestamp,
  data_end_time timestamp,
  bytes_processed bigint  default 0,
  lines_processed bigint  default 0,
  entries_processed bigint  default 0,
  CONSTRAINT PK_conninfo_id PRIMARY KEY (conninfo_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroups
--

CREATE TABLE  icinga_contactgroups (
  contactgroup_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  contactgroup_object_id bigint default 0,
  alias TEXT  default '',
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_contactgroup_id PRIMARY KEY (contactgroup_id) ,
  CONSTRAINT UQ_contactgroups UNIQUE (instance_id,config_type,contactgroup_object_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroup_members
--

CREATE TABLE  icinga_contactgroup_members (
  contactgroup_member_id bigserial,
  instance_id bigint default 0,
  contactgroup_id bigint default 0,
  contact_object_id bigint default 0,
  session_token INTEGER default NULL,
  CONSTRAINT PK_contactgroup_member_id PRIMARY KEY (contactgroup_member_id)
);

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotificationmethods
--

CREATE TABLE  icinga_contactnotificationmethods (
  contactnotificationmethod_id bigserial,
  instance_id bigint default 0,
  contactnotification_id bigint default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  command_object_id bigint default 0,
  command_args TEXT  default '',
  CONSTRAINT PK_contactnotificationmethod_id PRIMARY KEY (contactnotificationmethod_id) ,
  CONSTRAINT UQ_contactnotificationmethods UNIQUE (instance_id,contactnotification_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotifications
--

CREATE TABLE  icinga_contactnotifications (
  contactnotification_id bigserial,
  instance_id bigint default 0,
  notification_id bigint default 0,
  contact_object_id bigint default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  CONSTRAINT PK_contactnotification_id PRIMARY KEY (contactnotification_id) ,
  CONSTRAINT UQ_contactnotifications UNIQUE (instance_id,contact_object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contacts
--

CREATE TABLE  icinga_contacts (
  contact_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  contact_object_id bigint default 0,
  alias TEXT  default '',
  email_address TEXT  default '',
  pager_address TEXT  default '',
  host_timeperiod_object_id bigint default 0,
  service_timeperiod_object_id bigint default 0,
  host_notifications_enabled INTEGER  default 0,
  service_notifications_enabled INTEGER  default 0,
  can_submit_commands INTEGER  default 0,
  notify_service_recovery INTEGER  default 0,
  notify_service_warning INTEGER  default 0,
  notify_service_unknown INTEGER  default 0,
  notify_service_critical INTEGER  default 0,
  notify_service_flapping INTEGER  default 0,
  notify_service_downtime INTEGER  default 0,
  notify_host_recovery INTEGER  default 0,
  notify_host_down INTEGER  default 0,
  notify_host_unreachable INTEGER  default 0,
  notify_host_flapping INTEGER  default 0,
  notify_host_downtime INTEGER  default 0,
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_contact_id PRIMARY KEY (contact_id) ,
  CONSTRAINT UQ_contacts UNIQUE (instance_id,config_type,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactstatus
--

CREATE TABLE  icinga_contactstatus (
  contactstatus_id bigserial,
  instance_id bigint default 0,
  contact_object_id bigint default 0,
  status_update_time timestamp,
  host_notifications_enabled INTEGER  default 0,
  service_notifications_enabled INTEGER  default 0,
  last_host_notification timestamp,
  last_service_notification timestamp,
  modified_attributes INTEGER  default 0,
  modified_host_attributes INTEGER  default 0,
  modified_service_attributes INTEGER  default 0,
  CONSTRAINT PK_contactstatus_id PRIMARY KEY (contactstatus_id) ,
  CONSTRAINT UQ_contactstatus UNIQUE (contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_addresses
--

CREATE TABLE  icinga_contact_addresses (
  contact_address_id bigserial,
  instance_id bigint default 0,
  contact_id bigint default 0,
  address_number INTEGER  default 0,
  address TEXT  default '',
  CONSTRAINT PK_contact_address_id PRIMARY KEY (contact_address_id) ,
  CONSTRAINT UQ_contact_addresses UNIQUE (contact_id,address_number)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_notificationcommands
--

CREATE TABLE  icinga_contact_notificationcommands (
  contact_notificationcommand_id bigserial,
  instance_id bigint default 0,
  contact_id bigint default 0,
  notification_type INTEGER  default 0,
  command_object_id bigint default 0,
  command_args TEXT  default '',
  CONSTRAINT PK_contact_notificationcommand_id PRIMARY KEY (contact_notificationcommand_id) ,
  CONSTRAINT UQ_contact_notificationcommands UNIQUE (contact_id,notification_type,command_object_id,command_args)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariables
--

CREATE TABLE  icinga_customvariables (
  customvariable_id bigserial,
  instance_id bigint default 0,
  object_id bigint default 0,
  config_type INTEGER  default 0,
  has_been_modified INTEGER  default 0,
  varname TEXT  default '',
  varvalue TEXT  default '',
  is_json INTEGER  default 0,
  session_token INTEGER default NULL,
  CONSTRAINT PK_customvariable_id PRIMARY KEY (customvariable_id) ,
  CONSTRAINT UQ_customvariables UNIQUE (object_id,config_type,varname)
) ;
CREATE INDEX icinga_customvariables_i ON icinga_customvariables(varname);

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariablestatus
--

CREATE TABLE  icinga_customvariablestatus (
  customvariablestatus_id bigserial,
  instance_id bigint default 0,
  object_id bigint default 0,
  status_update_time timestamp,
  has_been_modified INTEGER  default 0,
  varname TEXT  default '',
  varvalue TEXT  default '',
  is_json INTEGER  default 0,
  session_token INTEGER default NULL,
  CONSTRAINT PK_customvariablestatus_id PRIMARY KEY (customvariablestatus_id) ,
  CONSTRAINT UQ_customvariablestatus UNIQUE (object_id,varname)
) ;
CREATE INDEX icinga_customvariablestatus_i ON icinga_customvariablestatus(varname);


-- --------------------------------------------------------

--
-- Table structure for table icinga_dbversion
--

CREATE TABLE  icinga_dbversion (
  dbversion_id bigserial,
  name TEXT  default '',
  version TEXT  default '',
  create_time timestamp,
  modify_time timestamp,
  CONSTRAINT PK_dbversion_id PRIMARY KEY (dbversion_id) ,
  CONSTRAINT UQ_dbversion UNIQUE (name)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_downtimehistory
--

CREATE TABLE  icinga_downtimehistory (
  downtimehistory_id bigserial,
  instance_id bigint default 0,
  downtime_type INTEGER  default 0,
  object_id bigint default 0,
  entry_time timestamp,
  author_name TEXT  default '',
  comment_data TEXT  default '',
  internal_downtime_id bigint default 0,
  triggered_by_id bigint default 0,
  is_fixed INTEGER  default 0,
  duration BIGINT  default 0,
  scheduled_start_time timestamp,
  scheduled_end_time timestamp,
  was_started INTEGER  default 0,
  actual_start_time timestamp,
  actual_start_time_usec INTEGER  default 0,
  actual_end_time timestamp,
  actual_end_time_usec INTEGER  default 0,
  was_cancelled INTEGER  default 0,
  is_in_effect INTEGER  default 0,
  trigger_time timestamp,
  name TEXT default NULL,
  CONSTRAINT PK_downtimehistory_id PRIMARY KEY (downtimehistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_eventhandlers
--

CREATE TABLE  icinga_eventhandlers (
  eventhandler_id bigserial,
  instance_id bigint default 0,
  eventhandler_type INTEGER  default 0,
  object_id bigint default 0,
  state INTEGER  default 0,
  state_type INTEGER  default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  command_object_id bigint default 0,
  command_args TEXT  default '',
  command_line TEXT  default '',
  timeout INTEGER  default 0,
  early_timeout INTEGER  default 0,
  execution_time double precision  default 0,
  return_code INTEGER  default 0,
  output TEXT  default '',
  long_output TEXT  default '',
  CONSTRAINT PK_eventhandler_id PRIMARY KEY (eventhandler_id) ,
  CONSTRAINT UQ_eventhandlers UNIQUE (instance_id,object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_externalcommands
--

CREATE TABLE  icinga_externalcommands (
  externalcommand_id bigserial,
  instance_id bigint default 0,
  entry_time timestamp,
  command_type INTEGER  default 0,
  command_name TEXT  default '',
  command_args TEXT  default '',
  CONSTRAINT PK_externalcommand_id PRIMARY KEY (externalcommand_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_flappinghistory
--

CREATE TABLE  icinga_flappinghistory (
  flappinghistory_id bigserial,
  instance_id bigint default 0,
  event_time timestamp,
  event_time_usec INTEGER  default 0,
  event_type INTEGER  default 0,
  reason_type INTEGER  default 0,
  flapping_type INTEGER  default 0,
  object_id bigint default 0,
  percent_state_change double precision  default 0,
  low_threshold double precision  default 0,
  high_threshold double precision  default 0,
  comment_time timestamp,
  internal_comment_id bigint default 0,
  CONSTRAINT PK_flappinghistory_id PRIMARY KEY (flappinghistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostchecks
--

CREATE TABLE  icinga_hostchecks (
  hostcheck_id bigserial,
  instance_id bigint default 0,
  host_object_id bigint default 0,
  check_type INTEGER  default 0,
  is_raw_check INTEGER  default 0,
  current_check_attempt INTEGER  default 0,
  max_check_attempts INTEGER  default 0,
  state INTEGER  default 0,
  state_type INTEGER  default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  command_object_id bigint default 0,
  command_args TEXT  default '',
  command_line TEXT  default '',
  timeout INTEGER  default 0,
  early_timeout INTEGER  default 0,
  execution_time double precision  default 0,
  latency double precision  default 0,
  return_code INTEGER  default 0,
  output TEXT  default '',
  long_output TEXT  default '',
  perfdata TEXT  default '',
  CONSTRAINT PK_hostcheck_id PRIMARY KEY (hostcheck_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostdependencies
--

CREATE TABLE  icinga_hostdependencies (
  hostdependency_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  host_object_id bigint default 0,
  dependent_host_object_id bigint default 0,
  dependency_type INTEGER  default 0,
  inherits_parent INTEGER  default 0,
  timeperiod_object_id bigint default 0,
  fail_on_up INTEGER  default 0,
  fail_on_down INTEGER  default 0,
  fail_on_unreachable INTEGER  default 0,
  CONSTRAINT PK_hostdependency_id PRIMARY KEY (hostdependency_id)
) ;
CREATE INDEX idx_hostdependencies ON icinga_hostdependencies(instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable);

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalations
--

CREATE TABLE  icinga_hostescalations (
  hostescalation_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  host_object_id bigint default 0,
  timeperiod_object_id bigint default 0,
  first_notification INTEGER  default 0,
  last_notification INTEGER  default 0,
  notification_interval double precision  default 0,
  escalate_on_recovery INTEGER  default 0,
  escalate_on_down INTEGER  default 0,
  escalate_on_unreachable INTEGER  default 0,
  CONSTRAINT PK_hostescalation_id PRIMARY KEY (hostescalation_id) ,
  CONSTRAINT UQ_hostescalations UNIQUE (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contactgroups
--

CREATE TABLE  icinga_hostescalation_contactgroups (
  hostescalation_contactgroup_id bigserial,
  instance_id bigint default 0,
  hostescalation_id bigint default 0,
  contactgroup_object_id bigint default 0,
  CONSTRAINT PK_hostescalation_contactgroup_id PRIMARY KEY (hostescalation_contactgroup_id) ,
  CONSTRAINT UQ_hostescalation_contactgroups UNIQUE (hostescalation_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contacts
--

CREATE TABLE  icinga_hostescalation_contacts (
  hostescalation_contact_id bigserial,
  instance_id bigint default 0,
  hostescalation_id bigint default 0,
  contact_object_id bigint default 0,
  CONSTRAINT PK_hostescalation_contact_id PRIMARY KEY (hostescalation_contact_id) ,
  CONSTRAINT UQ_hostescalation_contacts UNIQUE (instance_id,hostescalation_id,contact_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroups
--

CREATE TABLE  icinga_hostgroups (
  hostgroup_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  hostgroup_object_id bigint default 0,
  alias TEXT  default '',
  notes TEXT  default NULL,
  notes_url TEXT  default NULL,
  action_url TEXT  default NULL,
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_hostgroup_id PRIMARY KEY (hostgroup_id) ,
  CONSTRAINT UQ_hostgroups UNIQUE (instance_id,hostgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroup_members
--

CREATE TABLE  icinga_hostgroup_members (
  hostgroup_member_id bigserial,
  instance_id bigint default 0,
  hostgroup_id bigint default 0,
  host_object_id bigint default 0,
  session_token INTEGER default NULL,
  CONSTRAINT PK_hostgroup_member_id PRIMARY KEY (hostgroup_member_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hosts
--

CREATE TABLE  icinga_hosts (
  host_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  host_object_id bigint default 0,
  alias TEXT  default '',
  display_name TEXT  default '',
  address TEXT  default '',
  address6 TEXT  default '',
  check_command_object_id bigint default 0,
  check_command_args TEXT  default '',
  eventhandler_command_object_id bigint default 0,
  eventhandler_command_args TEXT  default '',
  notification_timeperiod_object_id bigint default 0,
  check_timeperiod_object_id bigint default 0,
  failure_prediction_options TEXT  default '',
  check_interval double precision  default 0,
  retry_interval double precision  default 0,
  max_check_attempts INTEGER  default 0,
  first_notification_delay double precision  default 0,
  notification_interval double precision  default 0,
  notify_on_down INTEGER  default 0,
  notify_on_unreachable INTEGER  default 0,
  notify_on_recovery INTEGER  default 0,
  notify_on_flapping INTEGER  default 0,
  notify_on_downtime INTEGER  default 0,
  stalk_on_up INTEGER  default 0,
  stalk_on_down INTEGER  default 0,
  stalk_on_unreachable INTEGER  default 0,
  flap_detection_enabled INTEGER  default 0,
  flap_detection_on_up INTEGER  default 0,
  flap_detection_on_down INTEGER  default 0,
  flap_detection_on_unreachable INTEGER  default 0,
  low_flap_threshold double precision  default 0,
  high_flap_threshold double precision  default 0,
  process_performance_data INTEGER  default 0,
  freshness_checks_enabled INTEGER  default 0,
  freshness_threshold INTEGER  default 0,
  passive_checks_enabled INTEGER  default 0,
  event_handler_enabled INTEGER  default 0,
  active_checks_enabled INTEGER  default 0,
  retain_status_information INTEGER  default 0,
  retain_nonstatus_information INTEGER  default 0,
  notifications_enabled INTEGER  default 0,
  obsess_over_host INTEGER  default 0,
  failure_prediction_enabled INTEGER  default 0,
  notes TEXT  default '',
  notes_url TEXT  default '',
  action_url TEXT  default '',
  icon_image TEXT  default '',
  icon_image_alt TEXT  default '',
  vrml_image TEXT  default '',
  statusmap_image TEXT  default '',
  have_2d_coords INTEGER  default 0,
  x_2d INTEGER  default 0,
  y_2d INTEGER  default 0,
  have_3d_coords INTEGER  default 0,
  x_3d double precision  default 0,
  y_3d double precision  default 0,
  z_3d double precision  default 0,
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_host_id PRIMARY KEY (host_id) ,
  CONSTRAINT UQ_hosts UNIQUE (instance_id,config_type,host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_hoststatus
--

CREATE TABLE  icinga_hoststatus (
  hoststatus_id bigserial,
  instance_id bigint default 0,
  host_object_id bigint default 0,
  status_update_time timestamp,
  output TEXT  default '',
  long_output TEXT  default '',
  perfdata TEXT  default '',
  check_source varchar(255) default '',
  current_state INTEGER  default 0,
  has_been_checked INTEGER  default 0,
  should_be_scheduled INTEGER  default 0,
  current_check_attempt INTEGER  default 0,
  max_check_attempts INTEGER  default 0,
  last_check timestamp,
  next_check timestamp,
  check_type INTEGER  default 0,
  last_state_change timestamp,
  last_hard_state_change timestamp,
  last_hard_state INTEGER  default 0,
  last_time_up timestamp,
  last_time_down timestamp,
  last_time_unreachable timestamp,
  state_type INTEGER  default 0,
  last_notification timestamp,
  next_notification timestamp,
  no_more_notifications INTEGER  default 0,
  notifications_enabled INTEGER  default 0,
  problem_has_been_acknowledged INTEGER  default 0,
  acknowledgement_type INTEGER  default 0,
  current_notification_number INTEGER  default 0,
  passive_checks_enabled INTEGER  default 0,
  active_checks_enabled INTEGER  default 0,
  event_handler_enabled INTEGER  default 0,
  flap_detection_enabled INTEGER  default 0,
  is_flapping INTEGER  default 0,
  percent_state_change double precision  default 0,
  latency double precision  default 0,
  execution_time double precision  default 0,
  scheduled_downtime_depth INTEGER  default 0,
  failure_prediction_enabled INTEGER  default 0,
  process_performance_data INTEGER  default 0,
  obsess_over_host INTEGER  default 0,
  modified_host_attributes INTEGER  default 0,
  original_attributes TEXT default NULL,
  event_handler TEXT  default '',
  check_command TEXT  default '',
  normal_check_interval double precision  default 0,
  retry_check_interval double precision  default 0,
  check_timeperiod_object_id bigint default 0,
  is_reachable INTEGER  default 0,
  CONSTRAINT PK_hoststatus_id PRIMARY KEY (hoststatus_id) ,
  CONSTRAINT UQ_hoststatus UNIQUE (host_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contactgroups
--

CREATE TABLE  icinga_host_contactgroups (
  host_contactgroup_id bigserial,
  instance_id bigint default 0,
  host_id bigint default 0,
  contactgroup_object_id bigint default 0,
  CONSTRAINT PK_host_contactgroup_id PRIMARY KEY (host_contactgroup_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contacts
--

CREATE TABLE  icinga_host_contacts (
  host_contact_id bigserial,
  instance_id bigint default 0,
  host_id bigint default 0,
  contact_object_id bigint default 0,
  CONSTRAINT PK_host_contact_id PRIMARY KEY (host_contact_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_parenthosts
--

CREATE TABLE  icinga_host_parenthosts (
  host_parenthost_id bigserial,
  instance_id bigint default 0,
  host_id bigint default 0,
  parent_host_object_id bigint default 0,
  CONSTRAINT PK_host_parenthost_id PRIMARY KEY (host_parenthost_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_instances
--

CREATE TABLE  icinga_instances (
  instance_id bigserial,
  instance_name TEXT  default '',
  instance_description TEXT  default '',
  CONSTRAINT PK_instance_id PRIMARY KEY (instance_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_logentries
--

CREATE TABLE  icinga_logentries (
  logentry_id bigserial,
  instance_id bigint default 0,
  logentry_time timestamp,
  entry_time timestamp,
  entry_time_usec INTEGER  default 0,
  logentry_type INTEGER  default 0,
  logentry_data TEXT  default '',
  realtime_data INTEGER  default 0,
  inferred_data_extracted INTEGER  default 0,
  object_id bigint default NULL,
  CONSTRAINT PK_logentry_id PRIMARY KEY (logentry_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_notifications
--

CREATE TABLE  icinga_notifications (
  notification_id bigserial,
  instance_id bigint default 0,
  notification_type INTEGER  default 0,
  notification_reason INTEGER  default 0,
  object_id bigint default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  state INTEGER  default 0,
  output TEXT  default '',
  long_output TEXT  default '',
  escalated INTEGER  default 0,
  contacts_notified INTEGER  default 0,
  CONSTRAINT PK_notification_id PRIMARY KEY (notification_id) ,
  CONSTRAINT UQ_notifications UNIQUE (instance_id,object_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_objects
--

CREATE TABLE  icinga_objects (
  object_id bigserial,
  instance_id bigint default 0,
  objecttype_id bigint default 0,
  name1 TEXT,
  name2 TEXT,
  is_active INTEGER  default 0,
  CONSTRAINT PK_object_id PRIMARY KEY (object_id)
--  UNIQUE (objecttype_id,name1,name2)
) ;
CREATE INDEX icinga_objects_i ON icinga_objects(objecttype_id,name1,name2);

-- --------------------------------------------------------

--
-- Table structure for table icinga_processevents
--

CREATE TABLE  icinga_processevents (
  processevent_id bigserial,
  instance_id bigint default 0,
  event_type INTEGER  default 0,
  event_time timestamp,
  event_time_usec INTEGER  default 0,
  process_id bigint default 0,
  program_name TEXT  default '',
  program_version TEXT  default '',
  program_date TEXT  default '',
  CONSTRAINT PK_processevent_id PRIMARY KEY (processevent_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_programstatus
--

CREATE TABLE  icinga_programstatus (
  programstatus_id bigserial,
  instance_id bigint default 0,
  program_version TEXT  default NULL,
  status_update_time timestamp,
  program_start_time timestamp,
  program_end_time timestamp,
  is_currently_running INTEGER  default 0,
  endpoint_name TEXT  default '',
  process_id bigint default 0,
  daemon_mode INTEGER  default 0,
  last_command_check timestamp,
  last_log_rotation timestamp,
  notifications_enabled INTEGER  default 0,
  disable_notif_expire_time timestamp,
  active_service_checks_enabled INTEGER  default 0,
  passive_service_checks_enabled INTEGER  default 0,
  active_host_checks_enabled INTEGER  default 0,
  passive_host_checks_enabled INTEGER  default 0,
  event_handlers_enabled INTEGER  default 0,
  flap_detection_enabled INTEGER  default 0,
  failure_prediction_enabled INTEGER  default 0,
  process_performance_data INTEGER  default 0,
  obsess_over_hosts INTEGER  default 0,
  obsess_over_services INTEGER  default 0,
  modified_host_attributes INTEGER  default 0,
  modified_service_attributes INTEGER  default 0,
  global_host_event_handler TEXT  default '',
  global_service_event_handler TEXT  default '',
  config_dump_in_progress INTEGER default 0,
  CONSTRAINT PK_programstatus_id PRIMARY KEY (programstatus_id) ,
  CONSTRAINT UQ_programstatus UNIQUE (instance_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_runtimevariables
--

CREATE TABLE  icinga_runtimevariables (
  runtimevariable_id bigserial,
  instance_id bigint default 0,
  varname TEXT  default '',
  varvalue TEXT  default '',
  CONSTRAINT PK_runtimevariable_id PRIMARY KEY (runtimevariable_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_scheduleddowntime
--

CREATE TABLE  icinga_scheduleddowntime (
  scheduleddowntime_id bigserial,
  instance_id bigint default 0,
  downtime_type INTEGER  default 0,
  object_id bigint default 0,
  entry_time timestamp,
  author_name TEXT  default '',
  comment_data TEXT  default '',
  internal_downtime_id bigint default 0,
  triggered_by_id bigint default 0,
  is_fixed INTEGER  default 0,
  duration BIGINT  default 0,
  scheduled_start_time timestamp,
  scheduled_end_time timestamp,
  was_started INTEGER  default 0,
  actual_start_time timestamp,
  actual_start_time_usec INTEGER  default 0,
  is_in_effect INTEGER  default 0,
  trigger_time timestamp,
  name TEXT default NULL,
  session_token INTEGER default NULL,
  CONSTRAINT PK_scheduleddowntime_id PRIMARY KEY (scheduleddowntime_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicechecks
--

CREATE TABLE  icinga_servicechecks (
  servicecheck_id bigserial,
  instance_id bigint default 0,
  service_object_id bigint default 0,
  check_type INTEGER  default 0,
  current_check_attempt INTEGER  default 0,
  max_check_attempts INTEGER  default 0,
  state INTEGER  default 0,
  state_type INTEGER  default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  command_object_id bigint default 0,
  command_args TEXT  default '',
  command_line TEXT  default '',
  timeout INTEGER  default 0,
  early_timeout INTEGER  default 0,
  execution_time double precision  default 0,
  latency double precision  default 0,
  return_code INTEGER  default 0,
  output TEXT  default '',
  long_output TEXT  default '',
  perfdata TEXT  default '',
  CONSTRAINT PK_servicecheck_id PRIMARY KEY (servicecheck_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicedependencies
--

CREATE TABLE  icinga_servicedependencies (
  servicedependency_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  service_object_id bigint default 0,
  dependent_service_object_id bigint default 0,
  dependency_type INTEGER  default 0,
  inherits_parent INTEGER  default 0,
  timeperiod_object_id bigint default 0,
  fail_on_ok INTEGER  default 0,
  fail_on_warning INTEGER  default 0,
  fail_on_unknown INTEGER  default 0,
  fail_on_critical INTEGER  default 0,
  CONSTRAINT PK_servicedependency_id PRIMARY KEY (servicedependency_id)
) ;
CREATE INDEX idx_servicedependencies ON icinga_servicedependencies(instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical);

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalations
--

CREATE TABLE  icinga_serviceescalations (
  serviceescalation_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  service_object_id bigint default 0,
  timeperiod_object_id bigint default 0,
  first_notification INTEGER  default 0,
  last_notification INTEGER  default 0,
  notification_interval double precision  default 0,
  escalate_on_recovery INTEGER  default 0,
  escalate_on_warning INTEGER  default 0,
  escalate_on_unknown INTEGER  default 0,
  escalate_on_critical INTEGER  default 0,
  CONSTRAINT PK_serviceescalation_id PRIMARY KEY (serviceescalation_id) ,
  CONSTRAINT UQ_serviceescalations UNIQUE (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contactgroups
--

CREATE TABLE  icinga_serviceescalation_contactgroups (
  serviceescalation_contactgroup_id bigserial,
  instance_id bigint default 0,
  serviceescalation_id bigint default 0,
  contactgroup_object_id bigint default 0,
  CONSTRAINT PK_serviceescalation_contactgroup_id PRIMARY KEY (serviceescalation_contactgroup_id) ,
  CONSTRAINT UQ_serviceescalation_contactgro UNIQUE (serviceescalation_id,contactgroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contacts
--

CREATE TABLE  icinga_serviceescalation_contacts (
  serviceescalation_contact_id bigserial,
  instance_id bigint default 0,
  serviceescalation_id bigint default 0,
  contact_object_id bigint default 0,
  CONSTRAINT PK_serviceescalation_contact_id PRIMARY KEY (serviceescalation_contact_id) ,
  CONSTRAINT UQ_serviceescalation_contacts UNIQUE (instance_id,serviceescalation_id,contact_object_id)
)  ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroups
--

CREATE TABLE  icinga_servicegroups (
  servicegroup_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  servicegroup_object_id bigint default 0,
  alias TEXT  default '',
  notes TEXT  default NULL,
  notes_url TEXT  default NULL,
  action_url TEXT  default NULL,
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_servicegroup_id PRIMARY KEY (servicegroup_id) ,
  CONSTRAINT UQ_servicegroups UNIQUE (instance_id,config_type,servicegroup_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroup_members
--

CREATE TABLE  icinga_servicegroup_members (
  servicegroup_member_id bigserial,
  instance_id bigint default 0,
  servicegroup_id bigint default 0,
  service_object_id bigint default 0,
  session_token INTEGER default NULL,
  CONSTRAINT PK_servicegroup_member_id PRIMARY KEY (servicegroup_member_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_services
--

CREATE TABLE  icinga_services (
  service_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  host_object_id bigint default 0,
  service_object_id bigint default 0,
  display_name TEXT  default '',
  check_command_object_id bigint default 0,
  check_command_args TEXT  default '',
  eventhandler_command_object_id bigint default 0,
  eventhandler_command_args TEXT  default '',
  notification_timeperiod_object_id bigint default 0,
  check_timeperiod_object_id bigint default 0,
  failure_prediction_options TEXT  default '',
  check_interval double precision  default 0,
  retry_interval double precision  default 0,
  max_check_attempts INTEGER  default 0,
  first_notification_delay double precision  default 0,
  notification_interval double precision  default 0,
  notify_on_warning INTEGER  default 0,
  notify_on_unknown INTEGER  default 0,
  notify_on_critical INTEGER  default 0,
  notify_on_recovery INTEGER  default 0,
  notify_on_flapping INTEGER  default 0,
  notify_on_downtime INTEGER  default 0,
  stalk_on_ok INTEGER  default 0,
  stalk_on_warning INTEGER  default 0,
  stalk_on_unknown INTEGER  default 0,
  stalk_on_critical INTEGER  default 0,
  is_volatile INTEGER  default 0,
  flap_detection_enabled INTEGER  default 0,
  flap_detection_on_ok INTEGER  default 0,
  flap_detection_on_warning INTEGER  default 0,
  flap_detection_on_unknown INTEGER  default 0,
  flap_detection_on_critical INTEGER  default 0,
  low_flap_threshold double precision  default 0,
  high_flap_threshold double precision  default 0,
  process_performance_data INTEGER  default 0,
  freshness_checks_enabled INTEGER  default 0,
  freshness_threshold INTEGER  default 0,
  passive_checks_enabled INTEGER  default 0,
  event_handler_enabled INTEGER  default 0,
  active_checks_enabled INTEGER  default 0,
  retain_status_information INTEGER  default 0,
  retain_nonstatus_information INTEGER  default 0,
  notifications_enabled INTEGER  default 0,
  obsess_over_service INTEGER  default 0,
  failure_prediction_enabled INTEGER  default 0,
  notes TEXT  default '',
  notes_url TEXT  default '',
  action_url TEXT  default '',
  icon_image TEXT  default '',
  icon_image_alt TEXT  default '',
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_service_id PRIMARY KEY (service_id) ,
  CONSTRAINT UQ_services UNIQUE (instance_id,config_type,service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicestatus
--

CREATE TABLE  icinga_servicestatus (
  servicestatus_id bigserial,
  instance_id bigint default 0,
  service_object_id bigint default 0,
  status_update_time timestamp,
  output TEXT  default '',
  long_output TEXT  default '',
  perfdata TEXT  default '',
  check_source varchar(255) default '',
  current_state INTEGER  default 0,
  has_been_checked INTEGER  default 0,
  should_be_scheduled INTEGER  default 0,
  current_check_attempt INTEGER  default 0,
  max_check_attempts INTEGER  default 0,
  last_check timestamp,
  next_check timestamp,
  check_type INTEGER  default 0,
  last_state_change timestamp,
  last_hard_state_change timestamp,
  last_hard_state INTEGER  default 0,
  last_time_ok timestamp,
  last_time_warning timestamp,
  last_time_unknown timestamp,
  last_time_critical timestamp,
  state_type INTEGER  default 0,
  last_notification timestamp,
  next_notification timestamp,
  no_more_notifications INTEGER  default 0,
  notifications_enabled INTEGER  default 0,
  problem_has_been_acknowledged INTEGER  default 0,
  acknowledgement_type INTEGER  default 0,
  current_notification_number INTEGER  default 0,
  passive_checks_enabled INTEGER  default 0,
  active_checks_enabled INTEGER  default 0,
  event_handler_enabled INTEGER  default 0,
  flap_detection_enabled INTEGER  default 0,
  is_flapping INTEGER  default 0,
  percent_state_change double precision  default 0,
  latency double precision  default 0,
  execution_time double precision  default 0,
  scheduled_downtime_depth INTEGER  default 0,
  failure_prediction_enabled INTEGER  default 0,
  process_performance_data INTEGER  default 0,
  obsess_over_service INTEGER  default 0,
  modified_service_attributes INTEGER  default 0,
  original_attributes TEXT default NULL,
  event_handler TEXT  default '',
  check_command TEXT  default '',
  normal_check_interval double precision  default 0,
  retry_check_interval double precision  default 0,
  check_timeperiod_object_id bigint default 0,
  is_reachable INTEGER  default 0,
  CONSTRAINT PK_servicestatus_id PRIMARY KEY (servicestatus_id) ,
  CONSTRAINT UQ_servicestatus UNIQUE (service_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contactgroups
--

CREATE TABLE  icinga_service_contactgroups (
  service_contactgroup_id bigserial,
  instance_id bigint default 0,
  service_id bigint default 0,
  contactgroup_object_id bigint default 0,
  CONSTRAINT PK_service_contactgroup_id PRIMARY KEY (service_contactgroup_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contacts
--

CREATE TABLE  icinga_service_contacts (
  service_contact_id bigserial,
  instance_id bigint default 0,
  service_id bigint default 0,
  contact_object_id bigint default 0,
  CONSTRAINT PK_service_contact_id PRIMARY KEY (service_contact_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_statehistory
--

CREATE TABLE  icinga_statehistory (
  statehistory_id bigserial,
  instance_id bigint default 0,
  state_time timestamp,
  state_time_usec INTEGER  default 0,
  object_id bigint default 0,
  state_change INTEGER  default 0,
  state INTEGER  default 0,
  state_type INTEGER  default 0,
  current_check_attempt INTEGER  default 0,
  max_check_attempts INTEGER  default 0,
  last_state INTEGER  default '-1',
  last_hard_state INTEGER  default '-1',
  output TEXT  default '',
  long_output TEXT  default '',
  check_source varchar(255) default '',
  CONSTRAINT PK_statehistory_id PRIMARY KEY (statehistory_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_systemcommands
--

CREATE TABLE  icinga_systemcommands (
  systemcommand_id bigserial,
  instance_id bigint default 0,
  start_time timestamp,
  start_time_usec INTEGER  default 0,
  end_time timestamp,
  end_time_usec INTEGER  default 0,
  command_line TEXT  default '',
  timeout INTEGER  default 0,
  early_timeout INTEGER  default 0,
  execution_time double precision  default 0,
  return_code INTEGER  default 0,
  output TEXT  default '',
  long_output TEXT  default '',
  CONSTRAINT PK_systemcommand_id PRIMARY KEY (systemcommand_id) ,
  CONSTRAINT UQ_systemcommands UNIQUE (instance_id,start_time,start_time_usec)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiods
--

CREATE TABLE  icinga_timeperiods (
  timeperiod_id bigserial,
  instance_id bigint default 0,
  config_type INTEGER  default 0,
  timeperiod_object_id bigint default 0,
  alias TEXT  default '',
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_timeperiod_id PRIMARY KEY (timeperiod_id) ,
  CONSTRAINT UQ_timeperiods UNIQUE (instance_id,config_type,timeperiod_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiod_timeranges
--

CREATE TABLE  icinga_timeperiod_timeranges (
  timeperiod_timerange_id bigserial,
  instance_id bigint default 0,
  timeperiod_id bigint default 0,
  day INTEGER  default 0,
  start_sec INTEGER  default 0,
  end_sec INTEGER  default 0,
  CONSTRAINT PK_timeperiod_timerange_id PRIMARY KEY (timeperiod_timerange_id)
) ;


-- --------------------------------------------------------
-- Icinga 2 specific schema extensions
-- --------------------------------------------------------

--
-- Table structure for table icinga_endpoints
--

CREATE TABLE  icinga_endpoints (
  endpoint_id bigserial,
  instance_id bigint default 0,
  endpoint_object_id bigint default 0,
  zone_object_id bigint default 0,
  config_type integer default 0,
  identity text DEFAULT NULL,
  node text DEFAULT NULL,
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_endpoint_id PRIMARY KEY (endpoint_id) ,
  CONSTRAINT UQ_endpoints UNIQUE (instance_id,config_type,endpoint_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_endpointstatus
--

CREATE TABLE  icinga_endpointstatus (
  endpointstatus_id bigserial,
  instance_id bigint default 0,
  endpoint_object_id bigint default 0,
  zone_object_id bigint default 0,
  status_update_time timestamp,
  identity text DEFAULT NULL,
  node text DEFAULT NULL,
  is_connected integer default 0,
  CONSTRAINT PK_endpointstatus_id PRIMARY KEY (endpointstatus_id) ,
  CONSTRAINT UQ_endpointstatus UNIQUE (endpoint_object_id)
) ;

--
-- Table structure for table icinga_zones
--

CREATE TABLE  icinga_zones (
  zone_id bigserial,
  instance_id bigint default 0,
  zone_object_id bigint default 0,
  parent_zone_object_id bigint default 0,
  config_type integer default 0,
  is_global integer default 0,
  config_hash varchar(64) DEFAULT NULL,
  CONSTRAINT PK_zone_id PRIMARY KEY (zone_id) ,
  CONSTRAINT UQ_zones UNIQUE (instance_id,config_type,zone_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_zonestatus
--

CREATE TABLE  icinga_zonestatus (
  zonestatus_id bigserial,
  instance_id bigint default 0,
  zone_object_id bigint default 0,
  parent_zone_object_id bigint default 0,
  status_update_time timestamp,
  CONSTRAINT PK_zonestatus_id PRIMARY KEY (zonestatus_id) ,
  CONSTRAINT UQ_zonestatus UNIQUE (zone_object_id)
) ;


ALTER TABLE icinga_servicestatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_hoststatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_contactstatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_programstatus ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_comments ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_scheduleddowntime ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_runtimevariables ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_customvariablestatus ADD COLUMN endpoint_object_id bigint default NULL;

ALTER TABLE icinga_acknowledgements ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_commenthistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_contactnotifications ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_downtimehistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_eventhandlers ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_externalcommands ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_flappinghistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_hostchecks ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_logentries ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_notifications ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_processevents ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_servicechecks ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_statehistory ADD COLUMN endpoint_object_id bigint default NULL;
ALTER TABLE icinga_systemcommands ADD COLUMN endpoint_object_id bigint default NULL;


-- -----------------------------------------
-- add index (delete)
-- -----------------------------------------

-- for periodic delete
-- instance_id and
-- TIMEDEVENTS => scheduled_time
-- SYSTEMCOMMANDS, SERVICECHECKS, HOSTCHECKS, EVENTHANDLERS  => start_time
-- EXTERNALCOMMANDS => entry_time

-- instance_id
CREATE INDEX systemcommands_i_id_idx on icinga_systemcommands(instance_id);
CREATE INDEX servicechecks_i_id_idx on icinga_servicechecks(instance_id);
CREATE INDEX hostchecks_i_id_idx on icinga_hostchecks(instance_id);
CREATE INDEX eventhandlers_i_id_idx on icinga_eventhandlers(instance_id);
CREATE INDEX externalcommands_i_id_idx on icinga_externalcommands(instance_id);

-- time
CREATE INDEX systemcommands_time_id_idx on icinga_systemcommands(start_time);
CREATE INDEX servicechecks_time_id_idx on icinga_servicechecks(start_time);
CREATE INDEX hostchecks_time_id_idx on icinga_hostchecks(start_time);
CREATE INDEX eventhandlers_time_id_idx on icinga_eventhandlers(start_time);
CREATE INDEX externalcommands_time_id_idx on icinga_externalcommands(entry_time);


-- for starting cleanup - referenced in dbhandler.c:882
-- instance_id only

-- realtime data
CREATE INDEX programstatus_i_id_idx on icinga_programstatus(instance_id);
CREATE INDEX hoststatus_i_id_idx on icinga_hoststatus(instance_id);
CREATE INDEX servicestatus_i_id_idx on icinga_servicestatus(instance_id);
CREATE INDEX contactstatus_i_id_idx on icinga_contactstatus(instance_id);
CREATE INDEX comments_i_id_idx on icinga_comments(instance_id);
CREATE INDEX scheduleddowntime_i_id_idx on icinga_scheduleddowntime(instance_id);
CREATE INDEX runtimevariables_i_id_idx on icinga_runtimevariables(instance_id);
CREATE INDEX customvariablestatus_i_id_idx on icinga_customvariablestatus(instance_id);

-- config data
CREATE INDEX configfiles_i_id_idx on icinga_configfiles(instance_id);
CREATE INDEX configfilevariables_i_id_idx on icinga_configfilevariables(instance_id);
CREATE INDEX customvariables_i_id_idx on icinga_customvariables(instance_id);
CREATE INDEX commands_i_id_idx on icinga_commands(instance_id);
CREATE INDEX timeperiods_i_id_idx on icinga_timeperiods(instance_id);
CREATE INDEX timeperiod_timeranges_i_id_idx on icinga_timeperiod_timeranges(instance_id);
CREATE INDEX contactgroups_i_id_idx on icinga_contactgroups(instance_id);
CREATE INDEX contactgroup_members_i_id_idx on icinga_contactgroup_members(instance_id);
CREATE INDEX hostgroups_i_id_idx on icinga_hostgroups(instance_id);
CREATE INDEX hostgroup_members_i_id_idx on icinga_hostgroup_members(instance_id);
CREATE INDEX servicegroups_i_id_idx on icinga_servicegroups(instance_id);
CREATE INDEX servicegroup_members_i_id_idx on icinga_servicegroup_members(instance_id);
CREATE INDEX hostesc_i_id_idx on icinga_hostescalations(instance_id);
CREATE INDEX hostesc_contacts_i_id_idx on icinga_hostescalation_contacts(instance_id);
CREATE INDEX serviceesc_i_id_idx on icinga_serviceescalations(instance_id);
CREATE INDEX serviceesc_contacts_i_id_idx on icinga_serviceescalation_contacts(instance_id);
CREATE INDEX hostdependencies_i_id_idx on icinga_hostdependencies(instance_id);
CREATE INDEX contacts_i_id_idx on icinga_contacts(instance_id);
CREATE INDEX contact_addresses_i_id_idx on icinga_contact_addresses(instance_id);
CREATE INDEX contact_notifcommands_i_id_idx on icinga_contact_notificationcommands(instance_id);
CREATE INDEX hosts_i_id_idx on icinga_hosts(instance_id);
CREATE INDEX host_parenthosts_i_id_idx on icinga_host_parenthosts(instance_id);
CREATE INDEX host_contacts_i_id_idx on icinga_host_contacts(instance_id);
CREATE INDEX services_i_id_idx on icinga_services(instance_id);
CREATE INDEX service_contacts_i_id_idx on icinga_service_contacts(instance_id);
CREATE INDEX service_contactgroups_i_id_idx on icinga_service_contactgroups(instance_id);
CREATE INDEX host_contactgroups_i_id_idx on icinga_host_contactgroups(instance_id);
CREATE INDEX hostesc_cgroups_i_id_idx on icinga_hostescalation_contactgroups(instance_id);
CREATE INDEX serviceesc_cgroups_i_id_idx on icinga_serviceescalation_contactgroups(instance_id);

-- -----------------------------------------
-- more index stuff (WHERE clauses)
-- -----------------------------------------

-- hosts
CREATE INDEX hosts_host_object_id_idx on icinga_hosts(host_object_id);

-- hoststatus
CREATE INDEX hoststatus_stat_upd_time_idx on icinga_hoststatus(status_update_time);
CREATE INDEX hoststatus_current_state_idx on icinga_hoststatus(current_state);
CREATE INDEX hoststatus_check_type_idx on icinga_hoststatus(check_type);
CREATE INDEX hoststatus_state_type_idx on icinga_hoststatus(state_type);
CREATE INDEX hoststatus_last_state_chg_idx on icinga_hoststatus(last_state_change);
CREATE INDEX hoststatus_notif_enabled_idx on icinga_hoststatus(notifications_enabled);
CREATE INDEX hoststatus_problem_ack_idx on icinga_hoststatus(problem_has_been_acknowledged);
CREATE INDEX hoststatus_act_chks_en_idx on icinga_hoststatus(active_checks_enabled);
CREATE INDEX hoststatus_pas_chks_en_idx on icinga_hoststatus(passive_checks_enabled);
CREATE INDEX hoststatus_event_hdl_en_idx on icinga_hoststatus(event_handler_enabled);
CREATE INDEX hoststatus_flap_det_en_idx on icinga_hoststatus(flap_detection_enabled);
CREATE INDEX hoststatus_is_flapping_idx on icinga_hoststatus(is_flapping);
CREATE INDEX hoststatus_p_state_chg_idx on icinga_hoststatus(percent_state_change);
CREATE INDEX hoststatus_latency_idx on icinga_hoststatus(latency);
CREATE INDEX hoststatus_ex_time_idx on icinga_hoststatus(execution_time);
CREATE INDEX hoststatus_sch_downt_d_idx on icinga_hoststatus(scheduled_downtime_depth);

-- services
CREATE INDEX services_host_object_id_idx on icinga_services(host_object_id);

--servicestatus
CREATE INDEX srvcstatus_stat_upd_time_idx on icinga_servicestatus(status_update_time);
CREATE INDEX srvcstatus_current_state_idx on icinga_servicestatus(current_state);
CREATE INDEX srvcstatus_check_type_idx on icinga_servicestatus(check_type);
CREATE INDEX srvcstatus_state_type_idx on icinga_servicestatus(state_type);
CREATE INDEX srvcstatus_last_state_chg_idx on icinga_servicestatus(last_state_change);
CREATE INDEX srvcstatus_notif_enabled_idx on icinga_servicestatus(notifications_enabled);
CREATE INDEX srvcstatus_problem_ack_idx on icinga_servicestatus(problem_has_been_acknowledged);
CREATE INDEX srvcstatus_act_chks_en_idx on icinga_servicestatus(active_checks_enabled);
CREATE INDEX srvcstatus_pas_chks_en_idx on icinga_servicestatus(passive_checks_enabled);
CREATE INDEX srvcstatus_event_hdl_en_idx on icinga_servicestatus(event_handler_enabled);
CREATE INDEX srvcstatus_flap_det_en_idx on icinga_servicestatus(flap_detection_enabled);
CREATE INDEX srvcstatus_is_flapping_idx on icinga_servicestatus(is_flapping);
CREATE INDEX srvcstatus_p_state_chg_idx on icinga_servicestatus(percent_state_change);
CREATE INDEX srvcstatus_latency_idx on icinga_servicestatus(latency);
CREATE INDEX srvcstatus_ex_time_idx on icinga_servicestatus(execution_time);
CREATE INDEX srvcstatus_sch_downt_d_idx on icinga_servicestatus(scheduled_downtime_depth);

-- hostchecks
CREATE INDEX hostchks_h_obj_id_idx on icinga_hostchecks(host_object_id);

-- servicechecks
CREATE INDEX servicechks_s_obj_id_idx on icinga_servicechecks(service_object_id);

-- objects
CREATE INDEX objects_objtype_id_idx ON icinga_objects(objecttype_id);
CREATE INDEX objects_name1_idx ON icinga_objects(name1);
CREATE INDEX objects_name2_idx ON icinga_objects(name2);
CREATE INDEX objects_inst_id_idx ON icinga_objects(instance_id);

-- instances
-- CREATE INDEX instances_name_idx on icinga_instances(instance_name);

-- logentries
-- CREATE INDEX loge_instance_id_idx on icinga_logentries(instance_id);
-- #236
CREATE INDEX loge_time_idx on icinga_logentries(logentry_time);
-- CREATE INDEX loge_data_idx on icinga_logentries(logentry_data);
CREATE INDEX loge_inst_id_time_idx on icinga_logentries (instance_id, logentry_time);


-- commenthistory
-- CREATE INDEX c_hist_instance_id_idx on icinga_logentries(instance_id);
-- CREATE INDEX c_hist_c_time_idx on icinga_logentries(comment_time);
-- CREATE INDEX c_hist_i_c_id_idx on icinga_logentries(internal_comment_id);

-- downtimehistory
-- CREATE INDEX d_t_hist_nstance_id_idx on icinga_downtimehistory(instance_id);
-- CREATE INDEX d_t_hist_type_idx on icinga_downtimehistory(downtime_type);
-- CREATE INDEX d_t_hist_object_id_idx on icinga_downtimehistory(object_id);
-- CREATE INDEX d_t_hist_entry_time_idx on icinga_downtimehistory(entry_time);
-- CREATE INDEX d_t_hist_sched_start_idx on icinga_downtimehistory(scheduled_start_time);
-- CREATE INDEX d_t_hist_sched_end_idx on icinga_downtimehistory(scheduled_end_time);

-- scheduleddowntime
-- CREATE INDEX sched_d_t_downtime_type_idx on icinga_scheduleddowntime(downtime_type);
-- CREATE INDEX sched_d_t_object_id_idx on icinga_scheduleddowntime(object_id);
-- CREATE INDEX sched_d_t_entry_time_idx on icinga_scheduleddowntime(entry_time);
-- CREATE INDEX sched_d_t_start_time_idx on icinga_scheduleddowntime(scheduled_start_time);
-- CREATE INDEX sched_d_t_end_time_idx on icinga_scheduleddowntime(scheduled_end_time);

-- Icinga Web Notifications
CREATE INDEX notification_idx ON icinga_notifications(notification_type, object_id, start_time);
CREATE INDEX notification_object_id_idx ON icinga_notifications(object_id);
CREATE INDEX contact_notification_idx ON icinga_contactnotifications(notification_id, contact_object_id);
CREATE INDEX contacts_object_id_idx ON icinga_contacts(contact_object_id);
CREATE INDEX contact_notif_meth_notif_idx ON icinga_contactnotificationmethods(contactnotification_id, command_object_id);
CREATE INDEX command_object_idx ON icinga_commands(object_id);
CREATE INDEX services_combined_object_idx ON icinga_services(service_object_id, host_object_id);

-- statehistory
CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on icinga_statehistory(instance_id, object_id, state_type, state_time);
--#2274
create index statehist_state_idx on icinga_statehistory(object_id,state);

-- #2618
CREATE INDEX cntgrpmbrs_cgid_coid ON icinga_contactgroup_members (contactgroup_id,contact_object_id);
CREATE INDEX hstgrpmbrs_hgid_hoid ON icinga_hostgroup_members (hostgroup_id,host_object_id);
CREATE INDEX hstcntgrps_hid_cgoid ON icinga_host_contactgroups (host_id,contactgroup_object_id);
CREATE INDEX hstprnthsts_hid_phoid ON icinga_host_parenthosts (host_id,parent_host_object_id);
CREATE INDEX runtimevars_iid_varn ON icinga_runtimevariables (instance_id,varname);
CREATE INDEX sgmbrs_sgid_soid ON icinga_servicegroup_members (servicegroup_id,service_object_id);
CREATE INDEX scgrps_sid_cgoid ON icinga_service_contactgroups (service_id,contactgroup_object_id);
CREATE INDEX tperiod_tid_d_ss_es ON icinga_timeperiod_timeranges (timeperiod_id,day,start_sec,end_sec);

-- #3649
CREATE INDEX sla_idx_sthist ON icinga_statehistory (object_id, state_time DESC);
CREATE INDEX sla_idx_dohist ON icinga_downtimehistory (object_id, actual_start_time, actual_end_time);
CREATE INDEX sla_idx_obj ON icinga_objects (objecttype_id, is_active, name1);

-- #4985
CREATE INDEX commenthistory_delete_idx ON icinga_commenthistory (instance_id, comment_time, internal_comment_id);

-- #10070
CREATE INDEX idx_comments_object_id on icinga_comments(object_id);
CREATE INDEX idx_scheduleddowntime_object_id on icinga_scheduleddowntime(object_id);

-- #10066
CREATE INDEX idx_endpoints_object_id on icinga_endpoints(endpoint_object_id);
CREATE INDEX idx_endpointstatus_object_id on icinga_endpointstatus(endpoint_object_id);

CREATE INDEX idx_endpoints_zone_object_id on icinga_endpoints(zone_object_id);
CREATE INDEX idx_endpointstatus_zone_object_id on icinga_endpointstatus(zone_object_id);

CREATE INDEX idx_zones_object_id on icinga_zones(zone_object_id);
CREATE INDEX idx_zonestatus_object_id on icinga_zonestatus(zone_object_id);

CREATE INDEX idx_zones_parent_object_id on icinga_zones(parent_zone_object_id);
CREATE INDEX idx_zonestatus_parent_object_id on icinga_zonestatus(parent_zone_object_id);

-- #12210
CREATE INDEX idx_comments_session_del ON icinga_comments (instance_id, session_token);
CREATE INDEX idx_downtimes_session_del ON icinga_scheduleddowntime (instance_id, session_token);

-- #12107
CREATE INDEX idx_statehistory_cleanup on icinga_statehistory(instance_id, state_time);

-- #12435
CREATE INDEX idx_customvariables_object_id on icinga_customvariables(object_id);
CREATE INDEX idx_contactgroup_members_object_id on icinga_contactgroup_members(contact_object_id);
CREATE INDEX idx_hostgroup_members_object_id on icinga_hostgroup_members(host_object_id);
CREATE INDEX idx_servicegroup_members_object_id on icinga_servicegroup_members(service_object_id);
CREATE INDEX idx_servicedependencies_dependent_service_object_id on icinga_servicedependencies(dependent_service_object_id);
CREATE INDEX idx_hostdependencies_dependent_host_object_id on icinga_hostdependencies(dependent_host_object_id);
CREATE INDEX idx_service_contacts_service_id on icinga_service_contacts(service_id);
CREATE INDEX idx_host_contacts_host_id on icinga_host_contacts(host_id);

-- #5458
CREATE INDEX idx_downtimehistory_remove ON icinga_downtimehistory (object_id, entry_time, scheduled_start_time, scheduled_end_time);
CREATE INDEX idx_scheduleddowntime_remove ON icinga_scheduleddowntime (object_id, entry_time, scheduled_start_time, scheduled_end_time);

-- #5492
CREATE INDEX idx_commenthistory_remove ON icinga_commenthistory (object_id, entry_time);
CREATE INDEX idx_comments_remove ON icinga_comments (object_id, entry_time);

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.14.3');

