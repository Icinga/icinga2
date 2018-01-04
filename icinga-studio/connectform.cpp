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

#include "icinga-studio/connectform.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

ConnectForm::ConnectForm(wxWindow *parent, const Url::Ptr& url)
	: ConnectFormBase(parent)
{
#ifdef _WIN32
	SetIcon(wxICON(icinga));
#endif /* _WIN32 */

	std::string authority = url->GetAuthority();

	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, authority, boost::is_any_of("@"));

	if (tokens.size() > 1) {
		std::vector<std::string> userinfo;
		boost::algorithm::split(userinfo, tokens[0], boost::is_any_of(":"));

		m_UserText->SetValue(userinfo[0]);
		m_PasswordText->SetValue(userinfo[1]);
	}

	std::vector<std::string> hostport;
	boost::algorithm::split(hostport, tokens.size() > 1 ? tokens[1] : tokens[0], boost::is_any_of(":"));

	m_HostText->SetValue(hostport[0]);

	if (hostport.size() > 1)
		m_PortText->SetValue(hostport[1]);
	else
		m_PortText->SetValue("5665");

	SetDefaultItem(m_ButtonsOK);
}

Url::Ptr ConnectForm::GetUrl() const
{
	wxString url = "https://" + m_UserText->GetValue() + ":" + m_PasswordText->GetValue()
		+ "@" + m_HostText->GetValue() + ":" + m_PortText->GetValue() + "/";

	return new Url(url.ToStdString());
}
