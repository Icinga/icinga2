/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef TROUBLESHOOTCOMMAND_H
#define TROUBLESHOOTCOMMAND_H

#include "cli/clicommand.hpp"
#include "base/i2-base.hpp"
#include "base/dictionary.hpp"

namespace icinga
{

/**
 * The "troubleshoot" command.
 *
 * @ingroup cli
 */
class TroubleshootCommand final : public CLICommand
{
public:
	DECLARE_PTR_TYPEDEFS(TroubleshootCommand);

	virtual String GetDescription() const override;
	virtual String GetShortDescription() const override;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const override;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const override;

private:
	class InfoLog;
	class InfoLogLine;
	static bool GeneralInfo(InfoLog& log, const boost::program_options::variables_map& vm);
	static bool FeatureInfo(InfoLog& log, const boost::program_options::variables_map& vm);
	static bool ObjectInfo(InfoLog& log, const boost::program_options::variables_map& vm,
		Dictionary::Ptr& logs, const String& path);
	static bool ReportInfo(InfoLog& log, const boost::program_options::variables_map& vm,
		Dictionary::Ptr& logs);
	static bool ConfigInfo(InfoLog& log, const boost::program_options::variables_map& vm);

	static int Tail(const String& file, const int numLines, InfoLog& log);
	static bool CheckFeatures(InfoLog& log);
	static void GetLatestReport(const String& filename, time_t& bestTimestamp, String& bestFilename);
	static bool PrintCrashReports(InfoLog& log);
	static bool PrintFile(InfoLog& log, const String& path);
	static bool CheckConfig();
	static void CheckObjectFile(const String& objectfile, InfoLog& log, InfoLog *OFile, const bool objectConsole,
		Dictionary::Ptr& logs, std::set<String>& configs);
	static bool PrintVarsFile(const String& path, const bool console);
	static void PrintLoggers(InfoLog& log, Dictionary::Ptr& logs);
	static void PrintObjectOrigin(InfoLog& log, const std::set<String>& configSet);
};

}
#endif /* TROUBLESHOOTCOMMAND_H */
