/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CLASSCOMPILER_H
#define CLASSCOMPILER_H

#include <string>
#include <istream>
#include <utility>
#include <vector>
#include <algorithm>
#include <map>

namespace icinga
{

struct ClassDebugInfo
{
	std::string path;
	int first_line;
	int first_column;
	int last_line;
	int last_column;
};

enum FieldAccessorType
{
	FTGet,
	FTSet,
	FTDefault,
	FTTrack,
	FTNavigate
};

struct FieldAccessor
{
	FieldAccessorType Type;
	std::string Accessor;
	bool Pure;

	FieldAccessor(FieldAccessorType type, std::string accessor, bool pure)
		: Type(type), Accessor(std::move(accessor)), Pure(pure)
	{ }
};

/* keep this in sync with lib/base/type.hpp */
enum FieldAttribute
{
	FAEphemeral = 1,
	FAConfig = 2,
	FAState = 4,
	FAEnum = 8,
	FAGetProtected = 16,
	FASetProtected = 32,
	FANoStorage = 64,
	FALoadDependency = 128,
	FARequired = 256,
	FANavigation = 512,
	FANoUserModify = 1024,
	FANoUserView = 2048,
	FADeprecated = 4096,
	FAGetVirtual = 8192,
	FASetVirtual = 16384,
	FAActivationPriority = 32768
};

struct FieldType
{
	bool IsName{false};
	std::string TypeName;
	int ArrayRank{0};

	inline std::string GetRealType() const
	{
		if (ArrayRank > 0)
			return "Array::Ptr";

		if (IsName)
			return "String";

		return TypeName;
	}

	inline std::string GetArgumentType() const
	{
		std::string realType = GetRealType();

		if (realType == "bool" || realType == "double" || realType == "int")
			return realType;
		else
			return "const " + realType + "&";
	}
};

struct Field
{
	int Attributes{0};
	FieldType Type;
	std::string Name;
	std::string AlternativeName;
	std::string GetAccessor;
	bool PureGetAccessor{false};
	std::string SetAccessor;
	bool PureSetAccessor{false};
	std::string DefaultAccessor;
	std::string TrackAccessor;
	std::string NavigationName;
	std::string NavigateAccessor;
	bool PureNavigateAccessor{false};
	int Priority{0};

	inline std::string GetFriendlyName() const
	{
		if (!AlternativeName.empty())
			return AlternativeName;

		bool cap = true;
		std::string name = Name;

		for (char& ch : name) {
			if (ch == '_') {
				cap = true;
				continue;
			}

			if (cap) {
				ch = toupper(ch);
				cap = false;
			}
		}

		name.erase(
			std::remove(name.begin(), name.end(), '_'),
			name.end()
			);

		/* TODO: figure out name */
		return name;
	}
};

enum TypeAttribute
{
	TAAbstract = 1,
	TAVarArgConstructor = 2
};

struct Klass
{
	std::string Name;
	std::string Parent;
	std::string TypeBase;
	int Attributes;
	std::vector<Field> Fields;
	std::vector<std::string> LoadDependencies;
	int ActivationPriority{0};
};

enum RuleAttribute
{
	RARequired = 1
};

struct Rule
{
	int Attributes;
	bool IsName;
	std::string Type;
	std::string Pattern;

	std::vector<Rule> Rules;
};

enum ValidatorType
{
	ValidatorField,
	ValidatorArray,
	ValidatorDictionary
};

struct Validator
{
	std::string Name;
	std::vector<Rule> Rules;
};

class ClassCompiler
{
public:
	ClassCompiler(std::string path, std::istream& input, std::ostream& oimpl, std::ostream& oheader);
	~ClassCompiler();

	void Compile();

	std::string GetPath() const;

	void InitializeScanner();
	void DestroyScanner();

	void *GetScanner();

	size_t ReadInput(char *buffer, size_t max_size);

	void HandleInclude(const std::string& path, const ClassDebugInfo& locp);
	void HandleAngleInclude(const std::string& path, const ClassDebugInfo& locp);
	void HandleImplInclude(const std::string& path, const ClassDebugInfo& locp);
	void HandleAngleImplInclude(const std::string& path, const ClassDebugInfo& locp);
	void HandleClass(const Klass& klass, const ClassDebugInfo& locp);
	void HandleValidator(const Validator& validator, const ClassDebugInfo& locp);
	void HandleNamespaceBegin(const std::string& name, const ClassDebugInfo& locp);
	void HandleNamespaceEnd(const ClassDebugInfo& locp);
	void HandleCode(const std::string& code, const ClassDebugInfo& locp);
	void HandleLibrary(const std::string& library, const ClassDebugInfo& locp);
	void HandleMissingValidators();

	void CodeGenValidator(const std::string& name, const std::string& klass, const std::vector<Rule>& rules, const std::string& field, const FieldType& fieldType, ValidatorType validatorType);
	void CodeGenValidatorSubrules(const std::string& name, const std::string& klass, const std::vector<Rule>& rules);

	static void CompileFile(const std::string& inputpath, const std::string& implpath,
		const std::string& headerpath);
	static void CompileStream(const std::string& path, std::istream& input,
		std::ostream& oimpl, std::ostream& oheader);

	static void OptimizeStructLayout(std::vector<Field>& fields);

private:
	std::string m_Path;
	std::istream& m_Input;
	std::ostream& m_Impl;
	std::ostream& m_Header;
	void *m_Scanner;

	std::string m_Library;

	std::map<std::pair<std::string, std::string>, Field> m_MissingValidators;

	static unsigned long SDBM(const std::string& str, size_t len);
	static std::string BaseName(const std::string& path);
	static std::string FileNameToGuardName(const std::string& path);
};

}

#endif /* CLASSCOMPILER_H */

