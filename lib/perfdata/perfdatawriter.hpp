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

#ifndef PERFDATAWRITER_H
#define PERFDATAWRITER_H

#include "perfdata/perfdatawriter.thpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/timer.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga perfdata writer.
 *
 * @ingroup icinga
 */
class PerfdataWriter final : public ObjectImpl<PerfdataWriter>
{
public:
	DECLARE_OBJECT(PerfdataWriter);
	DECLARE_OBJECTNAME(PerfdataWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	virtual void ValidateHostFormatTemplate(const String& value, const ValidationUtils& utils) override;
	virtual void ValidateServiceFormatTemplate(const String& value, const ValidationUtils& utils) override;

protected:
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

private:
	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	static Value EscapeMacroMetric(const Value& value);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler(void);

	std::ofstream m_ServiceOutputFile;
	std::ofstream m_HostOutputFile;
	void RotateFile(std::ofstream& output, const String& temp_path, const String& perfdata_path);
};

}

#endif /* PERFDATAWRITER_H */
