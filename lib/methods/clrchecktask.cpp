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

#include "methods/clrchecktask.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread/once.hpp>
#include <objbase.h>
#include <mscoree.h>

#import "mscorlib.tlb"
#pragma comment(lib, "mscoree.lib")

using namespace icinga;

REGISTER_SCRIPTFUNCTION_NS(Internal, ClrCheck,  &ClrCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

static boost::once_flag l_OnceFlag = BOOST_ONCE_INIT;

static boost::mutex l_ObjectsMutex;
static std::map<Checkable::Ptr, variant_t> l_Objects;

static mscorlib::_AppDomainPtr l_AppDomain;

static void InitializeClr()
{
	ICorRuntimeHost *runtimeHost;

	if (FAILED(CorBindToRuntimeEx(nullptr, nullptr,
		STARTUP_LOADER_OPTIMIZATION_SINGLE_DOMAIN | STARTUP_CONCURRENT_GC,
		CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (void **)&runtimeHost))) {
		return;
	}

	runtimeHost->Start();

	IUnknownPtr punkAppDomain = nullptr;
	runtimeHost->GetDefaultDomain(&punkAppDomain);

	punkAppDomain->QueryInterface(__uuidof(mscorlib::_AppDomain), (void **)&l_AppDomain);

	runtimeHost->Release();
}

static variant_t CreateClrType(const String& assemblyName, const String& typeName)
{
	boost::call_once(l_OnceFlag, &InitializeClr);

	try {
		mscorlib::_ObjectHandlePtr pObjectHandle;
		pObjectHandle = l_AppDomain->CreateInstanceFrom(assemblyName.CStr(), typeName.CStr());

		return pObjectHandle->Unwrap();
	} catch (_com_error& error) {
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not load .NET type: " + String(error.Description())));
	}
}

static variant_t InvokeClrMethod(const variant_t& vtObject, const String& methodName, const Dictionary::Ptr& args)
{
	CLSID clsid;
	HRESULT hr = CLSIDFromProgID(L"System.Collections.Hashtable", &clsid);

	mscorlib::IDictionaryPtr pHashtable;
	CoCreateInstance(clsid, nullptr, CLSCTX_ALL, __uuidof(mscorlib::IDictionary), (void **)&pHashtable);

	ObjectLock olock(args);
	for (const Dictionary::Pair& kv : args) {
		String value = kv.second;
		pHashtable->Add(kv.first.CStr(), value.CStr());
	}

	mscorlib::_ObjectPtr pObject;
	vtObject.pdispVal->QueryInterface(__uuidof(mscorlib::_Object), (void**)&pObject);
	mscorlib::_TypePtr pType = pObject->GetType();

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);

	variant_t vtHashtable = static_cast<IUnknown *>(pHashtable);
	LONG idx = 0;
	SafeArrayPutElement(psa, &idx, &vtHashtable);

	variant_t result = pType->InvokeMember_3(methodName.CStr(),
		mscorlib::BindingFlags_InvokeMethod,
		nullptr,
		vtObject,
		psa);

	SafeArrayDestroy(psa);

	return result;
}

static void FillCheckResult(const CheckResult::Ptr& cr, variant_t vtResult)
{
	mscorlib::_ObjectPtr pObject;
	vtResult.pdispVal->QueryInterface(__uuidof(mscorlib::_Object), (void**)&pObject);
	mscorlib::_TypePtr pType = pObject->GetType();

	SAFEARRAY *psa = SafeArrayCreateVector(VT_VARIANT, 0, 0);
	int lState = pType->InvokeMember_3("State",
		mscorlib::BindingFlags_GetField,
		nullptr,
		vtResult,
		psa);
	cr->SetState(static_cast<ServiceState>(lState));
	bstr_t sOutput = pType->InvokeMember_3("Output",
		mscorlib::BindingFlags_GetField,
		nullptr,
		vtResult,
		psa);
	cr->SetOutput(static_cast<const char *>(sOutput));
	bstr_t sPerformanceData = pType->InvokeMember_3("PerformanceData",
		mscorlib::BindingFlags_GetField,
		nullptr,
		vtResult,
		psa);
	SafeArrayDestroy(psa);
	cr->SetPerformanceData(PluginUtility::SplitPerfdata(static_cast<const char *>(sPerformanceData)));
}

void ClrCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();
	Value raw_command = commandObj->GetCommandLine();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	Dictionary::Ptr envMacros = new Dictionary();

	Dictionary::Ptr env = commandObj->GetEnv();

	if (env) {
		ObjectLock olock(env);
		for (const Dictionary::Pair& kv : env) {
			String name = kv.second;

			Value value = MacroProcessor::ResolveMacros(name, resolvers, checkable->GetLastCheckResult(),
				nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

			envMacros->Set(kv.first, value);
		}
	}

	variant_t vtObject;

	{
		boost::mutex::scoped_lock lock(l_ObjectsMutex);

		auto it = l_Objects.find(checkable);

		if (it != l_Objects.end()) {
			vtObject = it->second;
		} else {
			String clr_assembly = MacroProcessor::ResolveMacros("$clr_assembly$", resolvers, checkable->GetLastCheckResult(),
				nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);
			String clr_type = MacroProcessor::ResolveMacros("$clr_type$", resolvers, checkable->GetLastCheckResult(),
				nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

			if (resolvedMacros && !useResolvedMacros)
				return;

			vtObject = CreateClrType(clr_assembly, clr_type);
			l_Objects[checkable] = vtObject;
		}
	}

	try {
		variant_t vtResult = InvokeClrMethod(vtObject, "Check", envMacros);
		FillCheckResult(cr, vtResult);
		checkable->ProcessCheckResult(cr);
	} catch (_com_error& error) {
		cr->SetOutput("Failed to invoke .NET method: " + String(error.Description()));
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
	}
}
