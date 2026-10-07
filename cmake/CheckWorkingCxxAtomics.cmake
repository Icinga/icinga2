# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(CheckCXXSourceCompiles)

function(check_working_cxx_atomics outvar)
check_cxx_source_compiles("
#include <atomic>

int main() {
    // This is the exact type used by `lib/base/object.hpp`
    std::atomic<uint_fast64_t> x{};
    x.fetch_add(1);
    x.fetch_sub(1);
    return 0;
}
"
  ${outvar}
)
endfunction()
