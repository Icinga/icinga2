/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CLICOMMAND_H
#define CLICOMMAND_H

#include "cli/i2-cli.hpp"
#include "base/value.hpp"
#include "base/utility.hpp"
#include "base/type.hpp"
#include <vector>
#include <boost/program_options.hpp>

namespace icinga
{

std::vector<String> GetBashCompletionSuggestions(const String& type, const String& word);
std::vector<String> GetFieldCompletionSuggestions(const Type::Ptr& type, const String& word);

enum ImpersonationLevel
{
	ImpersonateNone,
	ImpersonateRoot,
	ImpersonateIcinga
};

/**
 * A CLI command.
 *
 * @ingroup base
 */
class CLICommand : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(CLICommand);

	typedef std::vector<String>(*ArgumentCompletionCallback)(const String&, const String&);

	virtual String GetDescription() const = 0;
	virtual String GetShortDescription() const = 0;
	virtual int GetMinArguments() const;
	virtual int GetMaxArguments() const;
	virtual bool IsHidden() const;
	virtual bool IsDeprecated() const;
	virtual void InitParameters(boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc) const;
	virtual ImpersonationLevel GetImpersonationLevel() const;
	virtual int Run(const boost::program_options::variables_map& vm, const std::vector<std::string>& ap) const = 0;
	virtual std::vector<String> GetArgumentSuggestions(const String& argument, const String& word) const;
	virtual std::vector<String> GetPositionalSuggestions(const String& word) const;

	static CLICommand::Ptr GetByName(const std::vector<String>& name);
	static void Register(const std::vector<String>& name, const CLICommand::Ptr& command);
	static void Unregister(const std::vector<String>& name);

	static bool ParseCommand(int argc, char **argv, boost::program_options::options_description& visibleDesc,
		boost::program_options::options_description& hiddenDesc,
		boost::program_options::positional_options_description& positionalDesc,
		boost::program_options::variables_map& vm, String& cmdname, CLICommand::Ptr& command, bool autocomplete);

	static void ShowCommands(int argc, char **argv,
		boost::program_options::options_description *visibleDesc = nullptr,
		boost::program_options::options_description *hiddenDesc = nullptr,
		ArgumentCompletionCallback globalArgCompletionCallback = nullptr,
		bool autocomplete = false, int autoindex = -1);

private:
	static std::mutex& GetRegistryMutex();
	static std::map<std::vector<String>, CLICommand::Ptr>& GetRegistry();
};

#define REGISTER_CLICOMMAND(name, klass) \
	INITIALIZE_ONCE([]() { \
		std::vector<String> vname = String(name).Split("/"); \
		CLICommand::Register(vname, new klass()); \
	})

}

#endif /* CLICOMMAND_H */
