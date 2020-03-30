/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/initialize.hpp"
#include "base/locale.hpp"
#include <boost/locale.hpp>
#include <locale>

using namespace icinga;

namespace lc = boost::locale;

INITIALIZE_ONCE([]() {
	std::locale::global(lc::generator()(""));
})

/**
 * Allows e.g. to set month to 30 on 2020-02-29T00:00:00.
 */
void LocaleDateTime::set(lc::period::period_type f, int v)
{
	*this += f * (v - get(f));
}
