/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "base/i2-base.hpp"
#include "base/type.hpp"
#include "base/value.hpp"
#include "base/exception.hpp"

namespace icinga
{

class CircularReferenceError : virtual public user_error
{
public:
	CircularReferenceError(String message, std::vector<String> path);
	~CircularReferenceError() throw() = default;

	const char *what(void) const throw() final;
	std::vector<String> GetPath() const;

private:
	String m_Message;
	std::vector<String> m_Path;
};

Value Serialize(const Value& value, int attributeTypes = FAState);
Value Deserialize(const Value& value, bool safe_mode = false, int attributeTypes = FAState);
Value Deserialize(const Object::Ptr& object, const Value& value, bool safe_mode = false, int attributeTypes = FAState);

}

#endif /* SERIALIZER_H */
