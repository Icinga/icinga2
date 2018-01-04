/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/base64.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_base64)

BOOST_AUTO_TEST_CASE(base64)
{
	std::vector<String> clearText;
	clearText.emplace_back("");
	clearText.emplace_back("1");
	clearText.emplace_back("12");
	clearText.emplace_back("123");
	clearText.emplace_back("1234");
	clearText.emplace_back("VsowLvPqEiAeITDmo-5L_NB-k7fsT3sT2d3K9O4iC2uBk41hvCPAxrgGSxrdeX5s"
		"Zo0Z9b1kxDZlzf8GHQ9ARW6YLeGODMtiZo8cKkUzfSbxyZ_wlE9u6pCTTg9kODCM"
		"Ve-X_a3jWkOy89RoDkT5ahKBY-8S25L6wlvWt8ZyQ2bLxfplzEzuHgEknTMKKp2K"
		"jRlwI2p3gF4FYeQM7dx0E5O782Lh1P3IC6jPNqiZgTgWmsRYZbAN8oU2V626bQxD"
		"n8prQ0Xr_3aPdP7VIVgxNZMnF0NJrQvB_rzq1Dip1UM_xH_9nansbX25E64OQU-r"
		"q54EdO-vb_9FvyqdeVTJ3UTgXIP7OXtz4K8xlEHWdb1-hJChVvDc0KSnN5HVN2NJ"
		"yJrAofVyHBxXGRnGMdN8cOwvxzBFsz2Hih_lIqm1NVULm9_J9GoesY-aN8JzGocU"
		"U3hbhFQBiUlzliuodhwg4RXRcfmPHQRo7kWKaohpySkvqmWcXEAt2LPJ8nH70fW7"
		"vudgzwwWTnNcMlf0Wa-nKL4xXNNPQD0obDCfogN8uKuGqi0DltOUmFK62Zkkb0_d"
		"45grssnD5q89MjDGBkGMXuLY_JLOqc7Y9VV6H48vzoTNK1a2kOGV2TrAD8syuA5Z"
		"o8RLKjTqAYjKTUqEJjg0MflpiBnbDQvRqiSXs1cJuFNXRLpEC5GoqGqMd0zAGn4u"
		"5J3OurVd0SFp8_vkYUI6YwNUe00y8_Dn6DOBh_0KKADphZBgple82_8HrnQNreQn"
		"GkB2TpIsjwWud0yuhI-jQZEMNNlhEYMLwx7B-xTGhn0LFC1pLEXn_kZ2NOgDgUHd"
		"bdj906o3N2Jjo9Fb5GXkCrt-fNEYBjeXvIu73yeTGmsiAzfiICNHi_PmGkgq8fYQ"
		"O9lQgyRHCMic8zU7ffWuSoUPRgHsqztLHaCDbYIrNmgrn2taxcXSb57Xm_l-1xBH"
		"bZqdMvBziapJXaLJmhUg03lgdsIc_OuJmzt-sytDLVGIuNqpa4dETdhLsI7qis4B"
	);

	// 1024 chars

	for (const String& str : clearText) {
		String enc = Base64::Encode(str);
		String dec = Base64::Decode(enc);
		BOOST_CHECK(str == dec);
	}
}

BOOST_AUTO_TEST_SUITE_END()
