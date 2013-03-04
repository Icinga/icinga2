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

#include "i2-checker.h"

using namespace icinga;

REGISTER_COMPONENT("checker", CheckerComponent);

void CheckerComponent::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("checker", false);

	/* dummy registration so the delegation module knows this is a checker
	   TODO: figure out a better way for this */
	m_Endpoint->RegisterSubscription("checker");

	Service::OnCheckerChanged.connect(bind(&CheckerComponent::CheckerChangedHandler, this, _1));
	Service::OnNextCheckChanged.connect(bind(&CheckerComponent::NextCheckChangedHandler, this, _1));

	m_Stopped = false;

	m_Thread = thread(boost::bind(&CheckerComponent::CheckThreadProc, this));

	m_ResultTimer = boost::make_shared<Timer>();
	m_ResultTimer->SetInterval(5);
	m_ResultTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::ResultTimerHandler, this));
	m_ResultTimer->Start();
}

void CheckerComponent::Stop(void)
{
	m_Endpoint->Unregister();

	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Stopped = true;
		m_CV.notify_all();
	}

	m_Thread.join();
}

void CheckerComponent::CheckThreadProc(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	for (;;) {
		typedef nth_index<ServiceSet, 1>::type CheckTimeView;
		CheckTimeView& idx = boost::get<1>(m_IdleServices);

		while (idx.begin() == idx.end() && !m_Stopped)
			m_CV.wait(lock);

		if (m_Stopped)
			break;

		CheckTimeView::iterator it = idx.begin();
		Service::Ptr service = *it;

		if (!service->IsRegistered()) {
			idx.erase(it);
			continue;
		}

		double wait = service->GetNextCheck() - Utility::GetTime();

		if (wait > 0) {
			/* Make sure the service we just examined can be destroyed while we're waiting. */
			service.reset();

			/* Wait for the next check. */
			if (!m_Stopped)
				m_CV.timed_wait(lock, boost::posix_time::milliseconds(wait * 1000));

			continue;
		}

		m_IdleServices.erase(service);

		/* reschedule the service if checks are currently disabled
		 * for it and this is not a forced check */
		if (!service->GetEnableActiveChecks() && !service->GetForceNextCheck()) {
			Logger::Write(LogDebug, "checker", "Ignoring service check for disabled service: " + service->GetName());

			service->UpdateNextCheck();

			typedef nth_index<ServiceSet, 1>::type CheckTimeView;
			CheckTimeView& idx = boost::get<1>(m_IdleServices);

			idx.insert(service);

			continue;
		}

		{
			ObjectLock olock(service);
			service->SetForceNextCheck(false);
		}

		Logger::Write(LogDebug, "checker", "Executing service check for '" + service->GetName() + "'");

		m_IdleServices.erase(service);
		m_PendingServices.insert(service);

		try {
			CheckerComponent::Ptr self = GetSelf();
			service->BeginExecuteCheck(boost::bind(&CheckerComponent::CheckCompletedHandler, self, service));
		} catch (const exception& ex) {
			Logger::Write(LogCritical, "checker", "Exception occured while checking service '" + service->GetName() + "': " + diagnostic_information(ex));
		}
	}
}

void CheckerComponent::CheckCompletedHandler(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	/* remove the service from the list of pending services; if it's not in the
	 * list this was a manual (i.e. forced) check and we must not re-add the
	 * service to the services list because it's already there. */
	CheckerComponent::ServiceSet::iterator it;
	it = m_PendingServices.find(service);
	if (it != m_PendingServices.end()) {
		m_PendingServices.erase(it);
		m_IdleServices.insert(service);
		m_CV.notify_all();
	}

	Logger::Write(LogDebug, "checker", "Check finished for service '" + service->GetName() + "'");
}

void CheckerComponent::ResultTimerHandler(void)
{
	Logger::Write(LogDebug, "checker", "ResultTimerHandler entered.");

	stringstream msgbuf;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		msgbuf << "Pending services: " << m_PendingServices.size() << "; Idle services: " << m_IdleServices.size();
	}

	Logger::Write(LogInformation, "checker", msgbuf.str());
}

void CheckerComponent::CheckerChangedHandler(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	String checker = service->GetCurrentChecker();

	if (checker == EndpointManager::GetInstance()->GetIdentity() || Endpoint::GetByName(checker) == m_Endpoint) {
		if (m_PendingServices.find(service) != m_PendingServices.end())
			return;

		m_IdleServices.insert(service);
		m_CV.notify_all();
	} else {
		m_IdleServices.erase(service);
		m_PendingServices.erase(service);
		m_CV.notify_all();
	}
}

void CheckerComponent::NextCheckChangedHandler(const Service::Ptr& service)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	/* remove and re-insert the service from the set in order to force an index update */
	typedef nth_index<ServiceSet, 0>::type ServiceView;
	ServiceView& idx = boost::get<0>(m_IdleServices);

	ServiceView::iterator it = idx.find(service);
	if (it == idx.end())
		return;

	idx.erase(service);
	idx.insert(service);
	m_CV.notify_all();
}
