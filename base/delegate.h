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

#ifndef DELEGATE_H
#define DELEGATE_H

namespace icinga
{

template<class TObject, class TArgs>
int delegate_fwd(int (TObject::*function)(TArgs), weak_ptr<TObject> wref, TArgs args)
{
	shared_ptr<TObject> ref = wref.lock();

	if (!ref)
		return -1;

	return (ref.get()->*function)(args);
}

template<class TObject, class TArgs>
function<int (TArgs)> bind_weak(int (TObject::*function)(TArgs), const weak_ptr<TObject>& wref)
{
	return bind(&delegate_fwd<TObject, TArgs>, function, wref,
#ifdef HAVE_BOOST
	    _1
#else /* HAVE_BOOST */
	    placeholders::_1
#endif /* HAVE_BOOST */
	);
}

template<class TObject, class TArgs>
function<int (TArgs)> bind_weak(int (TObject::*function)(TArgs), shared_ptr<TObject> ref)
{
	weak_ptr<TObject> wref = weak_ptr<TObject>(ref);
	return bind_weak(function, wref);
}

template<class TObject, class TArgs>
function<int (TArgs)> bind_weak(int (TObject::*function)(TArgs), shared_ptr<Object> ref)
{
	weak_ptr<TObject> wref = weak_ptr<TObject>(static_pointer_cast<TObject>(ref));
	return bind_weak(function, wref);
}

}

#endif /* DELEGATE_H */
