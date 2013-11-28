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

#include "checker/checkercomponent.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(CheckerComponent);

void CheckerComponent::OnConfigLoaded(void)
{
	DynamicObject::OnStarted.connect(bind(&CheckerComponent::ObjectHandler, this, _1));
	DynamicObject::OnStopped.connect(bind(&CheckerComponent::ObjectHandler, this, _1));
	DynamicObject::OnAuthorityChanged.connect(bind(&CheckerComponent::ObjectHandler, this, _1));

	Service::OnNextCheckChanged.connect(bind(&CheckerComponent::NextCheckChangedHandler, this, _1));
}

void CheckerComponent::Start(void)
{
	DynamicObject::Start();

	m_Stopped = false;

	m_Thread = boost::thread(boost::bind(&CheckerComponent::CheckThreadProc, this));

	m_ResultTimer = make_shared<Timer>();
	m_ResultTimer->SetInterval(5);
	m_ResultTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::ResultTimerHandler, this));
	m_ResultTimer->Start();
}

void CheckerComponent::Stop(void)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Stopped = true;
		m_CV.notify_all();
	}

	m_Thread.join();
}

void CheckerComponent::CheckThreadProc(void)
{
	Utility::SetThreadName("Check Scheduler");

	boost::mutex::scoped_lock lock(m_Mutex);

	for (;;) {
		typedef boost::multi_index::nth_index<ServiceSet, 1>::type CheckTimeView;
		CheckTimeView& idx = boost::get<1>(m_IdleServices);

		while (idx.begin() == idx.end() && !m_Stopped)
			m_CV.wait(lock);

		if (m_Stopped)
			break;

		CheckTimeView::iterator it = idx.begin();
		Service::Ptr service = *it;

		if (!service->HasAuthority("checker")) {
			m_IdleServices.erase(service);

			continue;
		}

		double wait = service->GetNextCheck() - Utility::GetTime();

		if (wait > 0) {
			/* Wait for the next check. */
			m_CV.timed_wait(lock, boost::posix_time::milliseconds(wait * 1000));

			continue;
		}

		m_IdleServices.erase(service);

		bool forced = service->GetForceNextCheck();
		bool check = true;

		if (!forced) {
			if (!service->GetEnableActiveChecks() || !IcingaApplication::GetInstance()->GetEnableChecks()) {
				Log(LogDebug, "checker", "Skipping check for service '" + service->GetName() + "': active checks are disabled");
				check = false;
			}

			TimePeriod::Ptr tp = service->GetCheckPeriod();

			if (tp && !tp->IsInside(Utility::GetTime())) {
				Log(LogDebug, "checker", "Skipping check for service '" + service->GetName() + "': not in check_period");
				check = false;
			}
		}

		/* reschedule the service if checks are disabled */
		if (!check) {
			m_IdleServices.insert(service);
			lock.unlock();

			service->UpdateNextCheck();

			lock.lock();

			continue;
		}

		m_PendingServices.insert(service);

		lock.unlock();

		if (forced) {
			ObjectLock olock(service);
			service->SetForceNextCheck(false);
		}

		Log(LogDebug, "checker", "Executing service check for '" + service->GetName() + "'");

		CheckerComponent::Ptr self = GetSelf();
		m_Pool.Post(boost::bind(&CheckerComponent::ExecuteCheckHelper, self, service));

		lock.lock();
	}
}

void CheckerComponent::ExecuteCheckHelper(const Service::Ptr& service)
{
	try {
		service->ExecuteCheck();
	} catch (const std::exception& ex) {
		Log(LogCritical, "checker", "Exception occured while checking service '" + service->GetName() + "': " + DiagnosticInformation(ex));
	}

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		/* remove the service from the list of pending services; if it's not in the
		 * list this was a manual (i.e. forced) check and we must not re-add the
		 * service to the services list because it's already there. */
		CheckerComponent::ServiceSet::iterator it;
		it = m_PendingServices.find(service);
		if (it != m_PendingServices.end()) {
			m_PendingServices.erase(it);

			if (service->IsActive() && service->HasAuthority("checker"))
				m_IdleServices.insert(service);

			m_CV.notify_all();
		}
	}

	Log(LogDebug, "checker", "Check finished for service '" + service->GetName() + "'");
}

void CheckerComponent::ResultTimerHandler(void)
{
	Log(LogDebug, "checker", "ResultTimerHandler entered.");

	std::ostringstream msgbuf;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		msgbuf << "Pending services: " << m_PendingServices.size() << "; Idle services: " << m_IdleServices.size() << "; Checks/s: " << CIB::GetActiveChecksStatistics(5) / 5.0;
	}

	Log(LogInformation, "checker", msgbuf.str());
}

void CheckerComponent::ObjectHandler(const DynamicObject::Ptr& object)
{
	if (object->GetType() != DynamicType::GetByName("Service"))
		return;

	Service::Ptr service = static_pointer_cast<Service>(object);

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		if (object->IsActive() && object->HasAuthority("checker")) {
			if (m_PendingServices.find(service) != m_PendingServices.end())
				return;

			m_IdleServices.insert(service);
		} else {
			m_IdleServices.erase(service);
			m_PendingServices.erase(service);
		}

		m_CV.notify_all();
	}
}

void CheckerComponent::NextCheckChangedHandler(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	/* remove and re-insert the service from the set in order to force an index update */
	typedef boost::multi_index::nth_index<ServiceSet, 0>::type ServiceView;
	ServiceView& idx = boost::get<0>(m_IdleServices);

	ServiceView::iterator it = idx.find(service);
	if (it == idx.end())
		return;

	idx.erase(service);
	idx.insert(service);
	m_CV.notify_all();
}
