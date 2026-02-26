// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "icinga/host.hpp"
#include <ctime>
#include <functional>
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

std::string GetRandomString(std::string prefix, std::size_t length);

void ReceiveCheckResults(
	const icinga::Checkable::Ptr& host,
	std::size_t num,
	icinga::ServiceState state,
	const std::function<void(const icinga::CheckResult::Ptr&)>& fn = {}
);
