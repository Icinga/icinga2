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

#ifndef GRAPHITEWRITER_H
#define GRAPHITEWRITER_H

#include "icinga/service.h"
#include "base/dynamicobject.h"
#include "base/tcpsocket.h"
#include "base/timer.h"
#include <fstream>

namespace icinga
{

/**
 * An Icinga graphite writer.
 *
 * @ingroup perfdata
 */
class GraphiteWriter : public DynamicObject
{
public:
	DECLARE_PTR_TYPEDEFS(GraphiteWriter);
	DECLARE_TYPENAME(GraphiteWriter);

        String GetHost(void) const;
        String GetPort(void) const;

protected:
	virtual void Start(void);

	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	String m_Host;
	String m_Port;
        Stream::Ptr m_Stream;
        
        Timer::Ptr m_ReconnectTimer;

	void CheckResultHandler(const Service::Ptr& service, const Dictionary::Ptr& cr);
        static void AddServiceMetric(std::vector<String>& metrics, const Service::Ptr& service, const String& name, const Value& value);
        void SendMetrics(const std::vector<String>& metrics);
        static void SanitizeMetric(String& str);
        
        void ReconnectTimerHandler(void);
};

}

#endif /* GRAPHITEWRITER_H */
