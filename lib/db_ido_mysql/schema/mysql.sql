-- --------------------------------------------------------
-- mysql.sql
-- DB definition for IDO MySQL
--
-- Copyright (c) 2009-2017 Icinga Development Team (https://www.icinga.com/)
--
-- -- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: icinga
--

-- --------------------------------------------------------

--
-- Table structure for table icinga_acknowledgements
--

CREATE TABLE IF NOT EXISTS icinga_acknowledgements (
  acknowledgement_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  entry_time timestamp NULL,
  entry_time_usec  int default 0,
  acknowledgement_type smallint default 0,
  object_id bigint unsigned default 0,
  state smallint default 0,
  author_name varchar(64) character set latin1  default '',
  comment_data TEXT character set latin1,
  is_sticky smallint default 0,
  persistent_comment smallint default 0,
  notify_contacts smallint default 0,
  end_time timestamp NULL,
  PRIMARY KEY  (acknowledgement_id)
) ENGINE=InnoDB COMMENT='Current and historical host and service acknowledgements';

-- --------------------------------------------------------

--
-- Table structure for table icinga_commands
--

CREATE TABLE IF NOT EXISTS icinga_commands (
  command_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  object_id bigint unsigned default 0,
  command_line TEXT character set latin1,
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (command_id),
  UNIQUE KEY instance_id (instance_id,object_id,config_type)
) ENGINE=InnoDB  COMMENT='Command definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_commenthistory
--

CREATE TABLE IF NOT EXISTS icinga_commenthistory (
  commenthistory_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  entry_time timestamp NULL,
  entry_time_usec  int default 0,
  comment_type smallint default 0,
  entry_type smallint default 0,
  object_id bigint unsigned default 0,
  comment_time timestamp NULL,
  internal_comment_id bigint unsigned default 0,
  author_name varchar(64) character set latin1  default '',
  comment_data TEXT character set latin1,
  is_persistent smallint default 0,
  comment_source smallint default 0,
  expires smallint default 0,
  expiration_time timestamp NULL,
  deletion_time timestamp NULL,
  deletion_time_usec  int default 0,
  name TEXT character set latin1 default NULL,
  PRIMARY KEY  (commenthistory_id)
) ENGINE=InnoDB  COMMENT='Historical host and service comments';

-- --------------------------------------------------------

--
-- Table structure for table icinga_comments
--

CREATE TABLE IF NOT EXISTS icinga_comments (
  comment_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  entry_time timestamp NULL,
  entry_time_usec  int default 0,
  comment_type smallint default 0,
  entry_type smallint default 0,
  object_id bigint unsigned default 0,
  comment_time timestamp NULL,
  internal_comment_id bigint unsigned default 0,
  author_name varchar(64) character set latin1  default '',
  comment_data TEXT character set latin1,
  is_persistent smallint default 0,
  comment_source smallint default 0,
  expires smallint default 0,
  expiration_time timestamp NULL,
  name TEXT character set latin1 default NULL,
  session_token int default NULL,
  PRIMARY KEY  (comment_id)
) ENGINE=InnoDB  COMMENT='Usercomments on Icinga objects';

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfiles
--

CREATE TABLE IF NOT EXISTS icinga_configfiles (
  configfile_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  configfile_type smallint default 0,
  configfile_path varchar(255) character set latin1  default '',
  PRIMARY KEY  (configfile_id),
  UNIQUE KEY instance_id (instance_id,configfile_type,configfile_path)
) ENGINE=InnoDB  COMMENT='Configuration files';

-- --------------------------------------------------------

--
-- Table structure for table icinga_configfilevariables
--

CREATE TABLE IF NOT EXISTS icinga_configfilevariables (
  configfilevariable_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  configfile_id bigint unsigned default 0,
  varname varchar(64) character set latin1  default '',
  varvalue TEXT character set latin1,
  PRIMARY KEY  (configfilevariable_id)
) ENGINE=InnoDB  COMMENT='Configuration file variables';

-- --------------------------------------------------------

--
-- Table structure for table icinga_conninfo
--

CREATE TABLE IF NOT EXISTS icinga_conninfo (
  conninfo_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  agent_name varchar(32) character set latin1  default '',
  agent_version varchar(32) character set latin1  default '',
  disposition varchar(32) character set latin1  default '',
  connect_source varchar(32) character set latin1  default '',
  connect_type varchar(32) character set latin1  default '',
  connect_time timestamp NULL,
  disconnect_time timestamp NULL,
  last_checkin_time timestamp NULL,
  data_start_time timestamp NULL,
  data_end_time timestamp NULL,
  bytes_processed bigint unsigned  default '0',
  lines_processed bigint unsigned  default '0',
  entries_processed bigint unsigned  default '0',
  PRIMARY KEY  (conninfo_id)
) ENGINE=InnoDB  COMMENT='IDO2DB daemon connection information';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroups
--

CREATE TABLE IF NOT EXISTS icinga_contactgroups (
  contactgroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  contactgroup_object_id bigint unsigned default 0,
  alias varchar(255) character set latin1  default '',
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (contactgroup_id),
  UNIQUE KEY instance_id (instance_id,config_type,contactgroup_object_id)
) ENGINE=InnoDB  COMMENT='Contactgroup definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactgroup_members
--

CREATE TABLE IF NOT EXISTS icinga_contactgroup_members (
  contactgroup_member_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  contactgroup_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  PRIMARY KEY  (contactgroup_member_id)
) ENGINE=InnoDB  COMMENT='Contactgroup members';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotificationmethods
--

CREATE TABLE IF NOT EXISTS icinga_contactnotificationmethods (
  contactnotificationmethod_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  contactnotification_id bigint unsigned default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  command_object_id bigint unsigned default 0,
  command_args TEXT character set latin1,
  PRIMARY KEY  (contactnotificationmethod_id),
  UNIQUE KEY instance_id (instance_id,contactnotification_id,start_time,start_time_usec)
) ENGINE=InnoDB  COMMENT='Historical record of contact notification methods';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactnotifications
--

CREATE TABLE IF NOT EXISTS icinga_contactnotifications (
  contactnotification_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  notification_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  PRIMARY KEY  (contactnotification_id),
  UNIQUE KEY instance_id (instance_id,contact_object_id,start_time,start_time_usec)
) ENGINE=InnoDB  COMMENT='Historical record of contact notifications';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contacts
--

CREATE TABLE IF NOT EXISTS icinga_contacts (
  contact_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  contact_object_id bigint unsigned default 0,
  alias varchar(255) character set latin1  default '',
  email_address varchar(255) character set latin1  default '',
  pager_address varchar(64) character set latin1  default '',
  host_timeperiod_object_id bigint unsigned default 0,
  service_timeperiod_object_id bigint unsigned default 0,
  host_notifications_enabled smallint default 0,
  service_notifications_enabled smallint default 0,
  can_submit_commands smallint default 0,
  notify_service_recovery smallint default 0,
  notify_service_warning smallint default 0,
  notify_service_unknown smallint default 0,
  notify_service_critical smallint default 0,
  notify_service_flapping smallint default 0,
  notify_service_downtime smallint default 0,
  notify_host_recovery smallint default 0,
  notify_host_down smallint default 0,
  notify_host_unreachable smallint default 0,
  notify_host_flapping smallint default 0,
  notify_host_downtime smallint default 0,
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (contact_id),
  UNIQUE KEY instance_id (instance_id,config_type,contact_object_id)
) ENGINE=InnoDB  COMMENT='Contact definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contactstatus
--

CREATE TABLE IF NOT EXISTS icinga_contactstatus (
  contactstatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  status_update_time timestamp NULL,
  host_notifications_enabled smallint default 0,
  service_notifications_enabled smallint default 0,
  last_host_notification timestamp NULL,
  last_service_notification timestamp NULL,
  modified_attributes  int default 0,
  modified_host_attributes  int default 0,
  modified_service_attributes  int default 0,
  PRIMARY KEY  (contactstatus_id),
  UNIQUE KEY contact_object_id (contact_object_id)
) ENGINE=InnoDB  COMMENT='Contact status';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_addresses
--

CREATE TABLE IF NOT EXISTS icinga_contact_addresses (
  contact_address_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  contact_id bigint unsigned default 0,
  address_number smallint default 0,
  address varchar(255) character set latin1  default '',
  PRIMARY KEY  (contact_address_id),
  UNIQUE KEY contact_id (contact_id,address_number)
) ENGINE=InnoDB COMMENT='Contact addresses';

-- --------------------------------------------------------

--
-- Table structure for table icinga_contact_notificationcommands
--

CREATE TABLE IF NOT EXISTS icinga_contact_notificationcommands (
  contact_notificationcommand_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  contact_id bigint unsigned default 0,
  notification_type smallint default 0,
  command_object_id bigint unsigned default 0,
  command_args varchar(255) character set latin1  default '',
  PRIMARY KEY  (contact_notificationcommand_id),
  UNIQUE KEY contact_id (contact_id,notification_type,command_object_id,command_args)
) ENGINE=InnoDB  COMMENT='Contact host and service notification commands';

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariables
--

CREATE TABLE IF NOT EXISTS icinga_customvariables (
  customvariable_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  object_id bigint unsigned default 0,
  config_type smallint default 0,
  has_been_modified smallint default 0,
  varname varchar(255) character set latin1 collate latin1_general_cs default NULL,
  varvalue TEXT character set latin1,
  is_json smallint default 0,
  PRIMARY KEY  (customvariable_id),
  UNIQUE KEY object_id_2 (object_id,config_type,varname),
  KEY varname (varname)
) ENGINE=InnoDB COMMENT='Custom variables';

-- --------------------------------------------------------

--
-- Table structure for table icinga_customvariablestatus
--

CREATE TABLE IF NOT EXISTS icinga_customvariablestatus (
  customvariablestatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  object_id bigint unsigned default 0,
  status_update_time timestamp NULL,
  has_been_modified smallint default 0,
  varname varchar(255) character set latin1 collate latin1_general_cs default NULL,
  varvalue TEXT character set latin1,
  is_json smallint default 0,
  PRIMARY KEY  (customvariablestatus_id),
  UNIQUE KEY object_id_2 (object_id,varname),
  KEY varname (varname)
) ENGINE=InnoDB COMMENT='Custom variable status information';

-- --------------------------------------------------------

--
-- Table structure for table icinga_dbversion
--

CREATE TABLE IF NOT EXISTS icinga_dbversion (
  dbversion_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  name varchar(10) character set latin1  default '',
  version varchar(10) character set latin1  default '',
  create_time timestamp NULL,
  modify_time timestamp NULL,
  PRIMARY KEY (dbversion_id),
  UNIQUE KEY dbversion (name)
) ENGINE=InnoDB;

-- --------------------------------------------------------

--
-- Table structure for table icinga_downtimehistory
--

CREATE TABLE IF NOT EXISTS icinga_downtimehistory (
  downtimehistory_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  downtime_type smallint default 0,
  object_id bigint unsigned default 0,
  entry_time timestamp NULL,
  author_name varchar(64) character set latin1  default '',
  comment_data TEXT character set latin1,
  internal_downtime_id bigint unsigned default 0,
  triggered_by_id bigint unsigned default 0,
  is_fixed smallint default 0,
  duration bigint(20) default 0,
  scheduled_start_time timestamp NULL,
  scheduled_end_time timestamp NULL,
  was_started smallint default 0,
  actual_start_time timestamp NULL,
  actual_start_time_usec  int default 0,
  actual_end_time timestamp NULL,
  actual_end_time_usec  int default 0,
  was_cancelled smallint default 0,
  is_in_effect smallint default 0,
  trigger_time timestamp NULL,
  name TEXT character set latin1 default NULL,
  PRIMARY KEY  (downtimehistory_id)
) ENGINE=InnoDB  COMMENT='Historical scheduled host and service downtime';

-- --------------------------------------------------------

--
-- Table structure for table icinga_eventhandlers
--

CREATE TABLE IF NOT EXISTS icinga_eventhandlers (
  eventhandler_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  eventhandler_type smallint default 0,
  object_id bigint unsigned default 0,
  state smallint default 0,
  state_type smallint default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  command_object_id bigint unsigned default 0,
  command_args TEXT character set latin1,
  command_line TEXT character set latin1,
  timeout smallint default 0,
  early_timeout smallint default 0,
  execution_time double  default '0',
  return_code smallint default 0,
  output TEXT character set latin1,
  long_output TEXT,
  PRIMARY KEY  (eventhandler_id),
  UNIQUE KEY instance_id (instance_id,object_id,start_time,start_time_usec)
) ENGINE=InnoDB COMMENT='Historical host and service event handlers';

-- --------------------------------------------------------

--
-- Table structure for table icinga_externalcommands
--

CREATE TABLE IF NOT EXISTS icinga_externalcommands (
  externalcommand_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  entry_time timestamp NULL,
  command_type smallint default 0,
  command_name varchar(128) character set latin1  default '',
  command_args TEXT character set latin1,
  PRIMARY KEY  (externalcommand_id)
) ENGINE=InnoDB  COMMENT='Historical record of processed external commands';

-- --------------------------------------------------------

--
-- Table structure for table icinga_flappinghistory
--

CREATE TABLE IF NOT EXISTS icinga_flappinghistory (
  flappinghistory_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  event_time timestamp NULL,
  event_time_usec  int default 0,
  event_type smallint default 0,
  reason_type smallint default 0,
  flapping_type smallint default 0,
  object_id bigint unsigned default 0,
  percent_state_change double  default '0',
  low_threshold double  default '0',
  high_threshold double  default '0',
  comment_time timestamp NULL,
  internal_comment_id bigint unsigned default 0,
  PRIMARY KEY  (flappinghistory_id)
) ENGINE=InnoDB  COMMENT='Current and historical record of host and service flapping';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostchecks
--

CREATE TABLE IF NOT EXISTS icinga_hostchecks (
  hostcheck_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  host_object_id bigint unsigned default 0,
  check_type smallint default 0,
  is_raw_check smallint default 0,
  current_check_attempt smallint default 0,
  max_check_attempts smallint default 0,
  state smallint default 0,
  state_type smallint default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  command_object_id bigint unsigned default 0,
  command_args TEXT character set latin1,
  command_line TEXT character set latin1,
  timeout smallint default 0,
  early_timeout smallint default 0,
  execution_time double  default '0',
  latency double  default '0',
  return_code smallint default 0,
  output TEXT character set latin1,
  long_output TEXT,
  perfdata TEXT character set latin1,
  PRIMARY KEY  (hostcheck_id)
) ENGINE=InnoDB  COMMENT='Historical host checks';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostdependencies
--

CREATE TABLE IF NOT EXISTS icinga_hostdependencies (
  hostdependency_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  host_object_id bigint unsigned default 0,
  dependent_host_object_id bigint unsigned default 0,
  dependency_type smallint default 0,
  inherits_parent smallint default 0,
  timeperiod_object_id bigint unsigned default 0,
  fail_on_up smallint default 0,
  fail_on_down smallint default 0,
  fail_on_unreachable smallint default 0,
  PRIMARY KEY  (hostdependency_id),
  KEY instance_id (instance_id,config_type,host_object_id,dependent_host_object_id,dependency_type,inherits_parent,fail_on_up,fail_on_down,fail_on_unreachable)
) ENGINE=InnoDB COMMENT='Host dependency definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalations
--

CREATE TABLE IF NOT EXISTS icinga_hostescalations (
  hostescalation_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  host_object_id bigint unsigned default 0,
  timeperiod_object_id bigint unsigned default 0,
  first_notification smallint default 0,
  last_notification smallint default 0,
  notification_interval double  default '0',
  escalate_on_recovery smallint default 0,
  escalate_on_down smallint default 0,
  escalate_on_unreachable smallint default 0,
  PRIMARY KEY  (hostescalation_id),
  UNIQUE KEY instance_id (instance_id,config_type,host_object_id,timeperiod_object_id,first_notification,last_notification)
) ENGINE=InnoDB  COMMENT='Host escalation definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contactgroups
--

CREATE TABLE IF NOT EXISTS icinga_hostescalation_contactgroups (
  hostescalation_contactgroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  hostescalation_id bigint unsigned default 0,
  contactgroup_object_id bigint unsigned default 0,
  PRIMARY KEY  (hostescalation_contactgroup_id),
  UNIQUE KEY instance_id (hostescalation_id,contactgroup_object_id)
) ENGINE=InnoDB  COMMENT='Host escalation contact groups';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostescalation_contacts
--

CREATE TABLE IF NOT EXISTS icinga_hostescalation_contacts (
  hostescalation_contact_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  hostescalation_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  PRIMARY KEY  (hostescalation_contact_id),
  UNIQUE KEY instance_id (instance_id,hostescalation_id,contact_object_id)
) ENGINE=InnoDB  COMMENT='Host escalation contacts';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroups
--

CREATE TABLE IF NOT EXISTS icinga_hostgroups (
  hostgroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  hostgroup_object_id bigint unsigned default 0,
  alias varchar(255) character set latin1  default '',
  notes TEXT character set latin1  default NULL,
  notes_url TEXT character set latin1  default NULL,
  action_url TEXT character set latin1  default NULL,
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (hostgroup_id),
  UNIQUE KEY instance_id (instance_id,hostgroup_object_id)
) ENGINE=InnoDB  COMMENT='Hostgroup definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hostgroup_members
--

CREATE TABLE IF NOT EXISTS icinga_hostgroup_members (
  hostgroup_member_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  hostgroup_id bigint unsigned default 0,
  host_object_id bigint unsigned default 0,
  PRIMARY KEY  (hostgroup_member_id)
) ENGINE=InnoDB  COMMENT='Hostgroup members';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hosts
--

CREATE TABLE IF NOT EXISTS icinga_hosts (
  host_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  host_object_id bigint unsigned default 0,
  alias varchar(255) character set latin1  default '',
  display_name varchar(255) character set latin1 collate latin1_general_cs  default '',
  address varchar(128) character set latin1  default '',
  address6 varchar(128) character set latin1  default '',
  check_command_object_id bigint unsigned default 0,
  check_command_args TEXT character set latin1,
  eventhandler_command_object_id bigint unsigned default 0,
  eventhandler_command_args TEXT character set latin1,
  notification_timeperiod_object_id bigint unsigned default 0,
  check_timeperiod_object_id bigint unsigned default 0,
  failure_prediction_options varchar(128) character set latin1  default '',
  check_interval double  default '0',
  retry_interval double  default '0',
  max_check_attempts smallint default 0,
  first_notification_delay double  default '0',
  notification_interval double  default '0',
  notify_on_down smallint default 0,
  notify_on_unreachable smallint default 0,
  notify_on_recovery smallint default 0,
  notify_on_flapping smallint default 0,
  notify_on_downtime smallint default 0,
  stalk_on_up smallint default 0,
  stalk_on_down smallint default 0,
  stalk_on_unreachable smallint default 0,
  flap_detection_enabled smallint default 0,
  flap_detection_on_up smallint default 0,
  flap_detection_on_down smallint default 0,
  flap_detection_on_unreachable smallint default 0,
  low_flap_threshold double  default '0',
  high_flap_threshold double  default '0',
  process_performance_data smallint default 0,
  freshness_checks_enabled smallint default 0,
  freshness_threshold int default 0,
  passive_checks_enabled smallint default 0,
  event_handler_enabled smallint default 0,
  active_checks_enabled smallint default 0,
  retain_status_information smallint default 0,
  retain_nonstatus_information smallint default 0,
  notifications_enabled smallint default 0,
  obsess_over_host smallint default 0,
  failure_prediction_enabled smallint default 0,
  notes TEXT character set latin1,
  notes_url TEXT character set latin1,
  action_url TEXT character set latin1,
  icon_image TEXT character set latin1,
  icon_image_alt TEXT character set latin1,
  vrml_image TEXT character set latin1,
  statusmap_image TEXT character set latin1,
  have_2d_coords smallint default 0,
  x_2d smallint default 0,
  y_2d smallint default 0,
  have_3d_coords smallint default 0,
  x_3d double  default '0',
  y_3d double  default '0',
  z_3d double  default '0',
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (host_id),
  UNIQUE KEY instance_id (instance_id,config_type,host_object_id),
  KEY host_object_id (host_object_id)
) ENGINE=InnoDB  COMMENT='Host definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_hoststatus
--

CREATE TABLE IF NOT EXISTS icinga_hoststatus (
  hoststatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  host_object_id bigint unsigned default 0,
  status_update_time timestamp NULL,
  output TEXT character set latin1,
  long_output TEXT,
  perfdata TEXT character set latin1,
  check_source varchar(255) character set latin1  default '',
  current_state smallint default 0,
  has_been_checked smallint default 0,
  should_be_scheduled smallint default 0,
  current_check_attempt smallint default 0,
  max_check_attempts smallint default 0,
  last_check timestamp NULL,
  next_check timestamp NULL,
  check_type smallint default 0,
  last_state_change timestamp NULL,
  last_hard_state_change timestamp NULL,
  last_hard_state smallint default 0,
  last_time_up timestamp NULL,
  last_time_down timestamp NULL,
  last_time_unreachable timestamp NULL,
  state_type smallint default 0,
  last_notification timestamp NULL,
  next_notification timestamp NULL,
  no_more_notifications smallint default 0,
  notifications_enabled smallint default 0,
  problem_has_been_acknowledged smallint default 0,
  acknowledgement_type smallint default 0,
  current_notification_number int unsigned default 0,
  passive_checks_enabled smallint default 0,
  active_checks_enabled smallint default 0,
  event_handler_enabled smallint default 0,
  flap_detection_enabled smallint default 0,
  is_flapping smallint default 0,
  percent_state_change double  default '0',
  latency double  default '0',
  execution_time double  default '0',
  scheduled_downtime_depth smallint default 0,
  failure_prediction_enabled smallint default 0,
  process_performance_data smallint default 0,
  obsess_over_host smallint default 0,
  modified_host_attributes  int default 0,
  original_attributes TEXT character set latin1  default NULL,
  event_handler TEXT character set latin1,
  check_command TEXT character set latin1,
  normal_check_interval double  default '0',
  retry_check_interval double  default '0',
  check_timeperiod_object_id bigint unsigned default 0,
  is_reachable smallint default 0,
  PRIMARY KEY  (hoststatus_id),
  UNIQUE KEY object_id (host_object_id)
) ENGINE=InnoDB  COMMENT='Current host status information';

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contactgroups
--

CREATE TABLE IF NOT EXISTS icinga_host_contactgroups (
  host_contactgroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  host_id bigint unsigned default 0,
  contactgroup_object_id bigint unsigned default 0,
  PRIMARY KEY  (host_contactgroup_id)
) ENGINE=InnoDB  COMMENT='Host contact groups';

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_contacts
--

CREATE TABLE IF NOT EXISTS icinga_host_contacts (
  host_contact_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  host_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  PRIMARY KEY  (host_contact_id)
) ENGINE=InnoDB  COMMENT='Host contacts';

-- --------------------------------------------------------

--
-- Table structure for table icinga_host_parenthosts
--

CREATE TABLE IF NOT EXISTS icinga_host_parenthosts (
  host_parenthost_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  host_id bigint unsigned default 0,
  parent_host_object_id bigint unsigned default 0,
  PRIMARY KEY  (host_parenthost_id)
) ENGINE=InnoDB  COMMENT='Parent hosts';

-- --------------------------------------------------------

--
-- Table structure for table icinga_instances
--

CREATE TABLE IF NOT EXISTS icinga_instances (
  instance_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_name varchar(64) character set latin1  default '',
  instance_description varchar(128) character set latin1  default '',
  PRIMARY KEY  (instance_id)
) ENGINE=InnoDB  COMMENT='Location names of various Icinga installations';

-- --------------------------------------------------------

--
-- Table structure for table icinga_logentries
--

CREATE TABLE IF NOT EXISTS icinga_logentries (
  logentry_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  logentry_time timestamp NULL,
  entry_time timestamp NULL,
  entry_time_usec  int default 0,
  logentry_type  int default 0,
  logentry_data TEXT character set latin1,
  realtime_data smallint default 0,
  inferred_data_extracted smallint default 0,
  object_id bigint unsigned default NULL,
  PRIMARY KEY  (logentry_id)
) ENGINE=InnoDB COMMENT='Historical record of log entries';

-- --------------------------------------------------------

--
-- Table structure for table icinga_notifications
--

CREATE TABLE IF NOT EXISTS icinga_notifications (
  notification_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  notification_type smallint default 0,
  notification_reason smallint default 0,
  object_id bigint unsigned default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  state smallint default 0,
  output TEXT character set latin1,
  long_output TEXT,
  escalated smallint default 0,
  contacts_notified smallint default 0,
  PRIMARY KEY  (notification_id),
  UNIQUE KEY instance_id (instance_id,object_id,start_time,start_time_usec)
) ENGINE=InnoDB  COMMENT='Historical record of host and service notifications';

-- --------------------------------------------------------

--
-- Table structure for table icinga_objects
--

CREATE TABLE IF NOT EXISTS icinga_objects (
  object_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  objecttype_id bigint unsigned default 0,
  name1 varchar(128) character set latin1 collate latin1_general_cs  default '',
  name2 varchar(128) character set latin1 collate latin1_general_cs default NULL,
  is_active smallint default 0,
  PRIMARY KEY  (object_id),
  KEY objecttype_id (objecttype_id,name1,name2)
) ENGINE=InnoDB  COMMENT='Current and historical objects of all kinds';

-- --------------------------------------------------------

--
-- Table structure for table icinga_processevents
--

CREATE TABLE IF NOT EXISTS icinga_processevents (
  processevent_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  event_type smallint default 0,
  event_time timestamp NULL,
  event_time_usec  int default 0,
  process_id bigint unsigned default 0,
  program_name varchar(16) character set latin1  default '',
  program_version varchar(20) character set latin1  default '',
  program_date varchar(10) character set latin1  default '',
  PRIMARY KEY  (processevent_id)
) ENGINE=InnoDB  COMMENT='Historical Icinga process events';

-- --------------------------------------------------------

--
-- Table structure for table icinga_programstatus
--

CREATE TABLE IF NOT EXISTS icinga_programstatus (
  programstatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  program_version varchar(64) character set latin1 collate latin1_general_cs default NULL,
  status_update_time timestamp NULL,
  program_start_time timestamp NULL,
  program_end_time timestamp NULL,
  endpoint_name varchar(255) character set latin1 collate latin1_general_cs default NULL,
  is_currently_running smallint default 0,
  process_id bigint unsigned default 0,
  daemon_mode smallint default 0,
  last_command_check timestamp NULL,
  last_log_rotation timestamp NULL,
  notifications_enabled smallint default 0,
  disable_notif_expire_time timestamp NULL,
  active_service_checks_enabled smallint default 0,
  passive_service_checks_enabled smallint default 0,
  active_host_checks_enabled smallint default 0,
  passive_host_checks_enabled smallint default 0,
  event_handlers_enabled smallint default 0,
  flap_detection_enabled smallint default 0,
  failure_prediction_enabled smallint default 0,
  process_performance_data smallint default 0,
  obsess_over_hosts smallint default 0,
  obsess_over_services smallint default 0,
  modified_host_attributes  int default 0,
  modified_service_attributes  int default 0,
  global_host_event_handler TEXT character set latin1,
  global_service_event_handler TEXT character set latin1,
  config_dump_in_progress smallint default 0,
  PRIMARY KEY  (programstatus_id),
  UNIQUE KEY instance_id (instance_id)
) ENGINE=InnoDB  COMMENT='Current program status information';

-- --------------------------------------------------------

--
-- Table structure for table icinga_runtimevariables
--

CREATE TABLE IF NOT EXISTS icinga_runtimevariables (
  runtimevariable_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  varname varchar(64) character set latin1  default '',
  varvalue TEXT character set latin1,
  PRIMARY KEY  (runtimevariable_id)
) ENGINE=InnoDB  COMMENT='Runtime variables from the Icinga daemon';

-- --------------------------------------------------------

--
-- Table structure for table icinga_scheduleddowntime
--

CREATE TABLE IF NOT EXISTS icinga_scheduleddowntime (
  scheduleddowntime_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  downtime_type smallint default 0,
  object_id bigint unsigned default 0,
  entry_time timestamp NULL,
  author_name varchar(64) character set latin1  default '',
  comment_data TEXT character set latin1,
  internal_downtime_id bigint unsigned default 0,
  triggered_by_id bigint unsigned default 0,
  is_fixed smallint default 0,
  duration bigint(20) default 0,
  scheduled_start_time timestamp NULL,
  scheduled_end_time timestamp NULL,
  was_started smallint default 0,
  actual_start_time timestamp NULL,
  actual_start_time_usec  int default 0,
  is_in_effect smallint default 0,
  trigger_time timestamp NULL,
  name TEXT character set latin1 default NULL,
  session_token int default NULL,
  PRIMARY KEY  (scheduleddowntime_id)
) ENGINE=InnoDB COMMENT='Current scheduled host and service downtime';

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicechecks
--

CREATE TABLE IF NOT EXISTS icinga_servicechecks (
  servicecheck_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  service_object_id bigint unsigned default 0,
  check_type smallint default 0,
  current_check_attempt smallint default 0,
  max_check_attempts smallint default 0,
  state smallint default 0,
  state_type smallint default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  command_object_id bigint unsigned default 0,
  command_args TEXT character set latin1,
  command_line TEXT character set latin1,
  timeout smallint default 0,
  early_timeout smallint default 0,
  execution_time double  default '0',
  latency double  default '0',
  return_code smallint default 0,
  output TEXT character set latin1,
  long_output TEXT,
  perfdata TEXT character set latin1,
  PRIMARY KEY  (servicecheck_id)
) ENGINE=InnoDB  COMMENT='Historical service checks';

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicedependencies
--

CREATE TABLE IF NOT EXISTS icinga_servicedependencies (
  servicedependency_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  service_object_id bigint unsigned default 0,
  dependent_service_object_id bigint unsigned default 0,
  dependency_type smallint default 0,
  inherits_parent smallint default 0,
  timeperiod_object_id bigint unsigned default 0,
  fail_on_ok smallint default 0,
  fail_on_warning smallint default 0,
  fail_on_unknown smallint default 0,
  fail_on_critical smallint default 0,
  PRIMARY KEY  (servicedependency_id),
  KEY instance_id (instance_id,config_type,service_object_id,dependent_service_object_id,dependency_type,inherits_parent,fail_on_ok,fail_on_warning,fail_on_unknown,fail_on_critical)
) ENGINE=InnoDB COMMENT='Service dependency definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalations
--

CREATE TABLE IF NOT EXISTS icinga_serviceescalations (
  serviceescalation_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  service_object_id bigint unsigned default 0,
  timeperiod_object_id bigint unsigned default 0,
  first_notification smallint default 0,
  last_notification smallint default 0,
  notification_interval double  default '0',
  escalate_on_recovery smallint default 0,
  escalate_on_warning smallint default 0,
  escalate_on_unknown smallint default 0,
  escalate_on_critical smallint default 0,
  PRIMARY KEY  (serviceescalation_id),
  UNIQUE KEY instance_id (instance_id,config_type,service_object_id,timeperiod_object_id,first_notification,last_notification)
) ENGINE=InnoDB  COMMENT='Service escalation definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contactgroups
--

CREATE TABLE IF NOT EXISTS icinga_serviceescalation_contactgroups (
  serviceescalation_contactgroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  serviceescalation_id bigint unsigned default 0,
  contactgroup_object_id bigint unsigned default 0,
  PRIMARY KEY  (serviceescalation_contactgroup_id),
  UNIQUE KEY instance_id (serviceescalation_id,contactgroup_object_id)
) ENGINE=InnoDB  COMMENT='Service escalation contact groups';

-- --------------------------------------------------------

--
-- Table structure for table icinga_serviceescalation_contacts
--

CREATE TABLE IF NOT EXISTS icinga_serviceescalation_contacts (
  serviceescalation_contact_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  serviceescalation_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  PRIMARY KEY  (serviceescalation_contact_id),
  UNIQUE KEY instance_id (instance_id,serviceescalation_id,contact_object_id)
) ENGINE=InnoDB  COMMENT='Service escalation contacts';

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroups
--

CREATE TABLE IF NOT EXISTS icinga_servicegroups (
  servicegroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  servicegroup_object_id bigint unsigned default 0,
  alias varchar(255) character set latin1  default '',
  notes TEXT character set latin1  default NULL,
  notes_url TEXT character set latin1  default NULL,
  action_url TEXT character set latin1  default NULL,
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (servicegroup_id),
  UNIQUE KEY instance_id (instance_id,config_type,servicegroup_object_id)
) ENGINE=InnoDB  COMMENT='Servicegroup definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicegroup_members
--

CREATE TABLE IF NOT EXISTS icinga_servicegroup_members (
  servicegroup_member_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  servicegroup_id bigint unsigned default 0,
  service_object_id bigint unsigned default 0,
  PRIMARY KEY  (servicegroup_member_id)
) ENGINE=InnoDB  COMMENT='Servicegroup members';

-- --------------------------------------------------------

--
-- Table structure for table icinga_services
--

CREATE TABLE IF NOT EXISTS icinga_services (
  service_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  host_object_id bigint unsigned default 0,
  service_object_id bigint unsigned default 0,
  display_name varchar(255) character set latin1 collate latin1_general_cs  default '',
  check_command_object_id bigint unsigned default 0,
  check_command_args TEXT character set latin1,
  eventhandler_command_object_id bigint unsigned default 0,
  eventhandler_command_args TEXT character set latin1,
  notification_timeperiod_object_id bigint unsigned default 0,
  check_timeperiod_object_id bigint unsigned default 0,
  failure_prediction_options varchar(64) character set latin1  default '',
  check_interval double  default '0',
  retry_interval double  default '0',
  max_check_attempts smallint default 0,
  first_notification_delay double  default '0',
  notification_interval double  default '0',
  notify_on_warning smallint default 0,
  notify_on_unknown smallint default 0,
  notify_on_critical smallint default 0,
  notify_on_recovery smallint default 0,
  notify_on_flapping smallint default 0,
  notify_on_downtime smallint default 0,
  stalk_on_ok smallint default 0,
  stalk_on_warning smallint default 0,
  stalk_on_unknown smallint default 0,
  stalk_on_critical smallint default 0,
  is_volatile smallint default 0,
  flap_detection_enabled smallint default 0,
  flap_detection_on_ok smallint default 0,
  flap_detection_on_warning smallint default 0,
  flap_detection_on_unknown smallint default 0,
  flap_detection_on_critical smallint default 0,
  low_flap_threshold double  default '0',
  high_flap_threshold double  default '0',
  process_performance_data smallint default 0,
  freshness_checks_enabled smallint default 0,
  freshness_threshold int default 0,
  passive_checks_enabled smallint default 0,
  event_handler_enabled smallint default 0,
  active_checks_enabled smallint default 0,
  retain_status_information smallint default 0,
  retain_nonstatus_information smallint default 0,
  notifications_enabled smallint default 0,
  obsess_over_service smallint default 0,
  failure_prediction_enabled smallint default 0,
  notes TEXT character set latin1,
  notes_url TEXT character set latin1,
  action_url TEXT character set latin1,
  icon_image TEXT character set latin1,
  icon_image_alt TEXT character set latin1,
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (service_id),
  UNIQUE KEY instance_id (instance_id,config_type,service_object_id),
  KEY service_object_id (service_object_id)
) ENGINE=InnoDB  COMMENT='Service definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_servicestatus
--

CREATE TABLE IF NOT EXISTS icinga_servicestatus (
  servicestatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  service_object_id bigint unsigned default 0,
  status_update_time timestamp NULL,
  output TEXT character set latin1,
  long_output TEXT,
  perfdata TEXT character set latin1,
  check_source varchar(255) character set latin1  default '',
  current_state smallint default 0,
  has_been_checked smallint default 0,
  should_be_scheduled smallint default 0,
  current_check_attempt smallint default 0,
  max_check_attempts smallint default 0,
  last_check timestamp NULL,
  next_check timestamp NULL,
  check_type smallint default 0,
  last_state_change timestamp NULL,
  last_hard_state_change timestamp NULL,
  last_hard_state smallint default 0,
  last_time_ok timestamp NULL,
  last_time_warning timestamp NULL,
  last_time_unknown timestamp NULL,
  last_time_critical timestamp NULL,
  state_type smallint default 0,
  last_notification timestamp NULL,
  next_notification timestamp NULL,
  no_more_notifications smallint default 0,
  notifications_enabled smallint default 0,
  problem_has_been_acknowledged smallint default 0,
  acknowledgement_type smallint default 0,
  current_notification_number int unsigned default 0,
  passive_checks_enabled smallint default 0,
  active_checks_enabled smallint default 0,
  event_handler_enabled smallint default 0,
  flap_detection_enabled smallint default 0,
  is_flapping smallint default 0,
  percent_state_change double  default '0',
  latency double  default '0',
  execution_time double  default '0',
  scheduled_downtime_depth smallint default 0,
  failure_prediction_enabled smallint default 0,
  process_performance_data smallint default 0,
  obsess_over_service smallint default 0,
  modified_service_attributes  int default 0,
  original_attributes TEXT character set latin1  default NULL,
  event_handler TEXT character set latin1,
  check_command TEXT character set latin1,
  normal_check_interval double  default '0',
  retry_check_interval double  default '0',
  check_timeperiod_object_id bigint unsigned default 0,
  is_reachable smallint default 0,
  PRIMARY KEY  (servicestatus_id),
  UNIQUE KEY object_id (service_object_id)
) ENGINE=InnoDB  COMMENT='Current service status information';

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contactgroups
--

CREATE TABLE IF NOT EXISTS icinga_service_contactgroups (
  service_contactgroup_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  service_id bigint unsigned default 0,
  contactgroup_object_id bigint unsigned default 0,
  PRIMARY KEY  (service_contactgroup_id)
) ENGINE=InnoDB  COMMENT='Service contact groups';

-- --------------------------------------------------------

--
-- Table structure for table icinga_service_contacts
--

CREATE TABLE IF NOT EXISTS icinga_service_contacts (
  service_contact_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  service_id bigint unsigned default 0,
  contact_object_id bigint unsigned default 0,
  PRIMARY KEY  (service_contact_id)
) ENGINE=InnoDB  COMMENT='Service contacts';

-- --------------------------------------------------------

--
-- Table structure for table icinga_statehistory
--

CREATE TABLE IF NOT EXISTS icinga_statehistory (
  statehistory_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  state_time timestamp NULL,
  state_time_usec  int default 0,
  object_id bigint unsigned default 0,
  state_change smallint default 0,
  state smallint default 0,
  state_type smallint default 0,
  current_check_attempt smallint default 0,
  max_check_attempts smallint default 0,
  last_state smallint default 0,
  last_hard_state smallint default 0,
  output TEXT character set latin1,
  long_output TEXT,
  check_source varchar(255) character set latin1 default NULL,
  PRIMARY KEY  (statehistory_id)
) ENGINE=InnoDB COMMENT='Historical host and service state changes';

-- --------------------------------------------------------

--
-- Table structure for table icinga_systemcommands
--

CREATE TABLE IF NOT EXISTS icinga_systemcommands (
  systemcommand_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  start_time timestamp NULL,
  start_time_usec  int default 0,
  end_time timestamp NULL,
  end_time_usec  int default 0,
  command_line TEXT character set latin1,
  timeout smallint default 0,
  early_timeout smallint default 0,
  execution_time double  default '0',
  return_code smallint default 0,
  output TEXT character set latin1,
  long_output TEXT,
  PRIMARY KEY  (systemcommand_id),
  UNIQUE KEY instance_id (instance_id,start_time,start_time_usec)
) ENGINE=InnoDB  COMMENT='Historical system commands that are executed';

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiods
--

CREATE TABLE IF NOT EXISTS icinga_timeperiods (
  timeperiod_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  config_type smallint default 0,
  timeperiod_object_id bigint unsigned default 0,
  alias varchar(255) character set latin1  default '',
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (timeperiod_id),
  UNIQUE KEY instance_id (instance_id,config_type,timeperiod_object_id)
) ENGINE=InnoDB  COMMENT='Timeperiod definitions';

-- --------------------------------------------------------

--
-- Table structure for table icinga_timeperiod_timeranges
--

CREATE TABLE IF NOT EXISTS icinga_timeperiod_timeranges (
  timeperiod_timerange_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  timeperiod_id bigint unsigned default 0,
  day smallint default 0,
  start_sec  int default 0,
  end_sec  int default 0,
  PRIMARY KEY  (timeperiod_timerange_id)
) ENGINE=InnoDB  COMMENT='Timeperiod definitions';


-- --------------------------------------------------------
-- Icinga 2 specific schema extensions
-- --------------------------------------------------------

--
-- Table structure for table icinga_endpoints
--

CREATE TABLE IF NOT EXISTS icinga_endpoints (
  endpoint_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  endpoint_object_id bigint(20) unsigned DEFAULT '0',
  zone_object_id bigint(20) unsigned DEFAULT '0',
  config_type smallint(6) DEFAULT '0',
  identity varchar(255) DEFAULT NULL,
  node varchar(255) DEFAULT NULL,
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (endpoint_id)
) ENGINE=InnoDB COMMENT='Endpoint configuration';

-- --------------------------------------------------------

--
-- Table structure for table icinga_endpointstatus
--

CREATE TABLE IF NOT EXISTS icinga_endpointstatus (
  endpointstatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  endpoint_object_id bigint(20) unsigned DEFAULT '0',
  zone_object_id bigint(20) unsigned DEFAULT '0',
  status_update_time timestamp NULL,
  identity varchar(255) DEFAULT NULL,
  node varchar(255) DEFAULT NULL,
  is_connected smallint(6),
  PRIMARY KEY  (endpointstatus_id)
) ENGINE=InnoDB COMMENT='Endpoint status';

--
-- Table structure for table icinga_zones
--

CREATE TABLE IF NOT EXISTS icinga_zones (
  zone_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  zone_object_id bigint(20) unsigned DEFAULT '0',
  config_type smallint(6) DEFAULT '0',
  parent_zone_object_id bigint(20) unsigned DEFAULT '0',
  is_global smallint(6),
  config_hash varchar(64) DEFAULT NULL,
  PRIMARY KEY  (zone_id)
) ENGINE=InnoDB COMMENT='Zone configuration';

-- --------------------------------------------------------

--
-- Table structure for table icinga_zonestatus
--

CREATE TABLE IF NOT EXISTS icinga_zonestatus (
  zonestatus_id bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  instance_id bigint unsigned default 0,
  zone_object_id bigint(20) unsigned DEFAULT '0',
  status_update_time timestamp NULL,
  parent_zone_object_id bigint(20) unsigned DEFAULT '0',
  PRIMARY KEY  (zonestatus_id)
) ENGINE=InnoDB COMMENT='Zone status';




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

-- servicestatus
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
CREATE INDEX loge_inst_id_time_idx on icinga_logentries (instance_id ASC, logentry_time DESC);

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

-- statehistory
CREATE INDEX statehist_i_id_o_id_s_ty_s_ti on icinga_statehistory(instance_id, object_id, state_type, state_time);
-- #2274
create index statehist_state_idx on icinga_statehistory(object_id,state);


-- Icinga Web Notifications
CREATE INDEX notification_idx ON icinga_notifications(notification_type, object_id, start_time);
CREATE INDEX notification_object_id_idx ON icinga_notifications(object_id);
CREATE INDEX contact_notification_idx ON icinga_contactnotifications(notification_id, contact_object_id);
CREATE INDEX contacts_object_id_idx ON icinga_contacts(contact_object_id);
CREATE INDEX contact_notif_meth_notif_idx ON icinga_contactnotificationmethods(contactnotification_id, command_object_id);
CREATE INDEX command_object_idx ON icinga_commands(object_id);
CREATE INDEX services_combined_object_idx ON icinga_services(service_object_id, host_object_id);


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
create index idx_downtimehistory_remove on icinga_downtimehistory (object_id, entry_time, scheduled_start_time, scheduled_end_time);
create index idx_scheduleddowntime_remove on icinga_scheduleddowntime (object_id, entry_time, scheduled_start_time, scheduled_end_time);

-- #5492
CREATE INDEX idx_commenthistory_remove ON icinga_commenthistory (object_id, entry_time);
CREATE INDEX idx_comments_remove ON icinga_comments (object_id, entry_time);

-- -----------------------------------------
-- set dbversion
-- -----------------------------------------
INSERT INTO icinga_dbversion (name, version, create_time, modify_time) VALUES ('idoutils', '1.14.3', NOW(), NOW())
ON DUPLICATE KEY UPDATE version='1.14.3', modify_time=NOW();


