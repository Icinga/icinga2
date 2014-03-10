

-- -----------------------------------------
-- #5612
-- -----------------------------------------
ALTER TABLE icinga_statehistory ADD COLUMN check_source text default NULL;

-- --------------------------------------------------------
-- Icinga 2 specific schema extensions
-- --------------------------------------------------------

--
-- Table structure for table icinga_endpoints
--

CREATE TABLE IF NOT EXISTS icinga_endpoints (
  endpoint_id bigserial,
  instance_id bigint default 0,
  endpoint_object_id bigint default 0,
  config_type integer default 0,
  identity text DEFAULT NULL,
  node text DEFAULT NULL,
  CONSTRAINT PK_endpoint_id PRIMARY KEY (endpoint_id) ,
  CONSTRAINT UQ_endpoints UNIQUE (instance_id,config_type,endpoint_object_id)
) ;

-- --------------------------------------------------------

--
-- Table structure for table icinga_endpointstatus
--

CREATE TABLE IF NOT EXISTS icinga_endpointstatus (
  endpointstatus_id bigserial,
  instance_id bigint default 0,
  endpoint_object_id bigint default 0,
  status_update_time timestamp with time zone default '1970-01-01 00:00:00',
  identity text DEFAULT NULL,
  node text DEFAULT NULL,
  is_connected integer default 0,
  CONSTRAINT PK_endpointstatus_id PRIMARY KEY (endpointstatus_id) ,
  CONSTRAINT UQ_endpointstatus UNIQUE (endpoint_object_id)
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
-- set dbversion
-- -----------------------------------------

SELECT updatedbversion('1.11.0');

