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

#include "config/expression.hpp"
#include "config/vmops.hpp"
#include "base/object.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

void LiteralExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	if (m_Value.IsString())
		fp << "String(\"" << m_Value << "\")";
	else if (m_Value.IsNumber())
		fp << m_Value;
	else if (m_Value.IsEmpty())
		fp << "Value()";
	else
		throw std::invalid_argument("Literal expression has invalid type: " + m_Value.GetTypeName());
}

void VariableExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "VMOps::Variable(context, \"" << m_Variable << "\")";
}

void NegateExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "~(long)(";
	m_Operand->GenerateCode(definitions, fp);
	fp << ")";
}

void LogicalNegateExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "!(";
	m_Operand->GenerateCode(definitions, fp);
	fp << ")";
}

void AddExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") + (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void SubtractExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") - (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void MultiplyExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") * (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void DivideExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") / (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void BinaryAndExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") & (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void BinaryOrExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") | (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void ShiftLeftExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") << (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void ShiftRightExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") >> (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void EqualExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") == (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void NotEqualExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") != (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void LessThanExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") < (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void GreaterThanExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") > (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void LessThanOrEqualExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") <= (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void GreaterThanOrEqualExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ") >= (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")";
}

void InExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "!static_cast<Array::Ptr>(";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")->Contains(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ")";
}

void NotInExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "!static_cast<Array::Ptr>(";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ")->Contains(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ")";
}

void LogicalAndExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ").ToBool() && (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ").ToBool()";
}

void LogicalOrExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "(";
	m_Operand1->GenerateCode(definitions, fp);
	fp << ").ToBool() || (";
	m_Operand2->GenerateCode(definitions, fp);
	fp << ").ToBool()";
}

void FunctionCallExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_fcall_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  Value funcName = (";
	m_FName->GenerateCode(definitions, df);
	df << ");" << "\n"
	   << "  std::vector<Value> args;" << "\n";

	BOOST_FOREACH(Expression *expr, m_Args) {
		df << "  args.push_back(";
		expr->GenerateCode(definitions, df);
		df << ");" << "\n";
	}

	df << "  return VMOps::FunctionCall(context, funcName, args);" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void ArrayExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_array_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  Array::Ptr result = new Array();" << "\n";

	BOOST_FOREACH(Expression *aexpr, m_Expressions) {
		df << "  result->Add(";
		aexpr->GenerateCode(definitions, df);
		df << ");" << "\n";
	}

	df << "  return result;" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void DictExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_dict_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& ucontext)" << "\n"
	   << "{" << "\n";

	if (!m_Inline) {
		df << "  Dictionary::Ptr result = new Dictionary();" << "\n"
		   << "  result->Set(\"__parent\", ucontext);" << "\n"
		   << "  Object::Ptr context = result;" << "\n";
	} else
		df << "Object::Ptr context = ucontext;" << "\n";

	df << "  do {" << "\n";

	BOOST_FOREACH(Expression *expression, m_Expressions) {
		df << "    ";
		expression->GenerateCode(definitions, df);
		df << ";" << "\n"
		   << "    if (Expression::HasField(context, \"__result\"))" << "\n"
		   << "      break;" << "\n";
	}

	df << "  } while (0);" << "\n"
	   << "\n";

	if (!m_Inline) {
		df << "  Dictionary::Ptr xresult = result->ShallowClone();" << "\n"
		   << "  xresult->Remove(\"__parent\");" << "\n"
		   << "  return xresult;" << "\n";
	} else
		df << "  return Empty;" << "\n";

	df << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void SetExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_set_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  Value parent, object;" << "\n"
	   << "  String tempindex, index;" << "\n";

	for (Array::SizeType i = 0; i < m_Indexer.size(); i++) {
		Expression *indexExpr = m_Indexer[i];
		df << "  tempindex = (";
		indexExpr->GenerateCode(definitions, df);
		df << ");" << "\n";

		if (i == 0)
			df << "  parent = context;" << "\n";
		else
			df << "  parent = object;" << "\n";

		if (i == m_Indexer.size() - 1) {
			df << "  index = tempindex" << ";" << "\n";

			/* No need to look up the last indexer's value if this is a direct set */
			if (m_Op == OpSetLiteral)
				break;
		}

		df << "  object = VMOps::Indexer(context, parent, tempindex);" << "\n";

		if (i != m_Indexer.size() - 1) {
			df << "  if (object.IsEmpty()) {" << "\n"
			   << "    object = new Dictionary();" << "\n"
			   << "    Expression::SetField(parent, tempindex, object);" << "\n"
			   << "  }" << "\n";
		}
	}

	df << "  Value right = (";
	m_Operand2->GenerateCode(definitions, df);
	df << ");" << "\n";

	if (m_Op != OpSetLiteral) {
		String opcode;

		switch (m_Op) {
			case OpSetAdd:
				opcode = "+";
				break;
			case OpSetSubtract:
				opcode = "-";
				break;
			case OpSetMultiply:
				opcode = "*";
				break;
			case OpSetDivide:
				opcode = "/";
				break;
			default:
				VERIFY(!"Invalid opcode.");
		}

		df << "  right = object " << opcode << " right;" << "\n";
	}

	df << "  Expression::SetField(parent, index, right);" << "\n"
	   << "  return right;" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void IndexerExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_indexer_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  return VMOps::Indexer(context, (";
	m_Operand1->GenerateCode(definitions, df);
	df << "), (";
	m_Operand2->GenerateCode(definitions, df);
	df << "));" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void ImportExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_import_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  String name = (";
	m_Name->GenerateCode(definitions, df);
	df << ");" << "\n"
	   << "  String type = (";
	m_Type->GenerateCode(definitions, df);
	df << ");" << "\n"
	   << "\n"
	   << "  ConfigItem::Ptr item = ConfigItem::GetObject(type, name);" << "\n"
	   << "\n"
	   << "  if (!item)" << "\n"
	   << "    BOOST_THROW_EXCEPTION(ConfigError(\"Import references unknown template: '\" + name + \"' of type '\" + type + \"'\"));" << "\n"
	   << "\n"
	   << "  item->GetExpression()->Evaluate(context);" << "\n"
	   << "\n"
	   << "  return Empty;" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void FunctionExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_function_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  std::vector<String> args;" << "\n";

	BOOST_FOREACH(const String& arg, m_Args)
		df << "  args.push_back(\"" << arg << "\");" << "\n";

	df << "  return VMOps::NewFunction(context, \"" << m_Name << "\", args, boost::make_shared<NativeExpression>("
	   << CodeGenExpression(definitions, m_Expression.get()) << "));" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void SlotExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	fp << "VMOps::NewSlot(context, \"" << m_Signal << "\", ";
	m_Slot->GenerateCode(definitions, fp);
	fp << ")";
}

void ApplyExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_apply_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  boost::shared_ptr<Expression> fterm;" << "\n";

	if (m_FTerm)
		df << "  fterm = boost::make_shared<NativeExpression>(" << CodeGenExpression(definitions, m_FTerm.get()) << ");" << "\n";

	df << "  boost::shared_ptr<Expression> filter = boost::make_shared<NativeExpression>(" << CodeGenExpression(definitions, m_Filter.get()) << ");" << "\n"
	   << "  boost::shared_ptr<Expression> expression = boost::make_shared<NativeExpression>(" << CodeGenExpression(definitions, m_Expression.get()) << ");" << "\n"
	   << "  return VMOps::NewApply(context, \"" << m_Type << "\", \"" << m_Target << "\", (";
	m_Name->GenerateCode(definitions, df);
	df << "), filter, "
	   << "\"" << m_FKVar << "\", \"" << m_FVVar << "\", fterm, expression);" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void ObjectExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_object_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  String name;" << "\n";

	if (m_Name) {
		df << "  name = (";
		m_Name->GenerateCode(definitions, df);
		df << ");" << "\n";
	}

	df << "  boost::shared_ptr<Expression> filter;" << "\n";

	if (m_Filter)
		df << "  filter = boost::make_shared<NativeExpression>("
		   << CodeGenExpression(definitions, m_Filter.get()) << ");" << "\n";

	df << "  boost::shared_ptr<Expression> expression = boost::make_shared<NativeExpression>("
	   << CodeGenExpression(definitions, m_Expression.get()) << ");" << "\n"
	   << "  return VMOps::NewObject(context, " << m_Abstract << ", \"" << m_Type << "\", name, filter, \"" << m_Zone << "\", expression);" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

void ForExpression::GenerateCode(DefinitionMap& definitions, std::ostream& fp) const
{
	std::ostringstream namebuf, df;

	namebuf << "native_for_" << reinterpret_cast<uintptr_t>(this);

	df << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
	   << "{" << "\n"
	   << "  static NativeExpression expression("
	   << CodeGenExpression(definitions, m_Expression) << ");" << "\n"
	   << "  return VMOps::For(context, \"" << m_FKVar << "\", \"" << m_FVVar << "\", (";
	m_Value->GenerateCode(definitions, df);
	df << "), &expression);" << "\n"
	   << "}" << "\n";

	definitions[namebuf.str()] = df.str();

	fp << namebuf.str() << "(context)";
}

String icinga::CodeGenExpression(DefinitionMap& definitions, Expression *expression)
{
	std::ostringstream namebuf, definitionbuf;

	namebuf << "native_expression_" << reinterpret_cast<uintptr_t>(expression);

	definitionbuf << "static Value " << namebuf.str() << "(const Object::Ptr& context)" << "\n"
		      << "{" << "\n"
		      << "  return (";

	expression->GenerateCode(definitions, definitionbuf);

	definitionbuf << ");" << "\n"
		      << "}" << "\n";

	definitions[namebuf.str()] = definitionbuf.str();

	return namebuf.str();
}
