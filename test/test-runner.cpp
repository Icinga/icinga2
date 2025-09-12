/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#define BOOST_TEST_MODULE icinga2
#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_ALTERNATIVE_INIT_API

#define TEST_INCLUDE_IMPLEMENTATION
#include <BoostTestTargetConfig.h>

int BOOST_TEST_CALL_DECL
main(int argc, char **argv)
{
	std::_Exit(boost::unit_test::unit_test_main(init_unit_test, argc, argv));
	return EXIT_FAILURE;
}

#ifdef _WIN32
#include <boost/test/impl/unit_test_main.ipp>
#include <boost/test/impl/framework.ipp>
#endif /* _WIN32 */
