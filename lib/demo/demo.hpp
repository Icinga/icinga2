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

#ifndef DEMO_H
#define DEMO_H

#include "demo/demo.thpp"
#include "remote/messageorigin.hpp"
#include "base/timer.hpp"

namespace icinga
{

/**
 * @ingroup demo
 */
class Demo final : public ObjectImpl<Demo>
{
public:
	DECLARE_OBJECT(Demo);
	DECLARE_OBJECTNAME(Demo);

	virtual void Start(bool runtimeCreated) override;

	static Value DemoMessageHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);


private:
	Timer::Ptr m_DemoTimer;

	void DemoTimerHandler();
};

}

#endif /* DEMO_H */
