/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef PERFDATAWRITER_H
#define PERFDATAWRITER_H

#include "perfdata/perfdatawriter.th"
#include "icinga/service.h"
#include "base/dynamicobject.h"
#include "base/timer.h"
#include <fstream>

namespace icinga
{

/**
 * An Icinga perfdata writer.
 *
 * @ingroup icinga
 */
class PerfdataWriter : public ObjectImpl<PerfdataWriter>
{
public:
	DECLARE_PTR_TYPEDEFS(PerfdataWriter);
	DECLARE_TYPENAME(PerfdataWriter);

protected:
	virtual void Start(void);

private:
	void CheckResultHandler(const Service::Ptr& service, const CheckResult::Ptr& cr);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler(void);

	std::ofstream m_OutputFile;
	void RotateFile(void);
};

}

#endif /* PERFDATAWRITER_H */
