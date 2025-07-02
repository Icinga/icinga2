/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef HTTPUTILITY_H
#define HTTPUTILITY_H

#include "remote/url.hpp"
#include "base/dictionary.hpp"
#include <boost/beast/http.hpp>

namespace icinga
{

/**
 * Helper functions.
 *
 * @ingroup remote
 */
class HttpUtility
{

public:
	static Value GetLastParameter(const Dictionary::Ptr& params, const String& key);
};

}

#endif /* HTTPUTILITY_H */
