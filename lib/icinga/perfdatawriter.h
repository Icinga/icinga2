/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

namespace icinga
{

/**
 * An Icinga perfdata writer.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API PerfdataWriter : public DynamicObject
{
public:
	typedef shared_ptr<PerfdataWriter> Ptr;
	typedef weak_ptr<PerfdataWriter> WeakPtr;

	PerfdataWriter(const Dictionary::Ptr& properties);
	~PerfdataWriter(void);

	static PerfdataWriter::Ptr GetByName(const String& name);

	String GetPathPrefix(void) const;
	String GetFormatTemplate(void) const;
	double GetRotationInterval(void) const;

protected:
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);
	virtual void Start(void);

private:
	Attribute<String> m_PathPrefix;
	Attribute<String> m_FormatTemplate;
	Attribute<double> m_RotationInterval;

	Endpoint::Ptr m_Endpoint;
	void CheckResultRequestHandler(const RequestMessage& request);

	Timer::Ptr m_RotationTimer;
	void RotationTimerHandler(void);

	ofstream m_OutputFile;
	void RotateFile(void);
};

}

#endif /* PERFDATAWRITER_H */
