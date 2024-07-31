//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "unittest.hpp"

#include <boost/wintls/error.hpp>

extern "C" __declspec(dllimport) void __stdcall SetLastError(unsigned long);

TEST_CASE("SECURITY_STATUS error code") {
  auto sc = static_cast<SECURITY_STATUS>(0x80090326);
  auto ec = boost::wintls::error::make_error_code(sc);
  CHECK(ec.value() == sc);
  CHECK(ec.message() == "The message received was unexpected or badly formatted");
}

TEST_CASE("throw last error") {
  boost::system::system_error error{boost::system::error_code{}};
  REQUIRE_FALSE(error.code());

  ::SetLastError(0x00000053);
  try {
    boost::wintls::detail::throw_last_error("YetAnotherUglyWindowsAPIFunctionEx3");
  } catch (const boost::system::system_error& ex) {
    error = ex;
  }
  CHECK(error.code().value() == 0x00000053);
  CHECK(error.code().message() == "Fail on INT 24");
  CHECK_THAT(error.what(), Catch::Contains("YetAnotherUglyWindowsAPIFunctionEx3: Fail on INT 24"));
}
