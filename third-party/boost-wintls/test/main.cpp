#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "certificate.hpp"
#include "unittest.hpp"

#include <boost/wintls/certificate.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char* argv[]) {
  // Ensure the test are using US English locale. Some tests depend on
  // comparing potentially localized message strings.
  SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));

  boost::system::error_code ec;
  boost::wintls::delete_private_key(test_key_name, ec);
  boost::wintls::import_private_key(net::buffer(test_key), boost::wintls::file_format::pem, test_key_name, ec);
  if (ec) {
    std::cerr << "Unable to import private test key: " << ec.message() << "\n";
    return EXIT_FAILURE;
  }

  int result = Catch::Session().run(argc, argv);

  boost::wintls::delete_private_key(test_key_name, ec);
  if (ec) {
    std::cerr << "Unable to delete private test key: " << ec.message() << "\n";
    return EXIT_FAILURE;
  }

  return result;
}
