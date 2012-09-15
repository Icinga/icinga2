/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef COMPATIDOPROTOAPI_H
#define COMPATIDOPROTOAPI_H

/*
 * PROTOCOL VERSION 
 */

#define IDO_API_PROTOVERSION						  2


/*
 * CONTROL STRINGS 
 */

#define IDO_API_NONE								  ""
#define IDO_API_HELLO                                "HELLO"
#define IDO_API_GOODBYE                              "GOODBYE"
#define IDO_API_PROTOCOL                             "PROTOCOL"
#define IDO_API_AGENT                                "AGENT"
#define IDO_API_AGENTVERSION                         "AGENTVERSION"
#define IDO_API_DISPOSITION                          "DISPOSITION"  /* archived or realtime */
#define IDO_API_CONNECTION                           "CONNECTION"   /* immediate or deferred */
#define IDO_API_CONNECTTYPE                          "CONNECTTYPE"  /* initial or reconnection */
#define IDO_API_DISPOSITION_ARCHIVED                 "ARCHIVED"
#define IDO_API_DISPOSITION_REALTIME                 "REALTIME"
#define IDO_API_CONNECTION_FILE                      "FILE"
#define IDO_API_CONNECTION_UNIXSOCKET                "UNIXSOCKET"
#define IDO_API_CONNECTION_TCPSOCKET                 "TCPSOCKET"
#define IDO_API_CONNECTTYPE_INITIAL                  "INITIAL"
#define IDO_API_CONNECTTYPE_RECONNECT                "RECONNECT"
#define IDO_API_STARTDATADUMP                        "STARTDATADUMP"
#define IDO_API_STARTTIME                            "STARTTIME"
#define IDO_API_ENDTIME                              "ENDTIME"
#define IDO_API_CONFIGDUMP_ORIGINAL                  "ORIGINAL"
#define IDO_API_CONFIGDUMP_RETAINED                  "RETAINED"
#define IDO_API_INSTANCENAME                         "INSTANCENAME"

#define IDO_API_STARTCONFIGDUMP                      900
#define IDO_API_ENDCONFIGDUMP                        901
#define IDO_API_ENDDATA                              999
#define IDO_API_ENDDATADUMP                          1000

/*
 * DATA TYPES
 */

#define IDO_API_LOGENTRY                             100

#define IDO_API_PROCESSDATA                          200
#define IDO_API_TIMEDEVENTDATA                       201
#define IDO_API_LOGDATA                              202
#define IDO_API_SYSTEMCOMMANDDATA                    203
#define IDO_API_EVENTHANDLERDATA                     204
#define IDO_API_NOTIFICATIONDATA                     205
#define IDO_API_SERVICECHECKDATA                     206
#define IDO_API_HOSTCHECKDATA                        207
#define IDO_API_COMMENTDATA                          208
#define IDO_API_DOWNTIMEDATA                         209
#define IDO_API_FLAPPINGDATA                         210
#define IDO_API_PROGRAMSTATUSDATA                    211
#define IDO_API_HOSTSTATUSDATA                       212
#define IDO_API_SERVICESTATUSDATA                    213
#define IDO_API_ADAPTIVEPROGRAMDATA                  214
#define IDO_API_ADAPTIVEHOSTDATA                     215
#define IDO_API_ADAPTIVESERVICEDATA                  216
#define IDO_API_EXTERNALCOMMANDDATA                  217
#define IDO_API_AGGREGATEDSTATUSDATA                 218
#define IDO_API_RETENTIONDATA                        219
#define IDO_API_CONTACTNOTIFICATIONDATA              220
#define IDO_API_CONTACTNOTIFICATIONMETHODDATA        221
#define IDO_API_ACKNOWLEDGEMENTDATA                  222
#define IDO_API_STATECHANGEDATA                      223
#define IDO_API_CONTACTSTATUSDATA                    224
#define IDO_API_ADAPTIVECONTACTDATA                  225

#define IDO_API_MAINCONFIGFILEVARIABLES              300
#define IDO_API_RESOURCECONFIGFILEVARIABLES          301
#define IDO_API_CONFIGVARIABLES                      302
#define IDO_API_RUNTIMEVARIABLES                     303

#define IDO_API_HOSTDEFINITION                       400
#define IDO_API_HOSTGROUPDEFINITION                  401
#define IDO_API_SERVICEDEFINITION                    402
#define IDO_API_SERVICEGROUPDEFINITION               403
#define IDO_API_HOSTDEPENDENCYDEFINITION             404
#define IDO_API_SERVICEDEPENDENCYDEFINITION          405
#define IDO_API_HOSTESCALATIONDEFINITION             406
#define IDO_API_SERVICEESCALATIONDEFINITION          407
#define IDO_API_COMMANDDEFINITION                    408
#define IDO_API_TIMEPERIODDEFINITION                 409
#define IDO_API_CONTACTDEFINITION                    410
#define IDO_API_CONTACTGROUPDEFINITION               411

/* 
 * COMMON DATA ATTRIBUTES
 */

#define IDO_MAX_DATA_TYPES                           270

#define IDO_DATA_NONE                                0

#define IDO_DATA_TYPE                                1
#define IDO_DATA_FLAGS                               2
#define IDO_DATA_ATTRIBUTES                          3
#define IDO_DATA_TIMESTAMP                           4

/*
 * LIVE DATA ATTRIBUTES
 */

#define IDO_DATA_ACKAUTHOR                           5
#define IDO_DATA_ACKDATA                             6
#define IDO_DATA_ACKNOWLEDGEMENTTYPE                 7
#define IDO_DATA_ACTIVEHOSTCHECKSENABLED             8
#define IDO_DATA_ACTIVESERVICECHECKSENABLED          9
#define IDO_DATA_AUTHORNAME                          10
#define IDO_DATA_CHECKCOMMAND                        11
#define IDO_DATA_CHECKTYPE                           12
#define IDO_DATA_COMMANDARGS                         13
#define IDO_DATA_COMMANDLINE                         14
#define IDO_DATA_COMMANDSTRING                       15
#define IDO_DATA_COMMANDTYPE                         16
#define IDO_DATA_COMMENT                             17
#define IDO_DATA_COMMENTID                           18
#define IDO_DATA_COMMENTTIME                         19
#define IDO_DATA_COMMENTTYPE                         20
#define IDO_DATA_CONFIGFILENAME                      21
#define IDO_DATA_CONFIGFILEVARIABLE                  22
#define IDO_DATA_CONFIGVARIABLE                      23
#define IDO_DATA_CONTACTSNOTIFIED                    24
#define IDO_DATA_CURRENTCHECKATTEMPT                 25
#define IDO_DATA_CURRENTNOTIFICATIONNUMBER           26
#define IDO_DATA_CURRENTSTATE                        27
#define IDO_DATA_DAEMONMODE                          28
#define IDO_DATA_DOWNTIMEID                          29
#define IDO_DATA_DOWNTIMETYPE                        30
#define IDO_DATA_DURATION                            31
#define IDO_DATA_EARLYTIMEOUT                        32
#define IDO_DATA_ENDTIME                             33
#define IDO_DATA_ENTRYTIME                           34
#define IDO_DATA_ENTRYTYPE                           35
#define IDO_DATA_ESCALATED                           36
#define IDO_DATA_EVENTHANDLER                        37
#define IDO_DATA_EVENTHANDLERENABLED                 38
#define IDO_DATA_EVENTHANDLERSENABLED                39
#define IDO_DATA_EVENTHANDLERTYPE                    40
#define IDO_DATA_EVENTTYPE                           41
#define IDO_DATA_EXECUTIONTIME                       42
#define IDO_DATA_EXPIRATIONTIME                      43
#define IDO_DATA_EXPIRES                             44
#define IDO_DATA_FAILUREPREDICTIONENABLED            45
#define IDO_DATA_FIXED                               46
#define IDO_DATA_FLAPDETECTIONENABLED                47
#define IDO_DATA_FLAPPINGTYPE                        48
#define IDO_DATA_GLOBALHOSTEVENTHANDLER              49
#define IDO_DATA_GLOBALSERVICEEVENTHANDLER           50
#define IDO_DATA_HASBEENCHECKED                      51
#define IDO_DATA_HIGHTHRESHOLD                       52
#define IDO_DATA_HOST                                53
#define IDO_DATA_ISFLAPPING                          54
#define IDO_DATA_LASTCOMMANDCHECK                    55
#define IDO_DATA_LASTHARDSTATE                       56
#define IDO_DATA_LASTHARDSTATECHANGE                 57
#define IDO_DATA_LASTHOSTCHECK                       58
#define IDO_DATA_LASTHOSTNOTIFICATION                59
#define IDO_DATA_LASTLOGROTATION                     60
#define IDO_DATA_LASTSERVICECHECK                    61
#define IDO_DATA_LASTSERVICENOTIFICATION             62
#define IDO_DATA_LASTSTATECHANGE                     63
#define IDO_DATA_LASTTIMECRITICAL                    64
#define IDO_DATA_LASTTIMEDOWN                        65
#define IDO_DATA_LASTTIMEOK                          66
#define IDO_DATA_LASTTIMEUNKNOWN                     67
#define IDO_DATA_LASTTIMEUNREACHABLE                 68
#define IDO_DATA_LASTTIMEUP                          69
#define IDO_DATA_LASTTIMEWARNING                     70
#define IDO_DATA_LATENCY                             71
#define IDO_DATA_LOGENTRY                            72
#define IDO_DATA_LOGENTRYTIME                        73
#define IDO_DATA_LOGENTRYTYPE                        74
#define IDO_DATA_LOWTHRESHOLD                        75
#define IDO_DATA_MAXCHECKATTEMPTS                    76
#define IDO_DATA_MODIFIEDHOSTATTRIBUTE               77
#define IDO_DATA_MODIFIEDHOSTATTRIBUTES              78
#define IDO_DATA_MODIFIEDSERVICEATTRIBUTE            79
#define IDO_DATA_MODIFIEDSERVICEATTRIBUTES           80
#define IDO_DATA_NEXTHOSTCHECK                       81
#define IDO_DATA_NEXTHOSTNOTIFICATION                82
#define IDO_DATA_NEXTSERVICECHECK                    83
#define IDO_DATA_NEXTSERVICENOTIFICATION             84
#define IDO_DATA_NOMORENOTIFICATIONS                 85
#define IDO_DATA_NORMALCHECKINTERVAL                 86
#define IDO_DATA_NOTIFICATIONREASON                  87
#define IDO_DATA_NOTIFICATIONSENABLED                88
#define IDO_DATA_NOTIFICATIONTYPE                    89
#define IDO_DATA_NOTIFYCONTACTS                      90
#define IDO_DATA_OBSESSOVERHOST                      91
#define IDO_DATA_OBSESSOVERHOSTS                     92
#define IDO_DATA_OBSESSOVERSERVICE                   93
#define IDO_DATA_OBSESSOVERSERVICES                  94
#define IDO_DATA_OUTPUT                              95
#define IDO_DATA_PASSIVEHOSTCHECKSENABLED            96
#define IDO_DATA_PASSIVESERVICECHECKSENABLED         97
#define IDO_DATA_PERCENTSTATECHANGE                  98
#define IDO_DATA_PERFDATA                            99
#define IDO_DATA_PERSISTENT                          100
#define IDO_DATA_PROBLEMHASBEENACKNOWLEDGED          101
#define IDO_DATA_PROCESSID                           102
#define IDO_DATA_PROCESSPERFORMANCEDATA              103
#define IDO_DATA_PROGRAMDATE                         104
#define IDO_DATA_PROGRAMNAME                         105
#define IDO_DATA_PROGRAMSTARTTIME                    106
#define IDO_DATA_PROGRAMVERSION                      107
#define IDO_DATA_RECURRING                           108
#define IDO_DATA_RETRYCHECKINTERVAL                  109
#define IDO_DATA_RETURNCODE                          110
#define IDO_DATA_RUNTIME                             111
#define IDO_DATA_RUNTIMEVARIABLE                     112
#define IDO_DATA_SCHEDULEDDOWNTIMEDEPTH              113
#define IDO_DATA_SERVICE                             114
#define IDO_DATA_SHOULDBESCHEDULED                   115
#define IDO_DATA_SOURCE                              116
#define IDO_DATA_STARTTIME                           117
#define IDO_DATA_STATE                               118
#define IDO_DATA_STATECHANGE                         119
#define IDO_DATA_STATECHANGETYPE                     120
#define IDO_DATA_STATETYPE                           121
#define IDO_DATA_STICKY                              122
#define IDO_DATA_TIMEOUT                             123
#define IDO_DATA_TRIGGEREDBY                         124
#define IDO_DATA_LONGOUTPUT                          125

/*
 * OBJECT CONFIG DATA ATTRIBUTES
 */

#define IDO_DATA_ACTIONURL                           126
#define IDO_DATA_COMMANDNAME                         127
#define IDO_DATA_CONTACTADDRESS                      128
#define IDO_DATA_CONTACTALIAS                        129
#define IDO_DATA_CONTACTGROUP                        130
#define IDO_DATA_CONTACTGROUPALIAS                   131
#define IDO_DATA_CONTACTGROUPMEMBER                  132
#define IDO_DATA_CONTACTGROUPNAME                    133
#define IDO_DATA_CONTACTNAME                         134
#define IDO_DATA_DEPENDENCYTYPE                      135
#define IDO_DATA_DEPENDENTHOSTNAME                   136
#define IDO_DATA_DEPENDENTSERVICEDESCRIPTION         137
#define IDO_DATA_EMAILADDRESS                        138
#define IDO_DATA_ESCALATEONCRITICAL                  139
#define IDO_DATA_ESCALATEOIDOWN                      140
#define IDO_DATA_ESCALATEONRECOVERY                  141
#define IDO_DATA_ESCALATEONUNKNOWN                   142
#define IDO_DATA_ESCALATEONUNREACHABLE               143
#define IDO_DATA_ESCALATEONWARNING                   144
#define IDO_DATA_ESCALATIONPERIOD                    145
#define IDO_DATA_FAILONCRITICAL                      146
#define IDO_DATA_FAILOIDOWN                          147
#define IDO_DATA_FAILONOK                            148
#define IDO_DATA_FAILONUNKNOWN                       149
#define IDO_DATA_FAILONUNREACHABLE                   150
#define IDO_DATA_FAILONUP                            151
#define IDO_DATA_FAILONWARNING                       152
#define IDO_DATA_FIRSTNOTIFICATION                   153
#define IDO_DATA_HAVE2DCOORDS                        154
#define IDO_DATA_HAVE3DCOORDS                        155
#define IDO_DATA_HIGHHOSTFLAPTHRESHOLD               156
#define IDO_DATA_HIGHSERVICEFLAPTHRESHOLD            157
#define IDO_DATA_HOSTADDRESS                         158
#define IDO_DATA_HOSTALIAS                           159
#define IDO_DATA_HOSTCHECKCOMMAND                    160
#define IDO_DATA_HOSTCHECKINTERVAL                   161
#define IDO_DATA_HOSTCHECKPERIOD                     162
#define IDO_DATA_HOSTEVENTHANDLER                    163
#define IDO_DATA_HOSTEVENTHANDLERENABLED             164
#define IDO_DATA_HOSTFAILUREPREDICTIONENABLED        165
#define IDO_DATA_HOSTFAILUREPREDICTIONOPTIONS        166
#define IDO_DATA_HOSTFLAPDETECTIONENABLED            167
#define IDO_DATA_HOSTFRESHNESSCHECKSENABLED          168
#define IDO_DATA_HOSTFRESHNESSTHRESHOLD              169
#define IDO_DATA_HOSTGROUPALIAS                      170
#define IDO_DATA_HOSTGROUPMEMBER                     171
#define IDO_DATA_HOSTGROUPNAME                       172
#define IDO_DATA_HOSTMAXCHECKATTEMPTS                173
#define IDO_DATA_HOSTNAME                            174
#define IDO_DATA_HOSTNOTIFICATIONCOMMAND             175
#define IDO_DATA_HOSTNOTIFICATIONINTERVAL            176
#define IDO_DATA_HOSTNOTIFICATIONPERIOD              177
#define IDO_DATA_HOSTNOTIFICATIONSENABLED            178
#define IDO_DATA_ICONIMAGE                           179
#define IDO_DATA_ICONIMAGEALT                        180
#define IDO_DATA_INHERITSPARENT                      181
#define IDO_DATA_LASTNOTIFICATION                    182
#define IDO_DATA_LOWHOSTFLAPTHRESHOLD                183
#define IDO_DATA_LOWSERVICEFLAPTHRESHOLD             184
#define IDO_DATA_MAXSERVICECHECKATTEMPTS             185
#define IDO_DATA_NOTES                               186
#define IDO_DATA_NOTESURL                            187
#define IDO_DATA_NOTIFICATIONINTERVAL                188
#define IDO_DATA_NOTIFYHOSTDOWN                      189
#define IDO_DATA_NOTIFYHOSTFLAPPING                  190
#define IDO_DATA_NOTIFYHOSTRECOVERY                  191
#define IDO_DATA_NOTIFYHOSTUNREACHABLE               192
#define IDO_DATA_NOTIFYSERVICECRITICAL               193
#define IDO_DATA_NOTIFYSERVICEFLAPPING               194
#define IDO_DATA_NOTIFYSERVICERECOVERY               195
#define IDO_DATA_NOTIFYSERVICEUNKNOWN                196
#define IDO_DATA_NOTIFYSERVICEWARNING                197
#define IDO_DATA_PAGERADDRESS                        198

#define IDO_DATA_PARENTHOST                          200
#define IDO_DATA_PROCESSHOSTPERFORMANCEDATA          201
#define IDO_DATA_PROCESSSERVICEPERFORMANCEDATA       202
#define IDO_DATA_RETAINHOSTNONSTATUSINFORMATION      203
#define IDO_DATA_RETAINHOSTSTATUSINFORMATION         204
#define IDO_DATA_RETAINSERVICENONSTATUSINFORMATION   205
#define IDO_DATA_RETAINSERVICESTATUSINFORMATION      206
#define IDO_DATA_SERVICECHECKCOMMAND                 207
#define IDO_DATA_SERVICECHECKINTERVAL                208
#define IDO_DATA_SERVICECHECKPERIOD                  209
#define IDO_DATA_SERVICEDESCRIPTION                  210
#define IDO_DATA_SERVICEEVENTHANDLER                 211
#define IDO_DATA_SERVICEEVENTHANDLERENABLED          212
#define IDO_DATA_SERVICEFAILUREPREDICTIONENABLED     213
#define IDO_DATA_SERVICEFAILUREPREDICTIONOPTIONS     214
#define IDO_DATA_SERVICEFLAPDETECTIONENABLED         215
#define IDO_DATA_SERVICEFRESHNESSCHECKSENABLED       216
#define IDO_DATA_SERVICEFRESHNESSTHRESHOLD           217
#define IDO_DATA_SERVICEGROUPALIAS                   218
#define IDO_DATA_SERVICEGROUPMEMBER                  219
#define IDO_DATA_SERVICEGROUPNAME                    220
#define IDO_DATA_SERVICEISVOLATILE                   221
#define IDO_DATA_SERVICENOTIFICATIONCOMMAND          222
#define IDO_DATA_SERVICENOTIFICATIONINTERVAL         223
#define IDO_DATA_SERVICENOTIFICATIONPERIOD           224
#define IDO_DATA_SERVICENOTIFICATIONSENABLED         225
#define IDO_DATA_SERVICERETRYINTERVAL                226

#define IDO_DATA_STALKHOSTOIDOWN                     228
#define IDO_DATA_STALKHOSTONUNREACHABLE              229
#define IDO_DATA_STALKHOSTONUP                       230
#define IDO_DATA_STALKSERVICEONCRITICAL              231
#define IDO_DATA_STALKSERVICEONOK                    232
#define IDO_DATA_STALKSERVICEONUNKNOWN               233
#define IDO_DATA_STALKSERVICEONWARNING               234
#define IDO_DATA_STATUSMAPIMAGE                      235
#define IDO_DATA_TIMEPERIODALIAS                     236
#define IDO_DATA_TIMEPERIODNAME                      237
#define IDO_DATA_TIMERANGE                           238
#define IDO_DATA_VRMLIMAGE                           239
#define IDO_DATA_X2D                                 240
#define IDO_DATA_X3D                                 241
#define IDO_DATA_Y2D                                 242
#define IDO_DATA_Y3D                                 243
#define IDO_DATA_Z3D                                 244

#define IDO_DATA_CONFIGDUMPTYPE                      245

#define IDO_DATA_FIRSTNOTIFICATIONDELAY              246
#define IDO_DATA_HOSTRETRYINTERVAL                   247
#define IDO_DATA_NOTIFYHOSTDOWNTIME                  248
#define IDO_DATA_NOTIFYSERVICEDOWNTIME               249
#define IDO_DATA_CANSUBMITCOMMANDS                   250
#define IDO_DATA_FLAPDETECTIONONUP                   251
#define IDO_DATA_FLAPDETECTIONOIDOWN                 252
#define IDO_DATA_FLAPDETECTIONONUNREACHABLE          253
#define IDO_DATA_FLAPDETECTIONONOK                   254
#define IDO_DATA_FLAPDETECTIONONWARNING              255
#define IDO_DATA_FLAPDETECTIONONUNKNOWN              256
#define IDO_DATA_FLAPDETECTIONONCRITICAL             257
#define IDO_DATA_DISPLAYNAME                         258
#define IDO_DATA_DEPENDENCYPERIOD                    259
#define IDO_DATA_MODIFIEDCONTACTATTRIBUTE            260    /* LIVE DATA */
#define IDO_DATA_MODIFIEDCONTACTATTRIBUTES           261    /* LIVE DATA */
#define IDO_DATA_CUSTOMVARIABLE                      262
#define IDO_DATA_HASBEENMODIFIED                     263
#define IDO_DATA_CONTACT                             264
#define IDO_DATA_LASTSTATE                           265

#define IDO_DATA_HOSTADDRESS6                        266
#define IDO_DATA_END_TIME                            267    /* ACKNOWLEDGEMENT DATA */
#define IDO_DATA_DOWNTIMEISINEFFECT                  268
#define IDO_DATA_DOWNTIMETRIGGERTIME                 269


#endif /* COMPATIDOPROTOAPI_H */
