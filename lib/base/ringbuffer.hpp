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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include <vector>

namespace icinga
{

/**
 * A ring buffer that holds a pre-defined number of integers.
 *
 * @ingroup base
 */
class RingBuffer final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(RingBuffer);

	typedef std::vector<int>::size_type SizeType;

	RingBuffer(SizeType slots);

	SizeType GetLength(void) const;
	void InsertValue(SizeType tv, int num);
	int UpdateAndGetValues(SizeType tv, SizeType span);
	double CalculateRate(SizeType tv, SizeType span);

private:
	std::vector<int> m_Slots;
	SizeType m_TimeValue;
	SizeType m_InsertedValues;
};

}

#endif /* RINGBUFFER_H */
