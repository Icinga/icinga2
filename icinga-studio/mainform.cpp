/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga-studio/mainform.hpp"
#include "icinga-studio/aboutform.hpp"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <wx/msgdlg.h>

using namespace icinga;

MainForm::MainForm(wxWindow *parent, const Url::Ptr& url)
	: MainFormBase(parent)
{
#ifdef _WIN32
	SetIcon(wxICON(icinga));
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
#endif /* _WIN32 */

	String port = url->GetPort();

	if (port.IsEmpty())
		port = "5665";

	m_ApiClient = new ApiClient(url->GetHost(), port, url->GetUsername(), url->GetPassword());
	m_ApiClient->GetTypes(boost::bind(&MainForm::TypesCompletionHandler, this, _1, _2, true));

	std::string title = url->Format() + " - Icinga Studio";
	SetTitle(title);

	m_ObjectsList->InsertColumn(0, "Name", 0, 300);
}

void MainForm::TypesCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiType::Ptr>& types, bool forward)
{
	if (forward) {
		CallAfter(boost::bind(&MainForm::TypesCompletionHandler, this, eptr, types, false));
		return;
	}

	m_TypesTree->DeleteAllItems();

	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			std::string message = "HTTP query failed: " + std::string(ex.what());
			wxMessageBox(message, "Icinga Studio", wxOK | wxCENTRE | wxICON_ERROR, this);
			Close();
			return;
		}
	}

	wxTreeItemId rootNode = m_TypesTree->AddRoot("root");

	bool all = false;
	std::map<String, wxTreeItemId> items;

	m_Types.clear();

	while (!all) {
		all = true;

		BOOST_FOREACH(const ApiType::Ptr& type, types) {
			std::string name = type->Name;

			if (items.find(name) != items.end())
				continue;

			all = false;

			wxTreeItemId parent;
			
			if (type->BaseName.IsEmpty())
				parent = rootNode;
			else {
				std::map<String, wxTreeItemId>::const_iterator it = items.find(type->BaseName);

				if (it == items.end())
					continue;

				parent = it->second;
			}

			m_Types[name] = type;
			items[name] = m_TypesTree->AppendItem(parent, name, 0);
		}
	}
}

void MainForm::OnTypeSelected(wxTreeEvent& event)
{
	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	std::vector<String> attrs;
	attrs.push_back(type->Name.ToLower() + ".__name");

	m_ApiClient->GetObjects(type->PluralName, boost::bind(&MainForm::ObjectsCompletionHandler, this, _1, _2, true),
	    std::vector<String>(), attrs);
}

void MainForm::ObjectsCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiObject::Ptr>& objects, bool forward)
{
	if (forward) {
		CallAfter(boost::bind(&MainForm::ObjectsCompletionHandler, this, eptr, objects, false));
		return;
	}

	m_ObjectsList->DeleteAllItems();
	m_PropertyGrid->Clear();

	if (eptr) {

		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			std::string message = "HTTP query failed: " + std::string(ex.what());
			wxMessageBox(message, "Icinga Studio", wxOK | wxCENTRE | wxICON_ERROR, this);
			return;
		}
	}

	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	String nameAttr = type->Name.ToLower() + ".__name";

	BOOST_FOREACH(const ApiObject::Ptr& object, objects) {
		std::map<String, Value>::const_iterator it = object->Attrs.find(nameAttr);
		if (it == object->Attrs.end())
			continue;
		String name = it->second;
		m_ObjectsList->InsertItem(0, name.GetData());
	}
}

void MainForm::OnObjectSelected(wxListEvent& event)
{
	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	long itemIndex = -1;
	std::string objectName;

	while ((itemIndex = m_ObjectsList->GetNextItem(itemIndex,
		wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != wxNOT_FOUND) {
		objectName = m_ObjectsList->GetItemText(itemIndex);
		break;
	}

	if (objectName.empty())
		return;

	std::vector<String> names;
	names.push_back(objectName);

	m_ApiClient->GetObjects(type->PluralName, boost::bind(&MainForm::ObjectDetailsCompletionHandler, this, _1, _2, true), names);
}

wxPGProperty *MainForm::ValueToProperty(const String& name, const Value& value)
{
	wxPGProperty *prop;

	if (value.IsNumber()) {
		double val = value;
		return new wxFloatProperty(name.GetData(), wxPG_LABEL, value);
	} else if (value.IsBoolean()) {
		bool val = value;
		return new wxBoolProperty(name.GetData(), wxPG_LABEL, value);
	} else if (value.IsObjectType<Array>()) {
		wxArrayString val;
		Array::Ptr arr = value;
		ObjectLock olock(arr);
		BOOST_FOREACH(const Value& aitem, arr)
		{
			String val1 = aitem;
			val.Add(val1.GetData());
		}

		return new wxArrayStringProperty(name.GetData(), wxPG_LABEL, val);
	} else if (value.IsObjectType<Dictionary>()) {
		wxStringProperty *prop = new wxStringProperty(name.GetData(), wxPG_LABEL, "<dictionary>");
		
		Dictionary::Ptr dict = value;
		ObjectLock olock(dict);
		BOOST_FOREACH(const Dictionary::Pair& kv, dict) {
			prop->AppendChild(ValueToProperty(kv.first, kv.second));
		}

		return prop;
	} else {
		String val = value;
		return new wxStringProperty(name.GetData(), wxPG_LABEL, val.GetData());
	}
}

void MainForm::ObjectDetailsCompletionHandler(boost::exception_ptr eptr, const std::vector<ApiObject::Ptr>& objects, bool forward)
{
	if (forward) {
		CallAfter(boost::bind(&MainForm::ObjectDetailsCompletionHandler, this, eptr, objects, false));
		return;
	}

	m_PropertyGrid->Clear();

	if (eptr) {
		try {
			boost::rethrow_exception(eptr);
		} catch (const std::exception& ex) {
			std::string message = "HTTP query failed: " + std::string(ex.what());
			wxMessageBox(message, "Icinga Studio", wxOK | wxCENTRE | wxICON_ERROR, this);
		}
	}

	wxTreeItemId selectedId = m_TypesTree->GetSelection();
	wxString typeName = m_TypesTree->GetItemText(selectedId);
	ApiType::Ptr type = m_Types[typeName.ToStdString()];

	String nameAttr = type->Name.ToLower() + ".__name";

	if (objects.empty())
		return;

	ApiObject::Ptr object = objects[0];

	std::map<String, wxStringProperty *> parents;

	typedef std::pair<String, Value> kv_pair;
	BOOST_FOREACH(const kv_pair& kv, object->Attrs) {
		std::vector<String> tokens;
		boost::algorithm::split(tokens, kv.first, boost::is_any_of("."));

		std::map<String, wxStringProperty *>::const_iterator it = parents.find(tokens[0]);

		wxStringProperty *parent;

		if (it == parents.end()) {
			parent = new wxStringProperty(tokens[0].GetData(), wxPG_LABEL, "<object>");
			m_PropertyGrid->Append(parent);
			parents[tokens[0]] = parent;
		} else
			parent = it->second;

		wxPGProperty *prop = ValueToProperty(tokens[1], kv.second);
		parent->AppendChild(prop);
		m_PropertyGrid->SetPropertyReadOnly(prop);
	}
}

void MainForm::OnQuitClicked(wxCommandEvent& event)
{
	Close();
}

void MainForm::OnAboutClicked(wxCommandEvent& event)
{
	AboutForm form(this);
	form.ShowModal();
}
