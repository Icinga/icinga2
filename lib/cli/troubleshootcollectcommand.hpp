/******************************************************************************
* Icinga 2                                                                   *
* Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef TROUBLESHOOTCOLLECTCOMMAND_H
#define TROUBLESHOOTCOLLECTCOMMAND_H

#include "cli/clicommand.hpp"
#include "base/i2-base.hpp"
#include "base/dictionary.hpp"

namespace icinga
{
	/**
	* The "troubleshoot collect" command.
	*
	* @ingroup cli
	*/
	class TroubleshootCollectCommand : public CLICommand
	{
	public:
		DECLARE_PTR_TYPEDEFS(TroubleshootCollectCommand);

		virtual String GetDescription(void) const;
		virtual String GetShortDescription(void) const;
		virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const;
		virtual void InitParameters(boost::program_options::options_description& visibleDesc,
									boost::program_options::options_description& hiddenDesc) const;
	
	private:
		class InfoLog;
		class InfoLogLine;
		static bool GeneralInfo(InfoLog& log, boost::program_options::variables_map vm);
		static bool FeatureInfo(InfoLog& log, boost::program_options::variables_map vm);
		static bool ObjectInfo(InfoLog& log, boost::program_options::variables_map vm, Dictionary::Ptr& logs);
		static bool ReportInfo(InfoLog& log, boost::program_options::variables_map vm, Dictionary::Ptr& logs);
		static bool ConfigInfo(InfoLog& log, boost::program_options::variables_map vm);

		static int tail(const String& file, int numLines, InfoLog& log);
		static bool CheckFeatures(InfoLog& log);
		static void GetLatestReport(const String& filename, time_t& bestTimestamp, String& bestFilename);
		static bool PrintCrashReports(InfoLog& log);
		static bool PrintConf(InfoLog& log, const String& path);
		static bool CheckConfig(void);
		static void CheckObjectFile(const String& objectfile, InfoLog& log, const bool print,
									Dictionary::Ptr& logs, std::set<String>& configs);
		static void PrintLoggers(InfoLog& log, Dictionary::Ptr& logs);
		static void PrintConfig(InfoLog& log, const std::set<String>& configSet, const String::SizeType& countTotal);
	};
}
#endif /* TROUBLESHOOTCOLLECTCOMMAND_H */
