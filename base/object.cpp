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

#include "i2-base.h"

using namespace icinga;

vector<Object::Ptr> Object::m_HeldObjects;

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
{
}

/**
 * Destructor for the Object class.
 */
Object::~Object(void)
{
}

/**
 * Temporarily holds onto a reference for an object. This can
 * be used to safely clear the last reference to an object
 * in an event handler.
 */
void Object::Hold(void)
{
	m_HeldObjects.push_back(shared_from_this());
}

/**
 * Clears all temporarily held objects.
 */
void Object::ClearHeldObjects(void)
{
	m_HeldObjects.clear();
}

