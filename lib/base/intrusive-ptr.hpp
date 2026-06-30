// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/i2-base.hpp"
#include <memory>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/version.hpp>

// std::hash is only implemented starting from Boost 1.74. Implement it ourselves for older version to allow using
// boost::intrusive_ptr inside std::unordered_set<> or as the key of std::unordered_map<>.
// https://github.com/boostorg/smart_ptr/commit/5a18ffdc5609a0e64b63e47cb81c4f0847e0c087
#if BOOST_VERSION < 107400
template<class T>
struct std::hash<boost::intrusive_ptr<T>>
{
	std::size_t operator()(const boost::intrusive_ptr<T>& ptr) const noexcept
	{
		return std::hash<T*>{}(ptr.get());
	}
};
#endif /* BOOST_VERSION < 107400 */
