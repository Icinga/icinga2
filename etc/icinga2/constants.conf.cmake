/**
 * This file defines global constants which can be used in
 * the other configuration files.
 */

/* Array of directories which can contain plugins from the Monitoring Plugins
 * project or other sources.
 */
/*
const PluginPath = @ICINGA2_PLUGINPATH_ESCAPED@
*/

/* Default path to find plugins from the Monitoring Plugins project.
 *
 * This value is no longer used by the ITL in Icinga >= 2.9.0!
 */
/*
const PluginDir = "@ICINGA2_PLUGINDIR@"
*/

/* The directory which contains the Manubulon plugins.
 * Check the documentation, chapter "SNMP Manubulon Plugin Check Commands", for details.
 *
 * This value is no longer used by the ITL in Icinga >= 2.9.0!
 */
/*
const ManubulonPluginDir = "@ICINGA2_PLUGINDIR@"
*/

/* The directory which you use to store additional plugins which ITL provides user contributed command definitions for.
 * Check the documentation, chapter "Plugins Contribution", for details.
 *
 * This value is no longer used by the ITL in Icinga >= 2.9.0!
 */
/*
const PluginContribDir = "@ICINGA2_PLUGINDIR@"
*/

/* Our local instance name. By default this is the server's hostname as returned by `hostname --fqdn`.
 * This should be the common name from the API certificate.
 */
//const NodeName = "localhost"

/* Our local zone name. */
const ZoneName = NodeName

/* Secret key for remote node tickets */
const TicketSalt = ""
