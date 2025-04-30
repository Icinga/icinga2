/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include <ctime>
#include <string>

tm make_tm(std::string s);

struct GlobalTimezoneFixture
{
    /**
     * Timezone used for testing DST changes.
     *
     * DST changes in America/Los_Angeles:
     * 2021-03-14: 01:59:59 PST (UTC-8) -> 03:00:00 PDT (UTC-7)
     * 2021-11-07: 01:59:59 PDT (UTC-7) -> 01:00:00 PST (UTC-8)
     */
    static const char *TestTimezoneWithDST;

    GlobalTimezoneFixture(const char *fixed_tz = "");
    ~GlobalTimezoneFixture();

    char *tz;
};
