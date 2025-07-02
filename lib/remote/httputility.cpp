/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/httputility.hpp"
#include "remote/url.hpp"
#include "base/json.hpp"
#include <boost/beast/http.hpp>

using namespace icinga;

Value HttpUtility::GetLastParameter(const Dictionary::Ptr& params, const String& key)
{
	Value varr = params->Get(key);

	if (!varr.IsObjectType<Array>())
		return varr;

	Array::Ptr arr = varr;

	if (arr->GetLength() == 0)
		return Empty;
	else
		return arr->Get(arr->GetLength() - 1);
}
