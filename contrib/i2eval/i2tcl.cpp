#include "i2tcl.hpp"
#include "config/configcompiler.hpp"
#include "config/configcompilercontext.hpp"
#include "base/function.hpp"
#include "base/json.hpp"
#include "base/application.hpp"
#include <boost/algorithm/string.hpp>

using namespace icinga;

static bool l_Init_Called;
static Tcl_Interp *l_Interp;
static Tcl_Encoding l_Encoding;
static std::map<String, String> l_Lines;
static int l_NextLine = 1;

static Value i2_call_tcl(const String& command, const String& mtype, const std::vector<Value>& args)
{
	Tcl_Obj **objv = new Tcl_Obj *[args.size() + 1];
	objv[0] = Tcl_NewStringObj(command.CStr(), -1);
	Tcl_IncrRefCount(objv[0]);

	for (size_t i = 0; i < args.size(); i++) {
		Tcl_DString dsText;
		String arg = static_cast<String>(args[i]);
		Tcl_ExternalToUtfDString(l_Encoding, arg.CStr(), -1, &dsText);
		objv[i + 1] = Tcl_NewStringObj(Tcl_DStringValue(&dsText), Tcl_DStringLength(&dsText));
		Tcl_DStringFree(&dsText);
		Tcl_IncrRefCount(objv[i + 1]);
	}

	int code = Tcl_EvalObjv(l_Interp, args.size() + 1, objv, TCL_EVAL_GLOBAL);

	Tcl_Obj *result = Tcl_GetObjResult(l_Interp);

	for (size_t i = 0; i < args.size() + 1; i++)
		Tcl_DecrRefCount(objv[i]);

	delete [] objv;

	if (code == TCL_ERROR)
		BOOST_THROW_EXCEPTION(std::runtime_error("An error occurred in the TCL script"));

	Value vresult;

	if (mtype == "list") {
		Array::Ptr arr = new Array();

		int len;
		if (Tcl_ListObjLength(l_Interp, result, &len) != TCL_OK)
			BOOST_THROW_EXCEPTION(std::invalid_argument("TCL proc returned something that is not a list"));

		for (size_t i = 0; i < len; i++) {
			Tcl_Obj *obj;
			Tcl_ListObjIndex(l_Interp, result, i, &obj);

			const char* strObj = Tcl_GetString(obj);

			Tcl_DString dsObj;
			arr->Add(Tcl_UtfToExternalDString(l_Encoding, strObj, -1, &dsObj));
			Tcl_DStringFree(&dsObj);
		}

		vresult = arr;
	} else if (mtype == "null") {
		/* Nothing to do here */
	} else if (mtype == "number") {
		const char* strResult = Tcl_GetString(result);
		vresult = Convert::ToDouble(strResult);
	} else if (mtype == "bool") {
		const char* strResult = Tcl_GetString(result);
		vresult = Convert::ToBool(Convert::ToLong(strResult));
	} else {
		const char* strResult = Tcl_GetString(result);

		Tcl_DString dsResult;
		vresult = Tcl_UtfToExternalDString(l_Encoding, strResult, -1, &dsResult);
		Tcl_DStringFree(&dsResult);
	}


	return vresult;
}

void i2_register_command(const char *icmd, const char *tcmd, const char *mtype, Tcl_Interp *interp)
{
	Function::Ptr sf = new Function(icmd, boost::bind(i2_call_tcl, String(tcmd), String(mtype), _1));
	ScriptGlobal::Set(icmd, sf);
}

void *i2_new_frame(Tcl_Interp *interp)
{
	if (!l_Init_Called) {
		l_Init_Called = true;
		l_Encoding = Tcl_GetEncoding(l_Interp, "ISO8859-1");
		Application::InitializeBase();
	}

	ScriptFrame *frame = new ScriptFrame();
	frame->Self = ScriptGlobal::GetGlobals();
	return frame;
}

void i2_free_frame(void *frame, Tcl_Interp *interp)
{
	delete reinterpret_cast<ScriptFrame *>(frame);
}

char *i2_eval(void *uframe, const char *text, Tcl_Interp *interp)
{
	std::ostringstream msgbuf;
	Expression *expr;
	ScriptFrame *frame = reinterpret_cast<ScriptFrame *>(uframe);

	l_Interp = interp;

	try {
		String lineNum = Convert::ToString(l_NextLine);
		l_NextLine++;

		String fileName = "<" + lineNum + ">";
		l_Lines[fileName] = text;

		expr = ConfigCompiler::CompileText(fileName, text);

		if (expr) {
			Value result = expr->Evaluate(*frame);
			if (!result.IsObject() || result.IsObjectType<Array>() || result.IsObjectType<Dictionary>())
				msgbuf << JsonEncode(result);
			else
				msgbuf << result;
		}
	} catch (const ScriptError& ex) {
		DebugInfo di = ex.GetDebugInfo();

		if (di.FirstLine != 0) {
			String text = l_Lines[di.Path];

			std::vector<String> lines;
			boost::algorithm::split(lines, text, boost::is_any_of("\n"));

			for (int i = di.FirstLine; i <= di.LastLine; i++) {
				int start, len;

				if (i == di.FirstLine)
					start = di.FirstColumn;
				else
					start = 0;

				if (i == di.LastLine)
					len = di.LastColumn - di.FirstColumn + 1;
				else
					len = lines[i].GetLength();

				String pathInfo = di.Path;
				if (i != 1)
					pathInfo += "(" + Convert::ToString(i) + ")";
				pathInfo += ": ";

				msgbuf << pathInfo << lines[i - 1] << "\n";
				msgbuf << String(pathInfo.GetLength(), ' ');
				msgbuf << String(start, ' ') << String(len, '^') << "\n";
			}
		}

		msgbuf << ex.what();
	} catch (const std::exception& ex) {
		msgbuf << "Error: " << DiagnosticInformation(ex);
	}

	delete expr;

	std::string str = msgbuf.str();
	return strdup(str.c_str());
}

