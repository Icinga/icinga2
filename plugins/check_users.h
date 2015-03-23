#ifndef CHECK_USERS_H
#define CHECK_USERS_H
#include "thresholds.h"
#include "boost/program_options.hpp"
struct printInfoStruct
{
	threshold warn, crit;
	DOUBLE users;
};

INT parseArguments(INT, WCHAR **, boost::program_options::variables_map&, printInfoStruct&);
INT printOutput(printInfoStruct&);
INT check_users(printInfoStruct&);

#endif // !CHECK_USERS_H
