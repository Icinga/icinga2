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

void CheckerComponent::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("checker", false);

	/* dummy registration so the delegation module knows this is a checker
	   TODO: figure out a better way for this */
	m_Endpoint->RegisterSubscription("checker");

	Service::OnCheckerChanged.connect(bind(&CheckerComponent::CheckerChangedHandler, this, _1));
	Service::OnNextCheckChanged.connect(bind(&CheckerComponent::NextCheckChangedHandler, this, _1));
	DynamicObject::OnUnregistered.connect(bind(&CheckerComponent::ObjectRemovedHandler, this, _1));

	m_CheckTimer = boost::make_shared<Timer>();
	m_CheckTimer->SetInterval(1);
	m_CheckTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::CheckTimerHandler, this));
	m_CheckTimer->Start();

	m_ResultTimer = boost::make_shared<Timer>();
	m_ResultTimer->SetInterval(5);
	m_ResultTimer->OnTimerExpired.connect(boost::bind(&CheckerComponent::ResultTimerHandler, this));
	m_ResultTimer->Start();
}

void CheckerComponent::Stop(void)
{
	m_Endpoint->Unregister();
}

void CheckerComponent::CheckTimerHandler(void)
{
	Logger::Write(LogDebug, "checker", "CheckTimerHandler entered.");

	double now = Utility::GetTime();
	long tasks = 0;

	int missedServices = 0, missedChecks = 0;

	while (!m_IdleServices.empty()) {
		typedef nth_index<ServiceSet, 1>::type CheckTimeView;
		CheckTimeView& idx = boost::get<1>(m_IdleServices);

		CheckTimeView::iterator it = idx.begin();
		Service::Ptr service = *it;

		if (service->GetNextCheck() > now)
			break;

		idx.erase(it);

		/* reschedule the service if checks are currently disabled
		 * for it and this is not a forced check */
		if (!service->GetEnableActiveChecks()) {
			if (!service->GetForceNextCheck()) {
				Logger::Write(LogDebug, "checker", "Ignoring service check for disabled service: " + service->GetName());

				service->UpdateNextCheck();

				idx.insert(service);

				continue;
			}
		}

		service->SetForceNextCheck(false);

		Dictionary::Ptr cr = service->GetLastCheckResult();

		if (cr) {
			double lastCheck = cr->Get("execution_end");
			int missed = (Utility::GetTime() - lastCheck) / service->GetCheckInterval() - 1;

			if (missed > 0 && !service->GetFirstCheck()) {
				missedChecks += missed;
				missedServices++;
			}
		}

		service->SetFirstCheck(false);

		Logger::Write(LogDebug, "checker", "Executing service check for '" + service->GetName() + "'");

		m_IdleServices.erase(service);
		m_PendingServices.insert(service);

		try {
			service->BeginExecuteCheck(boost::bind(&CheckerComponent::CheckCompletedHandler, this, service));
		} catch (const exception& ex) {
			Logger::Write(LogCritical, "checker", "Exception occured while checking service '" + service->GetName() + "': " + diagnostic_information(ex));
		}

		tasks++;
	}

	Logger::Write(LogDebug, "checker", "CheckTimerHandler: past loop.");

	if (missedServices > 0) {
		stringstream msgbuf;
		msgbuf << "Missed " << missedChecks << " checks for " << missedServices << " services";;
		Logger::Write(LogWarning, "checker", msgbuf.str());
	}

	if (tasks > 0) {
		stringstream msgbuf;
		msgbuf << "CheckTimerHandler: created " << tasks << " task(s)";
		Logger::Write(LogInformation, "checker", msgbuf.str());
	}
}

void CheckerComponent::CheckCompletedHandler(const Service::Ptr& service)
{
	/* remove the service from the list of pending services; if it's not in the
	 * list this was a manual (i.e. forced) check and we must not re-add the
	 * service to the services list because it's already there. */
	CheckerComponent::ServiceSet::iterator it;
	it = m_PendingServices.find(service);
	if (it != m_PendingServices.end()) {
		m_PendingServices.erase(it);
		m_IdleServices.insert(service);
	}

	Logger::Write(LogDebug, "checker", "Check finished for service '" + service->GetName() + "'");
}

void CheckerComponent::ResultTimerHandler(void)
{
	Logger::Write(LogDebug, "checker", "ResultTimerHandler entered.");

	stringstream msgbuf;
	msgbuf << "Pending services: " << m_PendingServices.size() << "; Idle services: " << m_IdleServices.size();
	Logger::Write(LogInformation, "checker", msgbuf.str());
}

void CheckerComponent::CheckerChangedHandler(const Service::Ptr& service)
{
	String checker = service->GetChecker();

	if (checker == EndpointManager::GetInstance()->GetIdentity() || checker == m_Endpoint->GetName()) {
		if (m_PendingServices.find(service) != m_PendingServices.end())
			return;

		m_IdleServices.insert(service);
	} else {
		m_IdleServices.erase(service);
		m_PendingServices.erase(service);
	}
}

void CheckerComponent::NextCheckChangedHandler(const Service::Ptr& service)
{
	/* remove and re-insert the service from the set in order to force an index update */
	typedef nth_index<ServiceSet, 0>::type ServiceView;
	ServiceView& idx = boost::get<0>(m_IdleServices);

	ServiceView::iterator it = idx.find(service);
	if (it == idx.end())
		return;

	idx.erase(it);
	idx.insert(service);
}

void CheckerComponent::ObjectRemovedHandler(const DynamicObject::Ptr& object)
{
	Service::Ptr service = dynamic_pointer_cast<Service>(object);

	/* ignore it if the removed object is not a service */
	if (!service)
		return;

	m_IdleServices.erase(service);
	m_PendingServices.erase(service);
}

EXPORT_COMPONENT(checker, CheckerComponent);

