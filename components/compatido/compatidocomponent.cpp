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

#include "i2-compatido.h"

using namespace icinga;

const String CompatIdoComponent::DefaultSocketAddress = "127.0.0.1";
const String CompatIdoComponent::DefaultSocketPort = "5668";
const String CompatIdoComponent::DefaultInstanceName = "i2-default";
const int CompatIdoComponent::DefaultReconnectInterval = 15;


/**
 * Reads the socket address from the config
 * @returns address The config option, or static default
 */
String CompatIdoComponent::GetSocketAddress(void) const
{
	Value address = GetConfig()->Get("socket_address");
	if(address.IsEmpty())
		return DefaultSocketAddress;
	else
		return address;
}


/**
 * Reads the socket port from the config
 * @returns port The config option, or static default
 */
String CompatIdoComponent::GetSocketPort(void) const
{
	Value port = GetConfig()->Get("socket_port");
	if(port.IsEmpty())
		return DefaultSocketPort;
	else
		return port;
}

/**
 * Reads the instance name from the config
 * @returns instance The config option, or static default
 */
String CompatIdoComponent::GetInstanceName(void) const
{
	Value instance = GetConfig()->Get("instance_name");
	if(instance.IsEmpty())
		return DefaultInstanceName;
	else
		return instance;
}

/**
 * Reads the reconnect interval from the config
 * @returns reconnect_interval The config option, or static default
 */
int CompatIdoComponent::GetReconnectInterval(void) const
{
        Value interval = GetConfig()->Get("reconnect_interval");
        if(interval.IsEmpty())
                return DefaultReconnectInterval;
        else   
                return interval;
}

/**
 * Sets config dump in progress state
 */
void CompatIdoComponent::SetConfigDumpInProgress(bool state)
{
	m_ConfigDumpInProgress = state;
}

/**
 * Get state of config in progress
 *
 * @returns state bis config dump in progress.
 */
bool CompatIdoComponent::GetConfigDumpInProgress(void)
{
	return m_ConfigDumpInProgress;
}

/**
 * Starts the component.
 */
void CompatIdoComponent::Start(void)
{
	const int StatusTimerInterval = 60;
	const int ConfigTimerInterval = 3600;
	const int ProgramStatusTimerInterval = 15;
	const int ReconnectTimerInterval = GetReconnectInterval();

	/* HINTS - XXX
	 * - only tcp sockets
	 * - only icinga idoutils 1.8
	 * - only "retained" config
	 */
	/*
	 * open ido socket once
	 */
	OpenIdoSocket();

	/* 
	 * tell ido2db that we just started
	 */
	SendStartProcess();

	/*
	 * ddump the config later (can't do that within start of the component)
	 */
	m_ConfigTimer = boost::make_shared<Timer>();
	m_ConfigTimer->SetInterval(ConfigTimerInterval);
	m_ConfigTimer->OnTimerExpired.connect(boost::bind(&CompatIdoComponent::ConfigTimerHandler, this));
	m_ConfigTimer->Start();
	m_ConfigTimer->Reschedule(0);


	/*
	 * initialize timers
	 */
	m_StatusTimer = boost::make_shared<Timer>();
	m_StatusTimer->SetInterval(StatusTimerInterval);
	m_StatusTimer->OnTimerExpired.connect(boost::bind(&CompatIdoComponent::StatusTimerHandler, this));
	m_StatusTimer->Start();

	/*
	 * do not dummp that asynchronous
	 * TODO do that on dynamic updates for objects
	 */
	/*
	m_ProgramStatusTimer = boost::make_shared<Timer>();
	m_ProgramStatusTimer->SetInterval(ProgramStatusTimerInterval);
	m_ProgramStatusTimer->OnTimerExpired.connect(boost::bind(&CompatIdoComponent::ProgramStatusTimerHandler, this));
	m_ProgramStatusTimer->Start();
	m_ProgramStatusTimer->Reschedule(0);
	*/

	/*
	 * scheck for reconnect once in a while
	 */
	m_ReconnectTimer = boost::make_shared<Timer>();
	m_ReconnectTimer->SetInterval(ReconnectTimerInterval);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&CompatIdoComponent::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
}


/**
 * Stops the component.
 */
void CompatIdoComponent::Stop(void)
{
	CloseIdoSocket();
}


/**
 * Opens the ido socket, and sends hello to ido2db
 */
void CompatIdoComponent::OpenIdoSocket(void)
{
	OpenSink(GetSocketAddress(), GetSocketPort());
	SendHello(GetInstanceName());

	/* 
	 * if we're connected, do not reconnecte
	 */
	if(m_IdoSocket->IsConnected()) {
		m_IdoSocket->SetReconnect(false);
	} else {
		m_IdoSocket->SetReconnect(true);
	}
}

/*
 * Sends goodbye to ido2db, and closes ido socket
 */
void CompatIdoComponent::CloseIdoSocket(void)
{
	GoodByeSink();
	CloseSink();
}


/* TODO
 * subscribe to all status updates and checkresults and dump them
 * should remove the periodic statusdata dump
 * subscribe to config update events, and send insert/update/delete for configs to ido2db
 * detect if socket disconnected, add timer for reconnect
 */

/**
 * Periodically dumps status information
 */
void CompatIdoComponent::StatusTimerHandler(void)
{
	Logger::Write(LogInformation, "compatido", "Writing compat ido status information");

	DumpStatusData();
        DumpProgramStatusData();
}

/**
 * Periodically dumps config information
 */
void CompatIdoComponent::ConfigTimerHandler(void)
{
	Logger::Write(LogInformation, "compatido", "Writing compat ido config information");

	/* protect the dump of status update messages */
	SetConfigDumpInProgress(true);
	DumpConfigObjects();
	SetConfigDumpInProgress(false);
}

/**
 * Periodically dumps program status information
 */
void CompatIdoComponent::ProgramStatusTimerHandler(void)
{
	/* do not dump any data if config dump is still in progress */
	if(GetConfigDumpInProgress())
		return;

        Logger::Write(LogInformation, "compatido", "Writing compat ido program status information");

        DumpProgramStatusData();
}

/**
 * Periodically check if idosocket requires a reconnect
 */
void CompatIdoComponent::ReconnectTimerHandler(void)
{
        Logger::Write(LogInformation, "compatido", "Checking if ido socket requires reconnect");

	if(m_IdoSocket->GetReconnect()) {

		/* check if we aren't already connected */
		if(m_IdoSocket->IsConnected()) {
        		Logger::Write(LogInformation, "compatido", "Already connected to ido socket ... no reconnect necessary.");
			return;
		}

		/* socket was disconnected, recconnect */
		OpenIdoSocket();

		if(m_IdoSocket->IsConnected()) {
        		Logger::Write(LogInformation, "compatido", "Successfully reconnected to ido socket");
		} else {
			stringstream message;
			message << "Unable to reconnect to ido socket. Trying again in " << GetReconnectInterval() << " sec.";
        		Logger::Write(LogWarning, "compatido", message.str());
		}
	}
}


/**
 * opens a tcp connection to the ido socket
 */
void CompatIdoComponent::OpenSink(String node, String service)
{
	m_IdoSocket = boost::make_shared<IdoSocket>(RoleOutbound);
	m_IdoSocket->Connect(node, service);
	m_IdoSocket->Start();
}

/**
 * sends hello msg to ido2b
 */
void CompatIdoComponent::SendHello(String instancename)
{
	/* connection is always TCP */
	/* connecttype is always initial */
	stringstream message;
	message << "\n\n"
		<< "HELLO" << "\n"
		<< "PROTOCOL" << ": " << 2 << "\n"
		<< "AGENT" << ": " << "I2 COMPATIDO" << "\n"
		<< "AGENTVERSION" << ": " << "2.0" << "\n"
		<< "STARTTIME" << ": " << static_cast<int>(Utility::GetTime()) << "\n"
		<< "DISPOSITION" << ": " << "REALTIME" << "\n"
		<< "CONNECTION" << ": " << "TCPSOCKET" << "\n"
		<< "INSTANCENAME" << ": " << instancename << "\n"
		<< "STARTDATADUMP"
		<< "\n\n";

	m_IdoSocket->SendMessage(message.str());
}

/**
 * sends goodbye msg to ido
 */
void CompatIdoComponent::GoodByeSink(void)
{
        stringstream message;
        message << "\n"
                << 1000 << "\n" 				/* enddatadump */
                << "ENDTIME" << ": " << static_cast<int>(Utility::GetTime()) << "\n"		/* endtime */
		<< "GOODBYE"					/* goodbye */
                << "\n\n";

        m_IdoSocket->SendMessage(message.str());
}

/**
 * closes ido socket
 */
void CompatIdoComponent::CloseSink(void)
{
	m_IdoSocket->Close();
}

/**
 * tell ido2db that we are starting up (must be called before config dump)
 */
void CompatIdoComponent::SendStartProcess(void)
{
/* TODO */
#define PROGRAM_MODIFICATION_DATE "10-17-2012"

        stringstream message;
        message << "\n"
                << 200 << "\n" 						/* processdata */
		<< 1 << "=" << 104 << "\n"				/* type = pprocess prelaunch */
		<< 2 << "=" << "" << "\n"				/* flags */
		<< 3 << "=" << "" << "\n"				/* attributes */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 105 << "=" << "Icinga2" << "\n"			/* progranname */
		<< 107 << "=" << VERSION << "\n"			/*  programversion */
		<< 104 << "=" << PROGRAM_MODIFICATION_DATE << "\n"			/* programdata */
		<< 102 << "=" << Utility::GetPid() << "\n"			/* process id */
		<< 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());

}

/**
 * sends config dump start signal to ido
 */
void CompatIdoComponent::StartConfigDump(void)
{
	/* configtype =1 (original), =2 (retained == default) */
	stringstream message;
	message << "\n\n"
		<< 900 << ":" << "\n"					/* startconfigdump */
		<< 245 << "=" << "RETAINED" << "\n"			/* configdumptype */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 999							/* enddata */
		<< "\n\n";

	m_IdoSocket->SendMessage(message.str());
}

/**
 * sends config dump end signal to ido
 */
void CompatIdoComponent::EndConfigDump(void)
{
        stringstream message;
        message << "\n\n"
                << 901 << ":" << "\n"					/* endconfigdump */
                << 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
                << 999							/* enddata */
                << "\n\n";

        m_IdoSocket->SendMessage(message.str());
}

/**
 * enables host config object in ido
 */
void CompatIdoComponent::EnableHostObject(const Host::Ptr& host)
{
	stringstream message;
	message << "\n"
		<< 500 << ":" << "\n"					/* enableobject */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 53 << "=" << host->GetName() << "\n"			/* host */
                << 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * enables service config object in ido
 */
void CompatIdoComponent::EnableServiceObject(const Service::Ptr& service)
{
        stringstream message;
        message << "\n"
                << 500 << ":" << "\n"                                   /* enableobject */
                << 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"              /* timestamp */
                << 53 << "=" << service->GetHost()->GetName() << "\n"   /* host */
                << 114 << "=" << service->GetAlias() << "\n"            /* service */
                << 999 << "\n\n";                                       /* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * disables host config object in ido
 */
void CompatIdoComponent::DisableHostObject(const Host::Ptr& host)
{
        stringstream message;
        message << "\n"
                << 501 << ":" << "\n"                                   /* disableobject */
                << 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"              /* timestamp */
                << 53 << "=" << host->GetName() << "\n"                 /* host */
                << 999 << "\n\n";                                       /* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * disables service config object in ido
 */
void CompatIdoComponent::DisableServiceObject(const Service::Ptr& service)
{
        stringstream message;
        message << "\n"
                << 501 << ":" << "\n"                                   /* disableobject */
                << 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"              /* timestamp */
                << 53 << "=" << service->GetHost()->GetName() << "\n"   /* host */
                << 114 << "=" << service->GetAlias() << "\n"            /* service */
                << 999 << "\n\n";                                       /* enddata */
 
        m_IdoSocket->SendMessage(message.str());
}




/**
 * dump host config to ido
 *
 * @param host Pointer to the Host object
 */
void CompatIdoComponent::DumpHostObject(const Host::Ptr& host)
{
	//FIXME DEBUG only
	stringstream log;
	log << "Dumping Host Config: " << host->GetName();
        Logger::Write(LogInformation, "compatido", log.str());

	stringstream message;
	message << "\n"
		<< 400 << ":" << "\n"					/* hostdefinition */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 174 << "=" << host->GetName() << "\n"		/* hostname */
		<< 258 << "=" << host->GetAlias() << "\n"		/* displayname */
		<< 159 << "=" << host->GetAlias() << "\n"		/* hostalias */
		<< 158 << "=" << host->GetName() << "\n"		/* hostaddress */
		<< 266 << "=" << host->GetName() << "\n"		/* hostaddress6 */
		<< 160 << "=" << "" << "\n"				/* hostcheckcommand */
		<< 163 << "=" << "" << "\n"				/* hosteventhandler */
		<< 177 << "=" << "" << "\n"				/* hostnotificationperiod */
		<< 162 << "=" << "" << "\n"				/* hostcheckperiod */
		<< 166 << "=" << "" << "\n"				/* hostfailurepredictionoptions */
		<< 161 << "=" << 1 << "\n"				/* hostcheckinterval */
		<< 247 << "=" << 1 << "\n"				/* hostretryinterval */
		<< 173 << "=" << 1 << "\n"				/* hostmaxcheckattempts */
		<< 246 << "=" << 0 << "\n"				/* firstnotificationdelay */
		<< 176 << "=" << 0 << "\n"				/* hostnotificationinterval */
		<< 189 << "=" << 0 << "\n"				/* notifyhostdown */
		<< 192 << "=" << 0 << "\n"				/* notifyhostunreachable */
		<< 191 << "=" << 0 << "\n"				/* notifyhostrecovery */
		<< 190 << "=" << 0 << "\n"				/* notifyhostflapping */
		<< 248 << "=" << 0 << "\n"				/* notifyhostdowntime */
		<< 167 << "=" << 0 << "\n"				/* hostflapdetectionenabled */
		<< 251 << "=" << 0 << "\n"				/* flaptdetectiononup */
		<< 252 << "=" << 0 << "\n"				/* flapdetectionoidown */
		<< 253 << "=" << 0 << "\n"				/* flapdetectiononunreachable */
		<< 183 << "=" << 0 << "\n"				/* lowhostflatthreshold */
		<< 156 << "=" << 0 << "\n"				/* highhostflapthreshold */
		<< 230 << "=" << 0 << "\n"				/* stalkhostonup */
		<< 228 << "=" << 0 << "\n"				/* stalkhostoidown */
		<< 229 << "=" << 0 << "\n"				/* stalkhostonunreachable */
		<< 168 << "=" << 0 << "\n"				/* hostfreshnesschecksenable */
		<< 169 << "=" << 0 << "\n"				/* hostfreshnessthreshold */
		<< 201 << "=" << 1 << "\n"				/* processhostperformancedata */
		<< 8 << "=" << 1 << "\n"				/* activehostchecksenabled */
		<< 96 << "=" << 1 << "\n"				/* passivehostchecksenabled */
		<< 164 << "=" << 0 << "\n"				/* hosteventhanderenabled */
		<< 204 << "=" << 1 << "\n"				/* retainhoststatusinformation */
		<< 203 << "=" << 1 << "\n"				/* retainnonhoststatusinformation */
		<< 178 << "=" << 1 << "\n"				/* hostnotificationsenabled */
		<< 165 << "=" << 0 << "\n"				/* hostfailurepredictionenabled */
		<< 91 << "=" << 0 << "\n"				/* obsessoverhost */
		<< 186 << "=" << "i2_notes" << "\n"			/* notes */
		<< 187 << "=" << "" << "\n"				/* notesurl */
		<< 126 << "=" << "" << "\n"				/* actionurl */
		<< 179 << "=" << "" << "\n"				/* iconimage */
		<< 180 << "=" << "" << "\n"				/* iconimagealt */
		<< 239 << "=" << "" << "\n"				/* vrmlimage */
		<< 235 << "=" << "" << "\n"				/* statusmapimage */
		<< 154 << "=" << 0 << "\n"				/* have2dcoords */
		<< 240 << "=" << 0.0 << "\n"				/* x2d */
		<< 242 << "=" << 0.0 << "\n"				/* y2d */
		<< 155 << "=" << 0 << "\n"				/* have3dcoords */
		<< 241 << "=" << 0.0 << "\n"				/* x3d */
		<< 243 << "=" << 0.0 << "\n"				/* y3d */
		<< 244<< "=" << 0.0 << "\n"				/* z3d */
		/* FIXME add more related config items
	 	* parents, contactgroups, contacts, custom vars
	 	* before sending the message
	 	*/
		<< 200 << "=" << "i2_parent" << "\n"			/* parenthost */
		<< 130 << "=" << "i2_contactgroup" << "\n"		/* contactgroup */
		<< 264 << "=" << "i2_contact" << "\n"			/* contact */
                << 262 << "=" << "i2_customvar" << ":" << 1 << ":" << "i2_custom_var_mod" << "\n"	/* customvariable */
                << 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * dump host status to ido
 *
 * @param host Pointer to Host object
 */
void CompatIdoComponent::DumpHostStatus(const Host::Ptr& host)
{

        int state;
        if (!host->IsReachable())
                state = 2; /* unreachable */
        else if (!host->IsUp())
                state = 1; /* down */
        else   
                state = 0; /* up */

        stringstream message;
        message << "\n"
		<< 212 << ":" << "\n"					/* hoststatusdata */
		<< 1 << "=" << "" << "\n"				/* type */
		<< 2 << "=" << "" << "\n"				/* flags */
		<< 3 << "=" << "" << "\n"				/* attributes */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 53 << "=" << host->GetName() << "\n"			/* host */
		<< 95 << "=" << "i2 static" << "\n"				/* output */
		<< 125 << "=" << "" << "\n"				/* longout */
		<< 99 << "=" << "" << "\n"				/* perfdata */
		<< 27 << "=" << 0 << "\n"				/* currentstate */
		<< 51 << "=" << 1 << "\n"				/* hasbeenchecked */
		<< 115 << "=" << 1 << "\n"				/* shouldbescheduled */
		<< 25 << "=" << 1 << "\n"				/* currentcheckattempt */
		<< 76 << "=" << 1 << "\n"				/* maxcheckattempts */
		<< 58 << "=" << static_cast<int>(Utility::GetTime()) << "\n"		/* lasthostcheck (seconds only) */
		<< 81 << "=" << static_cast<int>(Utility::GetTime()) << "\n"		/* nexthostcheck (seconds only) */
		<< 12 << "=" << 0 << "\n"				/* checktype */
		<< 63 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* laststatechange */
		<< 57 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasthardstatechange */
		<< 56 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasthardstate */
		<< 69 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasttimeup */
		<< 65 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasttimedown */
		<< 68 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lastttimeunreachable */
		<< 121 << "=" << 0 << "\n"				/* statetype */
		<< 59 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasthostnotification */
		<< 82 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* nexthostnotification */
		<< 85 << "=" << 0 << "\n"				/* nomorenotifications */
		<< 88 << "=" << 0 << "\n"				/* notificationsenabled */
		<< 101 << "=" << 0 << "\n"				/* problemhasbeenacknowledged */
		<< 7 << "=" << 0 << "\n"				/* acknowledgementtype */
		<< 26 << "=" << 0 << "\n"				/* currentnotificationnumber */
		<< 96 << "=" << 1 << "\n"				/* passivehostchecksenabled */
		<< 38 << "=" << 0 << "\n"				/* eventhandlerenabled */
		<< 8 << "=" << 0 << "\n"				/* activehostchecksenabled */
		<< 47 << "=" << 0 << "\n"				/* flapdetectionenabled */
		<< 54 << "=" << 0 << "\n"				/* isflapping */
		<< 98 << "=" << 0.0 << "\n"				/* percentstatechange */
		<< 71 << "=" << 0.0 << "\n"				/* latency */
		<< 42 << "=" << 0.0 << "\n"				/* executiontime */
		<< 113 << "=" << 0 << "\n"				/* scheduleddowntimedepth */
		<< 45 << "=" << 0 << "\n"				/* failurepredictionsenabled */
		<< 103 << "=" << 1 << "\n"				/* processperformancedata */
		<< 91 << "=" << 0 << "\n"				/* obsessoverhost */
		<< 78 << "=" << 0 << "\n"				/* modifiedattributes */
		<< 37 << "=" << "" << "\n"				/* eventhandler */
		<< 11 << "=" << "i2_virtual_check" << "\n"		/* checkcommand */
		<< 86 << "=" << "" << "\n"				/* normalcheckinterval */
		<< 109 << "=" << 0 << "\n"				/* retrycheckinterval */
		<< 162 << "=" << "" << "\n"				/* hostcheckperiod */
		/* FIXME dump all customvars in a loop */
		<< 262 << "=" << "i2_customvar" << ":" << "1" << ":" << "i2_customvarmod" << "\n"	/* customvariable */
		<< 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * dump service config to ido
 *
 * @param service Pointer to Service object
 */
void CompatIdoComponent::DumpServiceObject(const Service::Ptr& service)
{
	//FIXME DEBUG only
	stringstream log;
	log << "Dumping Service Config: " << service->GetHost()->GetName() << "->" << service->GetAlias();
        Logger::Write(LogInformation, "compatido", log.str());

	stringstream message;
	message << "\n"
		<< 402 << ":" << "\n"					/* servicedefinition */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 174 << "=" << service->GetHost()->GetName() << "\n"	/* hostname */
		<< 258  << "=" << service->GetAlias() << "\n"		/* displayname */
		<< 210 << "=" << service->GetAlias() << "\n"		/* servicedescription */
		<< 207 << "=" << "check_i2" << "\n"			/* servicecheckcommand */
		<< 211 << "=" << "" << "\n"				/* serviceeventhandler */
		<< 224 << "=" << "" << "\n"				/* servicenotificationperiod */
		<< 209 << "=" << "" << "\n"				/* servicecheckperiod */
		<< 214 << "=" << "" << "\n"				/* servicefailurepredictionoptions */
		<< 208 << "=" << 1 << "\n"				/* servicecheckinterval */
		<< 226 << "=" << 1 << "\n"				/* serviceretryinterval */
		<< 185 << "=" << 1 << "\n"				/* maxservicecheckattempts */
		<< 246 << "=" << 0 << "\n"				/* firstnotificationdelay */
		<< 223 << "=" << 0 << "\n"				/* servicenotificationinterval */
		<< 196 << "=" << 0 << "\n"				/* notifyserviceunknown */
		<< 197 << "=" << 0 << "\n"				/* notifyservicewarning */
		<< 193 << "=" << 0 << "\n"				/* notifyservicecritical */
		<< 195 << "=" << 0 << "\n"				/* notifyservicerecovery */
		<< 194 << "=" << 0 << "\n"				/* notifyserviceflapping */
		<< 249 << "=" << 0 << "\n"				/* notifyservicedowntime */
		<< 232 << "=" << 0 << "\n"				/* stalkserviceonok */
		<< 234 << "=" << 0 << "\n"				/* stalkserviceonwarning */
		<< 233 << "=" << 0 << "\n"				/* stalkserviceonunknown */
		<< 231 << "=" << 0 << "\n"				/* stalkserviceoncritical */
		<< 221 << "=" << 0 << "\n"				/* serviceisvolatile */
		<< 215 << "=" << 0 << "\n"				/* serviceflapdetectionenabled */
		<< 254 << "=" << 0 << "\n"				/* flapdetectiononok */
		<< 255 << "=" << 0 << "\n"				/* flapdetectiononwarning */
		<< 256 << "=" << 0 << "\n"				/* flapdetectiononunknown */
		<< 257 << "=" << 0 << "\n"				/* flapdetectiononcritical */
		<< 184 << "=" << 0 << "\n"				/* lowserviceflapthreshold */
		<< 157 << "=" << 0 << "\n"				/* highserviceflapthreshold */
		<< 202 << "=" << 1 << "\n"				/* processserviceperformancedata */
		<< 216 << "=" << 0 << "\n"				/* servicefreshnesschecksenabled */
		<< 217 << "=" << 0 << "\n"				/* servicefreshnessthreshold */
		<< 97 << "=" << 1 << "\n"				/* passiveservicechecksenabled */
		<< 212 << "=" << 0 << "\n"				/* serviceeventhandlerenabled */
		<< 9 << "=" << 1 << "\n"				/* activeservicechecksenabled */
		<< 206 << "=" << 1 << "\n"				/* retainservicestatusinformation */
		<< 205 << "=" << 1 << "\n"				/* retainservicenonstatusinformation */
		<< 225 << "=" << 0 << "\n"				/* servicenotificationsenabled */
		<< 93 << "=" << 0 << "\n"				/* obsessoverservice */
		<< 213 << "=" << 0 << "\n"				/* servicefailurepredictionenabled */
		<< 186 << "=" << "i2_notes" << "\n"			/* notes */
		<< 187 << "=" << "" << "\n"				/* notesurl */
		<< 126 << "=" << "" << "\n"				/* actionurl */
		<< 179 << "=" << "" << "\n"				/* iconimage */
		<< 180 << "=" << "" << "\n"				/* iconimagealt */
		/* FIXME add more related config items
	 	* contactgroups, contacts, custom vars
	 	* before sending the message, in a loop
	 	*/
		<< 130 << "=" << "i2_contactgroup" << "\n"		/* contactgroup */
		<< 264 << "=" << "i2_contact" << "\n"			/* contact */
		<< 262 << "=" << "i2_customvar" << ":" << 1 << ":" << "i2_custom_var_mod" << "\n"	/* customvariable */
		<< 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * dump service status to ido
 *
 * @param service Pointer to Service object
 */ 
void CompatIdoComponent::DumpServiceStatus(const Service::Ptr& service)
{
        String output;
        String perfdata;
        double schedule_start = -1, schedule_end = -1;
        double execution_start = -1, execution_end = -1;

        Dictionary::Ptr cr = service->GetLastCheckResult();

        if (cr) {
                output = cr->Get("output");
                schedule_start = cr->Get("schedule_start");
                schedule_end = cr->Get("schedule_end");
                execution_start = cr->Get("execution_start");
                execution_end = cr->Get("execution_end");
                perfdata = cr->Get("performance_data_raw");
        }

        double execution_time = (execution_end - execution_start);
        double latency = (schedule_end - schedule_start) - execution_time;

        int state = service->GetState();

        if (state > StateUnknown)
                state = StateUnknown;

        stringstream message;
        message << "\n"
                << 213 << ":" << "\n"					/* servicestatusdata */
		<< 1 << "=" << "" << "\n"				/* type */
		<< 2 << "=" << "" << "\n"				/* flags */
		<< 3 << "=" << "" << "\n"				/* attributes */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 53 << "=" << service->GetHost()->GetName() << "\n"	/* host */
		<< 114 << "=" << service->GetAlias() << "\n"		/* service */
		<< 95 << "=" << output << "\n"				/* output */
		<< 125 << "=" << "" << "\n"				/* longoutput */
		<< 99 << "=" << perfdata << "\n"			/* perfdata */
		<< 27 << "=" << state << "\n"				/* currentstate */
		<< 51 << "=" << (service->GetLastCheckResult() ? 1 : 0) << "\n"	/* hasbeenchecked */
		<< 115 << "=" << 1 << "\n"				/* shouldbescheduled */
		<< 25 << "=" << service->GetCurrentCheckAttempt() << "\n"	/* currentcheckattempt */
		<< 76 << "=" << service->GetMaxCheckAttempts() << "\n"	/* maxcheckattempts */
		<< 61 << "=" << static_cast<int>(schedule_end) << "\n"	/* lastservicecheck (seconds only) */
		<< 83 << "=" << static_cast<int>(service->GetNextCheck()) << "\n"	/* nextservicecheck (seconds only) */
		<< 12 << "=" << 0 << "\n"				/* checktype */
		<< 63 << "=" << static_cast<int>(service->GetLastStateChange()) << "\n"	/* laststatechange (seconds only) */
		<< 57 << "=" << static_cast<int>(service->GetLastHardStateChange()) << "\n"	/* lasthardstatechange (seconds only) */
		<< 56 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasthardstate */
		<< 66 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasttimeok */
		<< 70 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasttimewarning */
		<< 67 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasttimeunknown */
		<< 64 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lasttimecritical */
		<< 121 << "=" << service->GetStateType() << "\n"	/* statetype */
		<< 62 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lastservicenotification */
		<< 84 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* nextservicenotification */
		<< 85 << "=" << 0 << "\n"				/* nomorenotifications */
		<< 88 << "=" << 0 << "\n"				/* notificationsenabled */
		<< 101 << "=" << 0 << "\n"				/* problemhasbeenacknowledged */
		<< 7 << "=" << 0 << "\n"				/* acknowledgementtype */
		<< 26 << "=" << 0 << "\n"				/* currentnotifcationnumber */
		<< 97 << "=" << 1 << "\n"				/* passiveservicechecksenabled */
		<< 38 << "=" << 0 << "\n"				/* eventhandlerenabled */
		<< 9 << "=" << 1 << "\n"				/* activeservicechecksenabled */
		<< 47 << "=" << 0 << "\n"				/* flapdetectionenabled */
		<< 54 << "=" << 0 << "\n"				/* isflapping */
		<< 98 << "=" << 0.0 << "\n"				/* percentstatechange */
		<< 71 << "=" << latency << "\n"				/* latency */
		<< 42 << "=" << execution_time << "\n"			/* executiontime */
		<< 113 << "=" << 0 << "\n"				/* scheduleddowntimedepth */
		<< 45 << "=" << 0 << "\n"				/* failurepredictionsenabled */
		<< 103 << "=" << 1 << "\n"				/* processperformancedata */
		<< 93 << "=" << 0 << "\n"				/* obsessoverservice */
		<< 80 << "=" << 0 << "\n"				/* modifiedserviceattributes */
		<< 37 << "=" << "" << "\n"				/* eventhandler */
		<< 11 << "=" << "i2_check_service" << "\n"		/* checkcommand */
		<< 86 << "=" << service->GetCheckInterval() / 60.0 << "\n"	/* normalcheckinterval */
		<< 109 << "=" << service->GetRetryInterval() / 60.0 << "\n"	/* retrycheckinterval */
		<< 209 << "=" << "" << "\n"				/* servicecheckperiod */
		/* FIXME dump customvars in a loop */
		<< 262 << "=" << "i2_customvar" << ":" << "1" << ":" << "i2_customvarmod" << "\n"	/* customvariable */
		<< 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());
}


/** 
 * dumps programstatus to ido
 */
void CompatIdoComponent::DumpProgramStatusData(void)
{
	double start_time = IcingaApplication::GetInstance()->GetStartTime();

        stringstream message;
        message << "\n"
                << 211 << ":" << "\n"					/* programstatusdata */
		<< 1 << "=" << "" << "\n"				/* type */
		<< 2 << "=" << "" << "\n"				/* flags */
		<< 3 << "=" << "" << "\n"				/* attributes */
		<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"		/* timestamp */
		<< 106 << "=" << static_cast<int>(start_time) << "\n"	/* programstarttime */
		<< 102 << "=" << Utility::GetPid() << "\n"		/* processid */
		<< 28 << "=" << "1" << "\n"				/* daemonmode */
		<< 55 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lastcommandcheck */
		<< 60 << "=" << static_cast<int>(Utility::GetTime()) << "\n"				/* lastlogrotation */
		<< 88 << "=" << 0 << "\n"				/* notificationsenabled */
		<< 9 << "=" << 1 << "\n"				/* activeservicechecksenabled */
		<< 97 << "=" << 1 << "\n"				/* passiveservicechecksenabled */
		<< 8 << "=" << 0 << "\n"				/* activehostchecksenabled */
		<< 96 << "=" << 0 << "\n"				/* passivehostchecksenabled */
		<< 39 << "=" << 0 << "\n"				/* eventhandlersenabled */
		<< 47 << "=" << 1 << "\n"				/* flaptdetectionenabled */
		<< 45 << "=" << 0 << "\n"				/* failurepredictionenabled */
		<< 103 << "=" << 1 << "\n"				/* processperformancedata */
		<< 92 << "=" << 0 << "\n"				/* obsessoverhosts */
		<< 94 << "=" << 0 << "\n"				/* obsessoverservices */
		<< 78 << "=" << 0 << "\n"				/* modifiedhostattributes */
		<< 80 << "=" << 0 << "\n"				/* modifiedserviceattributes */
		<< 49 << "=" << "" << "\n"				/* globalhosteventhandler */
		<< 50 << "=" << "" << "\n"				/* globalserviceeventhandler */
		<< 270 << "=" << static_cast<int>(Utility::GetTime()) << "\n" 				/* disablednotificationsexpiretime - supported in 1.8 XXX */
                << 999 << "\n\n";					/* enddata */

        m_IdoSocket->SendMessage(message.str());
}

/**
 * process and dump all config objects
 */
void CompatIdoComponent::DumpConfigObjects(void)
{
	/* TODO dump
	 * commands, timeperiods, contact*
	 * escalations, dependencies
	 * if needed/available.
	 */

	/* tell ido2db that we start now */
	StartConfigDump();

	/* hosts and hostgroups */
	map<String, vector<String> > hostgroups;

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicObject::GetObjects("Host")) {
		const Host::Ptr& host = static_pointer_cast<Host>(object);

		Dictionary::Ptr dict;
		dict = host->GetGroups();

		if (dict) {
			Value hostgroup;
			BOOST_FOREACH(tie(tuples::ignore, hostgroup), dict) {
				hostgroups[hostgroup].push_back(host->GetName());
			}
		}

		DumpHostObject(host);
		//FIXME remove me, debug only XXX
		//DisableHostObject(host);
	}

	pair<String, vector<String > > hgt;
	BOOST_FOREACH(hgt, hostgroups) {
		const String& name = hgt.first;
		const vector<String>& hosts = hgt.second;

		if(HostGroup::Exists(name)) {
			HostGroup::Ptr hg = HostGroup::GetByName(name);

			/* dump the hostgroup and its attributes/members to ido */
			stringstream message;
			message << "\n"
				<< 401 << ":" << "\n"				/* hostgroupdefinition */
				<< 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"	/* timestamp */
				<< 172 << "=" << name << "\n"			/* hostgroupname */
				<< 170 << "=" << hg->GetAlias() << "\n";	/* hostgroupalias */

			CreateMessageList(message, hosts, 171);			/* hostgroupmember */
				
			message << 999 << "\n\n";				/* enddata */

        		m_IdoSocket->SendMessage(message.str());
		}
	}

	/* services and servicegroups */
	map<String, vector<Service::Ptr> > servicegroups;

	BOOST_FOREACH(tie(tuples::ignore, object), DynamicObject::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		Dictionary::Ptr dict;

		dict = service->GetGroups();

		if (dict) {
			Value servicegroup;
			BOOST_FOREACH(tie(tuples::ignore, servicegroup), dict) {
				servicegroups[servicegroup].push_back(service);
			}
		}

		DumpServiceObject(service);
		//FIXME remove me, debug only XXX
		//DisableServiceObject(service);
	}

	pair<String, vector<Service::Ptr> > sgt;
	BOOST_FOREACH(sgt, servicegroups) {
		const String& name = sgt.first;
		const vector<Service::Ptr>& services = sgt.second;

		if (ServiceGroup::Exists(name)) {
			ServiceGroup::Ptr sg = ServiceGroup::GetByName(name);

			/* dump the servicegroup and its attributes/members to ido */
                        stringstream message;
                        message << "\n"
                                << 403 << ":" << "\n"				/* servicegroupdefinition */
                                << 4 << "=" << std::setprecision(17) << Utility::GetTime() << "\n"	/* timestamp */
                                << 220 << "=" << name << "\n"			/* servicegroupname */
                                << 218 << "=" << sg->GetAlias() << "\n";	/* servicegroupalias */

			vector<String> sglist;
			vector<Service::Ptr>::iterator vt;

			BOOST_FOREACH(const Service::Ptr& service, services) {
				sglist.push_back(service->GetHost()->GetName());
				sglist.push_back(service->GetAlias());
			}
	
			CreateMessageList(message, services, 219);		/* servicegroupmember */

                        message << 999 << "\n\n";				/* enddata */

        		m_IdoSocket->SendMessage(message.str());
		}
	}

	/* tell ido2db that we ended dumping the config */
	EndConfigDump();
}

/**
 * process and dump all status data 
 */
void CompatIdoComponent::DumpStatusData(void)
{
        /* hosts */
        DynamicObject::Ptr object;
        BOOST_FOREACH(tie(tuples::ignore, object), DynamicObject::GetObjects("Host")) {
                const Host::Ptr& host = static_pointer_cast<Host>(object);

                DumpHostStatus(host);
        }


        /* services */
        BOOST_FOREACH(tie(tuples::ignore, object), DynamicObject::GetObjects("Service")) {
                Service::Ptr service = static_pointer_cast<Service>(object);

                DumpServiceStatus(service);
        }
}


EXPORT_COMPONENT(compatido, CompatIdoComponent);
