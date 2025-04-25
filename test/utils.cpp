/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "utils.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <boost/test/unit_test.hpp>

tm make_tm(std::string s)
{
    int dst = -1;
    size_t l = strlen("YYYY-MM-DD HH:MM:SS");
    if (s.size() > l) {
        std::string zone = s.substr(l);
        if (zone == " PST") {
            dst = 0;
        } else if (zone == " PDT") {
            dst = 1;
        } else {
            // tests should only use PST/PDT (for now)
            BOOST_CHECK_MESSAGE(false, "invalid or unknown time time: " << zone);
        }
    }

    std::tm t = {};
    std::istringstream stream(s);
    stream >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
    t.tm_isdst = dst;

    return t;
}
