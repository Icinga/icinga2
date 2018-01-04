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

#ifndef MAINFORM_H
#define MAINFORM_H

#include "remote/apiclient.hpp"
#include "remote/url.hpp"
#include "base/exception.hpp"
#include "icinga-studio/forms.h"

namespace icinga
{

class MainForm final : public MainFormBase
{
public:
	MainForm(wxWindow *parent, const Url::Ptr& url);

	void OnQuitClicked(wxCommandEvent& event) override;
	void OnAboutClicked(wxCommandEvent& event) override;
	void OnTypeSelected(wxTreeEvent& event) override;
	void OnObjectSelected(wxListEvent& event) override;

private:
	ApiClient::Ptr m_ApiClient;
	std::map<String, ApiType::Ptr> m_Types;

	void TypesCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiType::Ptr>& types, bool forward);
	void ObjectsCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiObject::Ptr>& objects, bool forward);
	void ObjectDetailsCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiObject::Ptr>& objects, bool forward);

	wxPGProperty *ValueToProperty(const String& name, const Value& value);
};

}

#endif /* MAINFORM_H */
