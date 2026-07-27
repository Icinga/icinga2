# SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
# SPDX-License-Identifier: GPL-2.0-or-later

include(CheckCXXSourceCompiles)

function(check_working_cxx_atomics outvar)
check_cxx_source_compiles("
#include <atomic>

int main() {
    std::atomic<int> x{};
    x.fetch_add(1);
    x.fetch_sub(1);
    return 0;
}
"
  ${outvar}
)
endfunction()
