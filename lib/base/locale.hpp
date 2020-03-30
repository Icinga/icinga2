/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef LOCALE_H
#define LOCALE_H

#include <boost/locale.hpp>

namespace icinga
{

class LocaleDateTime : public boost::locale::date_time
{
public:
	using date_time::date_time;

	void set(boost::locale::period::period_type f, int v);
};

}

#endif /* LOCALE_H */
