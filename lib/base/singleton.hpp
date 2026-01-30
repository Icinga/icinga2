// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SINGLETON_H
#define SINGLETON_H

#include "base/i2-base.hpp"

namespace icinga
{

/**
 * A singleton.
 *
 * @ingroup base
 */
template<typename T>
class Singleton
{
public:
	static T *GetInstance()
	{
		static T instance;
		return &instance;
	}
};

}

#endif /* SINGLETON_H */
