#include <cstdlib>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <string>

using namespace std;

void trim(string& str, const char *whitespace = "\r\n\t ")
{
	string::size_type pos;

	pos = str.find_first_not_of(whitespace);
	if (pos != string::npos)
		str.erase(0, pos);

	pos = str.find_last_not_of(whitespace);
	if (pos != string::npos)
		str.erase(pos + 1);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		cerr << "Syntax: " << argv[0] << " <file.message>" << endl;
		return EXIT_FAILURE;
	}

	char *pos;
	pos = strrchr(argv[1], '.');

	if (pos == NULL || strcmp(pos, ".message") != 0) {
		cerr << "Input filename must have the '.message' extension." << endl;
		return EXIT_FAILURE;
	}

	char *headername, *implname;
	headername = strdup(argv[1]);
	strcpy(&(headername[pos - argv[1]]), ".h");

	implname = strdup(argv[1]);
	strcpy(&(implname[pos - argv[1]]), ".cpp");

	fstream inputfp, headerfp, implfp;

	inputfp.open(argv[1], fstream::in);
	headerfp.open(headername, fstream::out | fstream::trunc);
	implfp.open(implname, fstream::out | fstream::trunc);

	string line;
	string klass, klassupper, base;
	bool hasclass = false;

	while (true) {
		getline(inputfp, line);

		if (inputfp.fail())
			break;

		if (!hasclass) {
			string::size_type index = line.find(':');

			if (index == string::npos) {
				cerr << "Must specify class and base name." << endl;
				return EXIT_FAILURE;
			}

			klass = line.substr(0, index);
			trim(klass);

			klassupper = klass;
			transform(klassupper.begin(), klassupper.end(), klassupper.begin(), toupper);

			base = line.substr(index + 1);
			trim(base);

			cout << "Class: '" << klass << "' (inherits from: '" << base << "')" << endl;

			headerfp	<< "#ifndef " << klassupper << "_H" << endl
						<< "#define " << klassupper << "_H" << endl
						<< endl
						<< "namespace icinga" << endl
						<< "{" << endl
						<< endl
						<< "class " << klass << " : public " << base << endl
						<< "{" << endl
						<< endl
						<< "public:" << endl
						<< "\ttypedef shared_ptr<" << klass << "> Ptr;" << endl
						<< "\ttypedef weak_ptr<" << klass << "> WeakPtr;" << endl
						<< endl
						<< "\t" << klass << "(void) : " << base << "() { }" << endl
						<< "\t" << klass << "(const Message::Ptr& message) : " << base << "(message) { }" << endl
						<< endl;

			implfp		<< "#include \"i2-jsonrpc.h\"" << endl
						<< "#include \"" << headername << "\"" << endl
						<< endl
						<< "using namespace icinga;" << endl
						<< endl;

			hasclass = true;
		} else {
			string::size_type index = line.find(':');

			if (index == string::npos) {
				cerr << "Must specify type and property name." << endl;
				return EXIT_FAILURE;
			}

			string prop = line.substr(0, index);
			trim(prop);

			string type = line.substr(index + 1);
			trim(type);

			string typeaccessor = type;
			typeaccessor[0] = toupper(typeaccessor[0]);

			string rawtype = type;

			/* assume it's a reference type if we don't know the type */
			if (type != "int" && type != "string") {
				type = type + "::Ptr";
				typeaccessor = "Message";
			}

			cout << "Property: '" << prop << "' (Type: '" << type << "')" << endl;

			headerfp	<< endl
						<< "\tbool Get" << prop << "(" << type << " *value);" << endl
						<< "\tvoid Set" << prop << "(const " << type << "& value);" << endl;

			implfp		<< "bool " << klass << "::Get" << prop << "(" << type << " *value)" << endl
						<< "{" << endl;

			if (typeaccessor == "Message") {
				implfp		<< "\tMessage::Ptr message;" << endl
							<< endl
							<< "\tif (!GetProperty" << typeaccessor << "(\"" << prop << "\", &message))" << endl
							<< "\treturn false;" << endl
							<< endl
							<< "\t*value = message->Cast<" + rawtype + ">();" << endl
							<< "return true;" << endl
							<< endl;
			} else {
				implfp		<< "\treturn GetProperty" << typeaccessor << "(\"" << prop << "\", value);" << endl;
			}
	
			implfp		<< "}" << endl
						<< endl;

			implfp		<< "void " << klass << "::Set" << prop << "(const " << type << "& value)" << endl
						<< "{" << endl
						<< "\tSetProperty" << typeaccessor << "(\"" << prop << "\", value);" << endl
						<< "}" << endl
						<< endl;
		}
	}

	headerfp	<< endl
				<< "};" << endl
				<< endl
				<< "}" << endl
				<< endl
				<< "#endif /* " << klassupper << "_H */" << endl;

	inputfp.close();
	headerfp.close();
	implfp.close();

	free(headername);
	free(implname);

	return EXIT_SUCCESS;
}
