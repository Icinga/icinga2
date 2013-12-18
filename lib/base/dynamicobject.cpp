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

#include "base/dynamicobject.h"
#include "base/dynamictype.h"
#include "base/serializer.h"
#include "base/netstring.h"
#include "base/registry.h"
#include "base/stdiostream.h"
#include "base/debug.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/scriptfunction.h"
#include "base/initialize.h"
#include "base/scriptvariable.h"
#include <fstream>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

using namespace icinga;

REGISTER_TYPE(DynamicObject);
INITIALIZE_ONCE(&DynamicObject::StaticInitialize);

boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStarted;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStopped;
boost::signals2::signal<void (const DynamicObject::Ptr&)> DynamicObject::OnStateChanged;
boost::signals2::signal<void (const DynamicObject::Ptr&, const String&, bool)> DynamicObject::OnAuthorityChanged;

void DynamicObject::StaticInitialize(void)
{
	ScriptVariable::Set("DomainPrivRead", DomainPrivRead, true, true);
	ScriptVariable::Set("DomainPrivCheckResult", DomainPrivCheckResult, true, true);
	ScriptVariable::Set("DomainPrivCommand", DomainPrivCommand, true, true);

	ScriptVariable::Set("DomainPrevReadOnly", DomainPrivRead, true, true);
	ScriptVariable::Set("DomainPrivReadWrite", DomainPrivRead | DomainPrivCheckResult | DomainPrivCommand, true, true);
}

DynamicObject::DynamicObject(void)
{ }

DynamicType::Ptr DynamicObject::GetType(void) const
{
	return DynamicType::GetByName(GetTypeName());
}

bool DynamicObject::IsActive(void) const
{
	return GetActive();
}

void DynamicObject::SetAuthority(const String& type, bool value)
{
	ASSERT(!OwnsLock());

	{
		ObjectLock olock(this);

		bool old_value = HasAuthority(type);

		if (old_value == value)
			return;

		if (GetAuthorityInfo() == NULL)
			SetAuthorityInfo(make_shared<Dictionary>());

		GetAuthorityInfo()->Set(type, value);
	}

	OnAuthorityChanged(GetSelf(), type, value);
}

bool DynamicObject::HasAuthority(const String& type) const
{
	Dictionary::Ptr authorityInfo = GetAuthorityInfo();

	if (!authorityInfo || !authorityInfo->Contains(type))
		return true;

	return authorityInfo->Get(type);
}

void DynamicObject::SetPrivileges(const String& instance, int privs)
{
	m_Privileges[instance] = privs;
}

bool DynamicObject::HasPrivileges(const String& instance, int privs) const
{
	if (privs == 0)
		return true;

	std::map<String, int>::const_iterator it;
	it = m_Privileges.find(instance);

	if (it == m_Privileges.end())
		return false;

	return (it->second & privs) == privs;
}

void DynamicObject::SetExtension(const String& key, const Object::Ptr& object)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions) {
		extensions = make_shared<Dictionary>();
		SetExtensions(extensions);
	}

	extensions->Set(key, object);
}

Object::Ptr DynamicObject::GetExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return Object::Ptr();

	return extensions->Get(key);
}

void DynamicObject::ClearExtension(const String& key)
{
	Dictionary::Ptr extensions = GetExtensions();

	if (!extensions)
		return;

	extensions->Remove(key);
}

void DynamicObject::Register(void)
{
	ASSERT(!OwnsLock());

	DynamicType::Ptr dtype = GetType();
	dtype->RegisterObject(GetSelf());
}

void DynamicObject::Start(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStartCalled(true);
}

void DynamicObject::Activate(void)
{
	ASSERT(!OwnsLock());

	Start();

	ASSERT(GetStartCalled());

	{
		ObjectLock olock(this);
		ASSERT(!IsActive());
		SetActive(true);
	}

	OnStarted(GetSelf());
}

void DynamicObject::Stop(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	SetStopCalled(true);
}

void DynamicObject::Deactivate(void)
{
	ASSERT(!OwnsLock());

	{
		ObjectLock olock(this);

		if (!IsActive())
			return;

		SetActive(false);
	}

	Stop();

	ASSERT(GetStopCalled());

	OnStopped(GetSelf());
}

void DynamicObject::OnConfigLoaded(void)
{
	/* Nothing to do here. */
}

void DynamicObject::OnStateLoaded(void)
{
	/* Nothing to do here. */
}

Value DynamicObject::InvokeMethod(const String& method,
    const std::vector<Value>& arguments)
{
	Dictionary::Ptr methods;

	methods = GetMethods();

	if (!methods)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Method '" + method + "' does not exist."));

	String funcName = methods->Get(method);

	if (funcName.IsEmpty())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Method '" + method + "' does not exist."));

	ScriptFunction::Ptr func = ScriptFunctionRegistry::GetInstance()->GetItem(funcName);

	if (!func)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Function '" + funcName + "' does not exist."));

	return func->Invoke(arguments);
}

void DynamicObject::DumpObjects(const String& filename, int attributeTypes)
{
	Log(LogInformation, "base", "Dumping program state to file '" + filename + "'");

	String tempFilename = filename + ".tmp";

	std::fstream fp;
	fp.open(tempFilename.CStr(), std::ios_base::out);

	if (!fp)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open '" + filename + "' file"));

	StdioStream::Ptr sfp = make_shared<StdioStream>(&fp, false);

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			Dictionary::Ptr persistentObject = make_shared<Dictionary>();

			persistentObject->Set("type", type->GetName());
			persistentObject->Set("name", object->GetName());

			Dictionary::Ptr update = Serialize(object, attributeTypes);

			if (!update)
				continue;

			persistentObject->Set("update", update);

			String json = JsonSerialize(persistentObject);

			NetString::WriteStringToStream(sfp, json);
		}
	}

	sfp->Close();

	fp.close();

#ifdef _WIN32
	_unlink(filename.CStr());
#endif /* _WIN32 */

	if (rename(tempFilename.CStr(), filename.CStr()) < 0) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("rename")
		    << boost::errinfo_errno(errno)
		    << boost::errinfo_file_name(tempFilename));
	}
}

void DynamicObject::RestoreObjects(const String& filename, int attributeTypes)
{
	Log(LogInformation, "base", "Restoring program state from file '" + filename + "'");

	std::fstream fp;
	fp.open(filename.CStr(), std::ios_base::in);

	StdioStream::Ptr sfp = make_shared<StdioStream>(&fp, false);

	unsigned long restored = 0;

	String message;
	while (NetString::ReadStringFromStream(sfp, &message)) {
		Dictionary::Ptr persistentObject = JsonDeserialize(message);

		String type = persistentObject->Get("type");
		String name = persistentObject->Get("name");
		Dictionary::Ptr update = persistentObject->Get("update");

		DynamicType::Ptr dt = DynamicType::GetByName(type);

		if (!dt)
			continue;

		DynamicObject::Ptr object = dt->GetObject(name);

		if (object) {
			ASSERT(!object->IsActive());
#ifdef _DEBUG
			Log(LogDebug, "base", "Restoring object '" + name + "' of type '" + type + "'.");
#endif /* _DEBUG */
			Deserialize(object, update, false, attributeTypes);
			object->OnStateLoaded();
		}

		restored++;
	}

	sfp->Close();

	std::ostringstream msgbuf;
	msgbuf << "Restored " << restored << " objects";
	Log(LogInformation, "base", msgbuf.str());
}

void DynamicObject::StopObjects(void)
{
	BOOST_FOREACH(const DynamicType::Ptr& dt, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, dt->GetObjects()) {
			object->Deactivate();
		}
	}
}

DynamicObject::Ptr DynamicObject::GetObject(const String& type, const String& name)
{
	DynamicType::Ptr dtype = DynamicType::GetByName(type);
	return dtype->GetObject(name);
}
